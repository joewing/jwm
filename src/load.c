/****************************************************************************
 * Functions for obtaining load information.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"

#define LOAD_YOFFSET 2

int loadWidth = 0;

#ifdef SHOW_LOAD

static int loadEnabled = 1;
static int loadHeight = 0;
static float *loads = NULL;

#endif /* SHOW_LOAD */

/****************************************************************************
 ****************************************************************************/
void InitializeLoadDisplay() {
#ifdef SHOW_LOAD

	loadEnabled = 1;

#endif
}

/****************************************************************************
 ****************************************************************************/
void StartupLoadDisplay() {
#ifdef SHOW_LOAD
	int x;

	if(loadEnabled) {

		loadHeight = trayHeight - 4;
		loadWidth = (loadHeight * rootWidth) / rootHeight;

		loads = Allocate(loadWidth * sizeof(float));
		for(x = 0; x < loadWidth; x++) {
			loads[x] = 0.0;
		}
	} else {
		loadWidth = 0;
		loadHeight = 0;
	}

#endif
}

/****************************************************************************
 ****************************************************************************/
void ShutdownLoadDisplay() {
#ifdef SHOW_LOAD
	if(loads) {
		Release(loads);
		loads = NULL;
	}
#endif
}

/****************************************************************************
 ****************************************************************************/
void DestroyLoadDisplay() {
#ifdef SHOW_LOAD
#endif
}

/****************************************************************************
 ****************************************************************************/
void UpdateLoadDisplay(Window w, GC gc, int xoffset) {
#ifdef SHOW_LOAD

	float currentLoad = 0.0;
	int loadLines;
	float divideSize, y;
	int x;

	if(!loadEnabled) {
		return;
	}

	/* Compute the current load. */
	currentLoad = GetLoad();

	/* Update the array and determine how many load lines to draw. */
	loadLines = (int)currentLoad;
	for(x = 0; x < loadWidth - 1; x++) {
		loads[x] = loads[x + 1];
		if((int)loads[x] > loadLines) {
			loadLines = (int)loads[x];
		}
	}
	loads[loadWidth - 1] = currentLoad;

	/* Clear the draw area. */
	JXSetForeground(display, gc, colors[COLOR_LOAD_BG]);
	JXFillRectangle(display, w, gc, xoffset, LOAD_YOFFSET,
		loadWidth, loadHeight + LOAD_YOFFSET - 1);

	/* Draw the graph. */
	JXSetForeground(display, gc, colors[COLOR_LOAD_FG]);
	for(x = 0; x < loadWidth; x++) {
		y = (loads[x] * loadHeight) / (loadLines + 1);
		JXDrawLine(display, w, gc, x + xoffset,
			loadHeight - (int)y + LOAD_YOFFSET,
			x + xoffset, loadHeight + LOAD_YOFFSET);
	}

	/* Draw load lines */
	divideSize = (float)(loadHeight) / (float)(loadLines + 1);
	JXSetForeground(display, gc, colors[COLOR_LOAD_OUTLINE]);
	y = divideSize + LOAD_YOFFSET;
	for(x = 0; x < loadLines; x++) {
		JXDrawLine(display, w, gc, xoffset, (int)y,
			loadWidth + xoffset, (int)y);
		y += divideSize;
	}

	/* Draw the outline. */
	JXSetForeground(display, gc, colors[COLOR_TRAY_DOWN]);
	JXDrawRectangle(display, w, gc, xoffset - 1, 1,
		loadWidth + 1, trayHeight - 3);

#endif /* SHOW_LOAD */

}

/****************************************************************************
 ****************************************************************************/
void SetLoadEnabled(int e) {
#ifdef SHOW_LOAD
	loadEnabled = e;
#endif /* SHOW_LOAD */
}


