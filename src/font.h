/****************************************************************************
 * Header for the font functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef FONT_H
#define FONT_H

/*extern XFontStruct *fonts[FONT_COUNT];*/

void InitializeFonts();
void StartupFonts();
void ShutdownFonts();
void DestroyFonts();

void SetFont(FontType type, const char *value);

void RenderString(Drawable d, GC gc, FontType font, ColorType color,
	int x, int y, int width, const char *str);

int GetStringWidth(FontType type, const char *str);
int GetStringHeight(FontType type);

#endif

