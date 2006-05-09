/***************************************************************************
 * Header for functions to handle themes.
 * Copyright (C) 2006 Joe Wingbermuehle
 ***************************************************************************/

#ifndef THEME_H
#define THEME_H

#include "color.h"

struct ThemeImageNode;

typedef enum {

	PART_T1,  /* Top-left corner. 1x */
	PART_T2,  /* Behind menu button. 1x */
	PART_T3,  /* 1x */
	PART_T4,  /* Ax */
	PART_T5,  /* 1x */
	PART_T6,  /* Behind window title. Bx */
	PART_T7,  /* 1x */
	PART_T8,  /* Ax */
	PART_T9,  /* 1x */
	PART_T10, /* Behind window buttons. Cx */
	PART_T11, /* Top-right corner. 1x */

	PART_T,   /* Top for windows without a title bar. */
	PART_TL,  /* Top-left for windows without a title bar. */
	PART_TR,  /* Top-right for windows without a title bar. */

	PART_L,   /* Left border. */
	PART_R,   /* Right border. */
	PART_B,   /* Bottom border. */
	PART_BL,  /* Bottom-left. */
	PART_BR,  /* Bottom-right. */

	PART_MIN,
	PART_MAX,
	PART_CLOSE,

	PART_BUTTON_TL,
	PART_BUTTON_TR,
	PART_BUTTON_BL,
	PART_BUTTON_BR,
	PART_BUTTON_T,
	PART_BUTTON_B,
	PART_BUTTON_L,
	PART_BUTTON_R,
	PART_BUTTON_F,

	PART_TRAY_TL,
	PART_TRAY_TR,
	PART_TRAY_BL,
	PART_TRAY_BR,
	PART_TRAY_T,
	PART_TRAY_B,
	PART_TRAY_L,
	PART_TRAY_R,
	PART_TRAY_F,

	PART_MENU_TL,
	PART_MENU_TR,
	PART_MENU_BL,
	PART_MENU_BR,
	PART_MENU_T,
	PART_MENU_B,
	PART_MENU_L,
	PART_MENU_R,
	PART_MENU_F,
	PART_MENU_S,

	PART_COUNT

} PartType;

#define PART_BORDER_START PART_T1
#define PART_BORDER_STOP PART_BR

#define PART_BUTTON PART_BUTTON_TL
#define PART_TRAY PART_TRAY_TL
#define PART_MENU PART_MENU_TL

typedef struct PartNode {
	const char *name;
	const int count;
	struct ThemeImageNode *image;
	int width;
	int height;
} PartNode;

extern PartNode parts[PART_COUNT];

void InitializeTheme();
void StartupTheme();
void ShutdownTheme();
void DestroyTheme();

void DrawThemeOutline(PartType part, ColorType bg, Drawable d, GC g,
	int xoffset, int yoffset, int width, int height, int index);

void DrawThemeBackground(PartType part, ColorType color, Drawable d, GC g,
	int x, int y, int width, int height, int index);

void SetThemePath(const char *path, const char *name);

#endif

