//----------------------------------------------------------------------------------
// Cinematics Module -- libsmacker-backed implementation.
//
// Phase 6u replaced the proprietary Smacker decoder (binkw32.lib /
// SMACK.H from RAD Game Tools, Win32 only) with libsmacker, an
// open-source SMK decoder. Same public API the rest of the engine
// already calls (SmkInitialize / SmkPlayFlic / SmkPollFlics /
// SmkCloseFlic / SmkShutdown), so Intro.cpp didn't need a rewrite
// beyond dropping the _WIN32 gate that hid the whole VideoPlayer
// class on non-Windows builds.
//
// Frame display: each polled frame's 8-bit indexed pixels get
// expanded to RGB565 through the SMK's current palette and blitted
// straight into FRAME_BUFFER at (uiLeft, uiTop). The caller's normal
// RefreshScreen presents it. Frame timing comes from libsmacker's
// usf (microseconds per frame); we advance to the next frame when
// real time has elapsed past the current frame's display deadline.
//
// Audio decode is Stage C -- this stage is video-only. Cinematics
// run silent until then.
//
// Bink (.BIK) support stays absent. JA2's shipped data has no .BIK
// files, just .SMK, so the BinkInitialize / BinkPlayFlic stubs in
// Cinematics Bink.cpp keep returning failure and the VideoPlayer
// just never gets a chance to use them.
//----------------------------------------------------------------------------------

#include "types.h"
#include "Cinematics.h"
#include "FileMan.h"
#include "DEBUG.H"
#include "vsurface.h"

extern "C" {
#include "smacker.h"
}

#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

static uint64_t NowNs()
{
	using namespace std::chrono;
	return (uint64_t)duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

extern UINT16 SCREEN_WIDTH;
extern UINT16 SCREEN_HEIGHT;

namespace {

// SMKFLIC uiFlags
constexpr UINT32 SMK_FLIC_OPEN      = 0x00000001;
constexpr UINT32 SMK_FLIC_PLAYING   = 0x00000002;
constexpr UINT32 SMK_FLIC_LOOP      = 0x00000004;
constexpr UINT32 SMK_FLIC_AUTOCLOSE = 0x00000008;

constexpr int SMK_NUM_FLICS = 4;

} // namespace

struct SMKFLIC
{
	smk           smkHandle      = nullptr;     // libsmacker handle (NULL when slot is free)
	UINT32        uiFlags        = 0;
	UINT32        uiLeft         = 0;           // top-left blit position into FRAME_BUFFER
	UINT32        uiTop          = 0;
	UINT32        uiWidth        = 0;           // decoded video dimensions
	UINT32        uiHeight       = 0;
	UINT32        uiFrameCount   = 0;           // total frame count
	double        dUsecPerFrame  = 0.0;
	uint64_t      uiFrameStartNs = 0;           // wall-clock ns at which the current frame started displaying
	bool          fFirstFrame    = false;       // true == next poll calls smk_first() rather than smk_next()
	std::vector<uint8_t> rawFile;               // backing memory for smk_open_memory; must outlive smkHandle
};

namespace {

SMKFLIC gSmkList[SMK_NUM_FLICS];
bool    gFsuspendFlics = false;

SMKFLIC* SmkGetFreeFlic()
{
	for (int i = 0; i < SMK_NUM_FLICS; ++i) {
		if (!(gSmkList[i].uiFlags & SMK_FLIC_OPEN)) return &gSmkList[i];
	}
	return nullptr;
}

// Convert SMK 8-bit indexed frame -> RGB565 and blit into FRAME_BUFFER
// at (dstX, dstY). Clips against the framebuffer bounds. Palette is
// SMK_PALETTE_SIZE * 3 bytes of RGB888.
void BlitFrameToFrameBuffer(SMKFLIC& f, const unsigned char* palette, const unsigned char* pixels)
{
	UINT32 pitchBytes = 0;
	UINT16* fb = (UINT16*)LockVideoSurface(FRAME_BUFFER, &pitchBytes);
	if (!fb) return;
	const int stridePx = (int)(pitchBytes / sizeof(UINT16));

	const int dstX0 = (int)f.uiLeft;
	const int dstY0 = (int)f.uiTop;
	const int srcW  = (int)f.uiWidth;
	const int srcH  = (int)f.uiHeight;
	const int dstXEnd = dstX0 + srcW;
	const int dstYEnd = dstY0 + srcH;
	const int clipL   = dstX0 < 0 ? -dstX0 : 0;
	const int clipT   = dstY0 < 0 ? -dstY0 : 0;
	const int clipR   = dstXEnd > (int)SCREEN_WIDTH  ? dstXEnd - (int)SCREEN_WIDTH  : 0;
	const int clipB   = dstYEnd > (int)SCREEN_HEIGHT ? dstYEnd - (int)SCREEN_HEIGHT : 0;
	const int copyW   = srcW - clipL - clipR;
	const int copyH   = srcH - clipT - clipB;
	if (copyW <= 0 || copyH <= 0) {
		UnLockVideoSurface(FRAME_BUFFER);
		return;
	}

	for (int y = 0; y < copyH; ++y) {
		const unsigned char* srcRow = pixels + (clipT + y) * srcW + clipL;
		UINT16* dstRow = fb + (dstY0 + clipT + y) * stridePx + (dstX0 + clipL);
		for (int x = 0; x < copyW; ++x) {
			const unsigned char idx = srcRow[x];
			const unsigned char r8 = palette[idx * 3 + 0];
			const unsigned char g8 = palette[idx * 3 + 1];
			const unsigned char b8 = palette[idx * 3 + 2];
			dstRow[x] = (UINT16)(((r8 >> 3) << 11) | ((g8 >> 2) << 5) | (b8 >> 3));
		}
	}
	UnLockVideoSurface(FRAME_BUFFER);
}

void DecodeAndBlitCurrentFrame(SMKFLIC& f)
{
	const unsigned char* pal = smk_get_palette(f.smkHandle);
	const unsigned char* px  = smk_get_video(f.smkHandle);
	if (pal && px) BlitFrameToFrameBuffer(f, pal, px);
}

} // namespace

// ---- Public API -----------------------------------------------------------

void SmkInitialize(void* /*hWindow*/, UINT32 /*uiWidth*/, UINT32 /*uiHeight*/)
{
	// Wipe all flic slots. The window-size arguments are vestigial -- the
	// legacy DirectDraw path needed them to set up a video surface; we
	// blit straight into the shared FRAME_BUFFER instead.
	for (int i = 0; i < SMK_NUM_FLICS; ++i) {
		gSmkList[i] = SMKFLIC{};
	}
	gFsuspendFlics = false;
}

void SmkShutdown(void)
{
	for (int i = 0; i < SMK_NUM_FLICS; ++i) {
		if (gSmkList[i].uiFlags & SMK_FLIC_OPEN) {
			SmkCloseFlic(&gSmkList[i]);
		}
	}
}

SMKFLIC* SmkOpenFlic(const CHAR8* cFilename)
{
	SMKFLIC* p = SmkGetFreeFlic();
	if (!p) {
		std::fprintf(stderr, "[smk] no free flic slots\n");
		return nullptr;
	}

	// Load the file through FileMan so SLF archives (Intro.slf) work.
	HWFILE h = FileOpen(const_cast<STR>(cFilename), FILE_ACCESS_READ | FILE_OPEN_EXISTING, FALSE);
	if (!h) {
		std::fprintf(stderr, "[smk] FileOpen failed: %s\n", cFilename);
		return nullptr;
	}
	const UINT32 size = FileGetSize(h);
	if (size == 0) {
		FileClose(h);
		return nullptr;
	}
	p->rawFile.assign(size, 0);
	UINT32 bytesRead = 0;
	FileRead(h, p->rawFile.data(), size, &bytesRead);
	FileClose(h);
	if (bytesRead != size) {
		std::fprintf(stderr, "[smk] short read on %s: %u/%u\n", cFilename, bytesRead, size);
		p->rawFile.clear();
		return nullptr;
	}

	// SMK_MODE_MEMORY (0x00) -- libsmacker keeps the whole compressed
	// stream in memory and decodes frames on demand. Fits our small SMK
	// files (helicopter intro is ~4MB) and avoids holding a FILE* open
	// inside the decoder while the rest of the game does I/O.
	p->smkHandle = smk_open_memory(p->rawFile.data(), p->rawFile.size());
	if (!p->smkHandle) {
		std::fprintf(stderr, "[smk] smk_open_memory failed for %s\n", cFilename);
		p->rawFile.clear();
		return nullptr;
	}

	unsigned long w = 0, h_ = 0;
	unsigned char y_scale = 0;
	smk_info_video(p->smkHandle, &w, &h_, &y_scale);
	unsigned long fc = 0;
	double usf = 0.0;
	smk_info_all(p->smkHandle, nullptr, &fc, &usf);
	p->uiWidth       = (UINT32)w;
	p->uiHeight      = (UINT32)h_;
	p->uiFrameCount  = (UINT32)fc;
	p->dUsecPerFrame = usf;

	smk_enable_video(p->smkHandle, 1);
	// Audio stays disabled until Stage C wires it through SDL3_mixer.

	p->uiFlags     = SMK_FLIC_OPEN;
	p->fFirstFrame = true;
	return p;
}

SMKFLIC* SmkPlayFlic(const CHAR8* cFilename, UINT32 uiLeft, UINT32 uiTop, BOOLEAN fAutoClose)
{
	SMKFLIC* p = SmkOpenFlic(cFilename);
	if (!p) return nullptr;
	SmkSetBlitPosition(p, uiLeft, uiTop);
	p->uiFlags |= SMK_FLIC_PLAYING;
	if (fAutoClose) p->uiFlags |= SMK_FLIC_AUTOCLOSE;
	return p;
}

void SmkSetBlitPosition(SMKFLIC* pSmack, UINT32 uiLeft, UINT32 uiTop)
{
	if (!pSmack) return;
	pSmack->uiLeft = uiLeft;
	pSmack->uiTop  = uiTop;
}

void SmkCloseFlic(SMKFLIC* pSmack)
{
	if (!pSmack) return;
	if (pSmack->smkHandle) {
		smk_close(pSmack->smkHandle);
		pSmack->smkHandle = nullptr;
	}
	pSmack->rawFile.clear();
	pSmack->rawFile.shrink_to_fit();
	pSmack->uiFlags = 0;
	pSmack->fFirstFrame = false;
}

BOOLEAN SmkPollFlics(void)
{
	bool any = false;
	const uint64_t nowNs = NowNs();
	for (int i = 0; i < SMK_NUM_FLICS; ++i) {
		SMKFLIC& f = gSmkList[i];
		if (!(f.uiFlags & SMK_FLIC_PLAYING)) continue;
		any = true;
		if (gFsuspendFlics) continue;

		// First poll: prime the decoder on frame 0 and draw it.
		if (f.fFirstFrame) {
			if (smk_first(f.smkHandle) < 0) {
				std::fprintf(stderr, "[smk] smk_first failed\n");
				if (f.uiFlags & SMK_FLIC_AUTOCLOSE) SmkCloseFlic(&f);
				continue;
			}
			f.fFirstFrame    = false;
			f.uiFrameStartNs = nowNs;
			DecodeAndBlitCurrentFrame(f);
			continue;
		}

		// Has the current frame's display deadline passed?
		const uint64_t frameNs = (uint64_t)(f.dUsecPerFrame * 1000.0);
		if (nowNs - f.uiFrameStartNs < frameNs) continue;

		const char rc = smk_next(f.smkHandle);
		if (rc == SMK_DONE) {
			// Reached the end. Loop or close depending on flags.
			if (f.uiFlags & SMK_FLIC_LOOP) {
				if (smk_first(f.smkHandle) < 0) {
					if (f.uiFlags & SMK_FLIC_AUTOCLOSE) SmkCloseFlic(&f);
					continue;
				}
				f.uiFrameStartNs = nowNs;
				DecodeAndBlitCurrentFrame(f);
			} else if (f.uiFlags & SMK_FLIC_AUTOCLOSE) {
				SmkCloseFlic(&f);
				// Slot freed; loop iteration handled, next loop sees uiFlags=0.
			} else {
				f.uiFlags &= ~SMK_FLIC_PLAYING;
			}
		} else if (rc < 0) {
			std::fprintf(stderr, "[smk] smk_next failed\n");
			if (f.uiFlags & SMK_FLIC_AUTOCLOSE) SmkCloseFlic(&f);
		} else {
			f.uiFrameStartNs = nowNs;
			DecodeAndBlitCurrentFrame(f);
		}
	}

	return any ? TRUE : FALSE;
}
