/****************************************************************************
 * Header for the color functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef COLOR_H
#define COLOR_H

typedef enum {

	COLOR_BORDER_BG,
	COLOR_BORDER_FG,
	COLOR_BORDER_ACTIVE_BG,
	COLOR_BORDER_ACTIVE_FG,

	COLOR_TRAY_BG,
	COLOR_TRAY_FG,

	COLOR_TASK_BG,
	COLOR_TASK_FG,
	COLOR_TASK_ACTIVE_BG,
	COLOR_TASK_ACTIVE_FG,

	COLOR_PAGER_BG,
	COLOR_PAGER_FG,
	COLOR_PAGER_ACTIVE_BG,
	COLOR_PAGER_ACTIVE_FG,
	COLOR_PAGER_OUTLINE,

	COLOR_MENU_BG,
	COLOR_MENU_FG,
	COLOR_MENU_ACTIVE_BG,
	COLOR_MENU_ACTIVE_FG,

	COLOR_BORDER_UP,
	COLOR_BORDER_DOWN,
	COLOR_BORDER_ACTIVE_UP,
	COLOR_BORDER_ACTIVE_DOWN,

	COLOR_TRAY_UP,
	COLOR_TRAY_DOWN,

	COLOR_TASK_UP,
	COLOR_TASK_DOWN,
	COLOR_TASK_ACTIVE_UP,
	COLOR_TASK_ACTIVE_DOWN,

	COLOR_MENU_UP,
	COLOR_MENU_DOWN,
	COLOR_MENU_ACTIVE_UP,
	COLOR_MENU_ACTIVE_DOWN,

	COLOR_POPUP_BG,
	COLOR_POPUP_FG,
	COLOR_POPUP_OUTLINE,

	COLOR_TRAYBUTTON_BG,
	COLOR_TRAYBUTTON_FG,

	COLOR_CLOCK_BG,
	COLOR_CLOCK_FG,

	COLOR_COUNT

} ColorType;

extern unsigned long colors[COLOR_COUNT];

void InitializeColors();
void StartupColors();
void ShutdownColors();
void DestroyColors();

void SetColor(ColorType c, const char *value);

void GetColor(XColor *c);
void GetColorIndex(XColor *c);
void GetColorFromIndex(XColor *c);

#ifdef USE_XFT
XftColor *GetXftColor(ColorType type);
#endif

#endif

