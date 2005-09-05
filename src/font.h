/****************************************************************************
 * Header for the font functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef FONT_H
#define FONT_H

extern XFontStruct *fonts[FONT_COUNT];

void InitializeFonts();
void StartupFonts();
void ShutdownFonts();
void DestroyFonts();

void SetFont(FontType type, const char *value, int aa);

void RenderString(Drawable d, GC gc, FontType font, RampType ramp,
	int x, int y, int width, const char *str);

#endif

