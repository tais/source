// Definitions for globals that the JA2 codebase reads by extern but
// whose owning subsystem hasn't been ported off DirectDraw/Win32 yet.
// As each Phase replaces a subsystem with an SDL3-backed
// implementation, the corresponding globals move out of this file
// into the new implementation TUs.

#include "types.h"

// (sgp.cpp's globals -- gfProgramIsRunning, gfApplicationActive,
// gfGameInitialized, gzCommandLine, gzErrorMsg, iWindowedMode,
// guiMouseWheelMsg, g_bUseXML_Structures, gfDontUseDDBlits,
// gfIgnoreMessages -- now live in sgp.cpp directly; the WinMain
// rewrite made that TU unconditional.)

// ---- video.cpp (Phase 5) ---------------------------------------------------
UINT32  CurrentSurface          = 0;
INT32   giNumFrames             = 0;
BOOLEAN gfNextRefreshFullScreen = FALSE;
// gpFrameData (the framebuffer) -- not defined here; Phase 5 will
// allocate it when the SDL_Texture-backed renderer lands.

// (giMemUsedInSurfaces + ghFrameBuffer are now owned by sdl_vsurface.cpp.)

// Intro-screen globals moved back to Ja2/Intro.cpp in Phase 6u, when
// libsmacker took over for binkw32 and the Intro module stopped being
// Win32-only.

// ---- WinFont.cpp (Phase 9) -------------------------------------------------
// On Windows the real WinFont.cpp (GDI font system) is still compiled
// and owns these symbols, so only define the portable placeholders on
// non-Windows. Phase 9 retires WinFont.cpp entirely.
#ifndef _WIN32
INT32   TOOLTIP_IFONT         = -1;
INT32   TOOLTIP_IFONT_BOLD    = -1;
#ifndef MAX_WINFONTMAP
#define MAX_WINFONTMAP 25
#endif
INT32   WinFontMap[MAX_WINFONTMAP] = {0};
#endif
