/****************************************************************************
 * Header for the font functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef FONT_H
#define FONT_H

#include "color.h"

typedef enum {

	FONT_BORDER,
	FONT_MENU,
	FONT_TASK,
	FONT_POPUP,
	FONT_CLOCK,
	FONT_TRAY,
	FONT_TRAYBUTTON,

	FONT_COUNT

} FontType;

void InitializeFonts();
void StartupFonts();
void ShutdownFonts();
void DestroyFonts();

void SetFont(FontType type, const char *value);

void RenderString(Drawable d, FontType font, ColorType color,
	int x, int y, int width, Region region, const char *str);

int GetStringWidth(FontType type, const char *str);
int GetStringHeight(FontType type);

#endif

