/*************************************************************************
 * Functions for displaying window move/resize status.
 * Copyright (C) 2004 Joe Wingbermuehle
 *************************************************************************/

#include "jwm.h"

static Window statusWindow;
static GC statusGC;
static unsigned int statusWindowHeight;
static unsigned int statusWindowWidth;

static void DrawMoveResizeWindow();

/*************************************************************************
 *************************************************************************/
void CreateMoveWindow() {
	XSetWindowAttributes attrs;
	int x, y;

	statusWindowHeight = GetStringHeight(FONT_MENU) + 8;
	statusWindowWidth = GetStringWidth(FONT_MENU, " 00000 x 00000 ");

	x = rootWidth / 2 - statusWindowWidth / 2;
	y = rootHeight / 2 - statusWindowHeight / 2;

	attrs.background_pixel = colors[COLOR_MENU_BG];
	attrs.save_under = True;
	attrs.override_redirect = True;

	statusWindow = JXCreateWindow(display, rootWindow, x, y,
		statusWindowWidth, statusWindowHeight, 0,
		CopyFromParent, InputOutput, CopyFromParent,
		CWBackPixel | CWOverrideRedirect | CWSaveUnder,
		&attrs);

	JXMapRaised(display, statusWindow);
	statusGC = JXCreateGC(display, statusWindow, 0, NULL);
	JXSetBackground(display, statusGC, colors[COLOR_MENU_BG]);

}

/*************************************************************************
 *************************************************************************/
void DrawMoveResizeWindow() {

	JXSetForeground(display, statusGC, colors[COLOR_MENU_BG]);
	JXFillRectangle(display, statusWindow, statusGC, 2, 2,
		statusWindowWidth - 3, statusWindowHeight - 3);

	JXSetForeground(display, statusGC, colors[COLOR_MENU_UP]);
	JXDrawLine(display, statusWindow, statusGC,
		0, 0, statusWindowWidth - 1, 0);
	JXDrawLine(display, statusWindow, statusGC,
		0, 1, statusWindowWidth - 2, 1);
	JXDrawLine(display, statusWindow, statusGC,
		0, 2, 0, statusWindowHeight - 1);
	JXDrawLine(display, statusWindow, statusGC,
		1, 2, 1, statusWindowHeight - 2);

	JXSetForeground(display, statusGC, colors[COLOR_MENU_DOWN]);
	JXDrawLine(display, statusWindow, statusGC,
		1, statusWindowHeight - 1, statusWindowWidth - 1,
		statusWindowHeight - 1);
	JXDrawLine(display, statusWindow, statusGC,
		2, statusWindowHeight - 2, statusWindowWidth - 1,
		statusWindowHeight - 2);
	JXDrawLine(display, statusWindow, statusGC,
		statusWindowWidth - 1, 1, statusWindowWidth - 1,
		statusWindowHeight - 3);
	JXDrawLine(display, statusWindow, statusGC,
		statusWindowWidth - 2, 2, statusWindowWidth - 2,
		statusWindowHeight - 3);

}

/*************************************************************************
 *************************************************************************/
void UpdateMoveWindow(int x, int y) {
	char str[80];
	unsigned int width;

	DrawMoveResizeWindow();

	snprintf(str, sizeof(str), "(%d, %d)", x, y);
	width = GetStringWidth(FONT_MENU, str);
	RenderString(statusWindow, statusGC, FONT_MENU, COLOR_MENU_FG,
		statusWindowWidth / 2 - width / 2, 4, rootWidth, str);

	JXFlush(display);
}

/*************************************************************************
 *************************************************************************/
void DestroyMoveWindow() {
	if(statusWindow != None) {
		JXFreeGC(display, statusGC);
		JXDestroyWindow(display, statusWindow);
		statusWindow = None;
	}
}

/*************************************************************************
 *************************************************************************/
void CreateResizeWindow() {
	CreateMoveWindow();
}

/*************************************************************************
 *************************************************************************/
void UpdateResizeWindow(int width, int height) {
	char str[80];
	unsigned int fontWidth;

	DrawMoveResizeWindow();

	snprintf(str, sizeof(str), "%d x %d", width, height);
	fontWidth = GetStringWidth(FONT_MENU, str);
	RenderString(statusWindow, statusGC, FONT_MENU, COLOR_MENU_FG,
		statusWindowWidth / 2 - fontWidth / 2, 4, rootWidth, str);

	JXFlush(display);
}

/*************************************************************************
 *************************************************************************/
void DestroyResizeWindow() {
	DestroyMoveWindow();
}

