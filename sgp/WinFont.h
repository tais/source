#ifndef __WINFONT_
#define __WINFONT_

#ifdef _WIN32
#include <windows.h>

void InitWinFonts( );
void ShutdownWinFonts( );

void InitTooltipFonts();
void ShutdownTooltipFonts();

INT32 CreateWinFont( LOGFONT &logfont );
void	DeleteWinFont( INT32 iFont );

void SetWinFontBackColor( INT32 iFont, COLORVAL *pColor );
void SetWinFontForeColor( INT32 iFont, COLORVAL *pColor );

void PrintWinFont( UINT32 uiDestBuf, INT32 iFont, INT32 x, INT32 y, STR16 pFontString, ...);

INT16 WinFontStringPixLength( STR16	string, INT32 iFont );
INT16 GetWinFontHeight( INT32 iFont );

//if you cahnge this enum, you must change FontInfo struct in WinFont.cpp too.
enum {
WIN_LARGEFONT1 = 0, 
WIN_SMALLFONT1,
WIN_TINYFONT1, 
WIN_12POINTFONT1, 
WIN_COMPFONT,
WIN_SMALLCOMPFONT,
WIN_10POINTROMAN,
WIN_12POINTROMAN,
WIN_14POINTSANSSERIF,
WIN_10POINTARIAL,
WIN_14POINTARIAL,
WIN_12POINTARIAL,
WIN_BLOCKYFONT,
WIN_BLOCKYFONT2,
WIN_10POINTARIALBOLD,
WIN_12POINTARIALFIXEDFONT,
WIN_16POINTARIAL,
WIN_BLOCKFONTNARROW,
WIN_14POINTHUMANIST,
WIN_HUGEFONT,
WIN_LASTFONT
};
#define MAX_WINFONTMAP 25
extern INT32 WinFontMap[MAX_WINFONTMAP];
extern INT32 TOOLTIP_IFONT;
extern INT32 TOOLTIP_IFONT_BOLD;
#else // !_WIN32
// Non-Windows: the GDI-backed TrueType font path is replaced by
// bitmap fonts (or stb_truetype in Phase 9). The functions below are
// stubbed to no-ops so Font.cpp's WinFont call sites compile.
// COLORVAL is typedef'd to UINT32 in vobject.h; we re-use it.
#include "vobject.h"
inline void InitWinFonts() {}
inline void ShutdownWinFonts() {}
inline void InitTooltipFonts() {}
inline void ShutdownTooltipFonts() {}
inline INT32 CreateWinFont(void*) { return -1; }
inline void DeleteWinFont(INT32) {}
inline void SetWinFontBackColor(INT32, COLORVAL*) {}
inline void SetWinFontForeColor(INT32, COLORVAL*) {}
inline void PrintWinFont(UINT32, INT32, INT32, INT32, STR16, ...) {}
inline INT16 WinFontStringPixLength(STR16, INT32) { return 0; }
inline INT16 GetWinFontHeight(INT32) { return 12; }
#define MAX_WINFONTMAP 25
extern INT32 WinFontMap[MAX_WINFONTMAP];
extern INT32 TOOLTIP_IFONT;
extern INT32 TOOLTIP_IFONT_BOLD;
#endif // _WIN32

#endif
