/****************************************************************************
 * Outlines for moving and resizing.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"

static GC outlineGC;

static int lastX, lastY;
static int lastWidth, lastHeight;
static int outlineDrawn;

/****************************************************************************
 ****************************************************************************/
void InitializeOutline() {
}

/****************************************************************************
 ****************************************************************************/
void StartupOutline() {

	XGCValues gcValues;

	gcValues.function = GXinvert;
	gcValues.subwindow_mode = IncludeInferiors;
	gcValues.line_width = 2;
	outlineGC = JXCreateGC(display, rootWindow,
		GCFunction | GCSubwindowMode | GCLineWidth, &gcValues);
	outlineDrawn = 0;

}

/****************************************************************************
 ****************************************************************************/
void ShutdownOutline() {
	JXFreeGC(display, outlineGC);
}

/****************************************************************************
 ****************************************************************************/
void DestroyOutline() {
}

/****************************************************************************
 ****************************************************************************/
void DrawOutline(int x, int y, int width, int height) {
	if(!outlineDrawn) {
		JXSync(display, False);
		JXGrabServer(display);
		JXDrawRectangle(display, rootWindow, outlineGC, x, y, width, height);
		lastX = x;
		lastY = y;
		lastWidth = width;
		lastHeight = height;
		outlineDrawn = 1;
	}
}

/****************************************************************************
 ****************************************************************************/
void ClearOutline() {
	if(outlineDrawn) {
		JXDrawRectangle(display, rootWindow, outlineGC,
			lastX, lastY, lastWidth, lastHeight);
		outlineDrawn = 0;
		JXUngrabServer(display);
		JXSync(display, False);
	}
}

