#ifndef _CINEMATICS_H_
#define _CINEMATICS_H_

#include "types.h"

// SMKFLIC is opaque to callers; the implementation owns the libsmacker
// handle, palette/frame buffers, timing state, and blit position.
// Callers only ever see a pointer returned from SmkPlayFlic/SmkOpenFlic.
struct SMKFLIC;

void    SmkInitialize(void* hWindow, UINT32 uiWidth, UINT32 uiHeight);
void    SmkShutdown(void);

SMKFLIC* SmkPlayFlic(const CHAR8* cFilename, UINT32 uiLeft, UINT32 uiTop, BOOLEAN fAutoClose);

BOOLEAN  SmkPollFlics(void);

SMKFLIC* SmkOpenFlic(const CHAR8* cFilename);
void     SmkSetBlitPosition(SMKFLIC* pSmack, UINT32 uiLeft, UINT32 uiTop);
void     SmkCloseFlic(SMKFLIC* pSmack);

#endif
