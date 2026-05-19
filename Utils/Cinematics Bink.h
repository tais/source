#ifndef _CINEMATICS_BINK__H_
#define _CINEMATICS_BINK__H_

// Bink (.BIK) video support. JA2's shipped data set has no .BIK files
// (every cinematic is .SMK), so this whole subsystem is a placeholder:
// the public API stays so Intro.cpp's VideoPlayer state machine can
// keep the SMK / BINK dispatch logic intact, but every entry point is
// a no-op stub. If you ever need real Bink decoding you'd need to
// either license RAD's Bink SDK or use a reverse-engineered decoder.

#include "types.h"

// BINKFLIC uiFlags
#define BINK_FLIC_OPEN              0x00000001  // Flic is open
#define BINK_FLIC_PLAYING           0x00000002  // Flic is playing
#define BINK_FLIC_LOOP              0x00000004  // Play flic in a loop
#define BINK_FLIC_AUTOCLOSE         0x00000008  // Close when done
#define BINK_FLIC_CENTER_VERTICAL   0x00000010  // Center vertically

struct BINKFLIC;  // opaque -- no Bink files to play means no fields needed

void     BinkInitialize(void* hWindow, UINT32 uiWidth, UINT32 uiHeight);
BOOLEAN  BinkPollFlics(void);
void     BinkCloseFlic(BINKFLIC* pBink);
void     BinkShutdownVideo(void);
BINKFLIC* BinkPlayFlic(const CHAR8* cFilename, UINT32 uiLeft, UINT32 uiTop, UINT32 uiFlags);

#endif
