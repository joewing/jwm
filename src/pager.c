/****************************************************************************
 * Functions for displaying the pager.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"

int pagerWidth;

static int pagerEnabled = 1;

static int pagerDeskWidth;

static double scalex, scaley;

static void DrawPagerClient(Window w, GC gc, int xoffset,
	const ClientNode *np);

/****************************************************************************
 ****************************************************************************/
void InitializePager() {
	pagerEnabled = 1;
}

/****************************************************************************
 ****************************************************************************/
void StartupPager() {

	if(pagerEnabled) {
		pagerDeskWidth = (trayHeight * rootWidth) / rootHeight;

		pagerWidth = (pagerDeskWidth + 1) * desktopCount;

		scalex = (double)(pagerDeskWidth - 2) / (double)rootWidth;
		scaley = (double)(trayHeight - 2) / (double)rootHeight;
	} else {	
		pagerWidth = 0;
	}

}

/****************************************************************************
 ****************************************************************************/
void ShutdownPager() {
}

/****************************************************************************
 ****************************************************************************/
void DestroyPager() {
}

/****************************************************************************
 ****************************************************************************/
void DrawPager(Window w, GC gc, int xoffset) {

	ClientNode *np;
	int x;

	if(!pagerEnabled) {
		return;
	}

	JXSetForeground(display, gc, colors[COLOR_PAGER_BG]);
	JXFillRectangle(display, w, gc, xoffset, 0,
		(pagerDeskWidth + 1) * desktopCount, trayHeight);

	JXSetForeground(display, gc, colors[COLOR_PAGER_ACTIVE_BG]);
	JXFillRectangle(display, w, gc,
		currentDesktop * (pagerDeskWidth + 1) + xoffset, 0,
		pagerDeskWidth, trayHeight);

	/* Draw the window outlines */
	for(x = LAYER_BOTTOM; x <= LAYER_TOP; x++) {
		for(np = nodeTail[x]; np; np = np->prev) {
			DrawPagerClient(w, gc, xoffset, np);
		}
	}

	/* Draw the pager outline. */
	JXSetForeground(display, gc, colors[COLOR_TRAY_BG]);

	for(x = 1; x < desktopCount; x++) {
		JXDrawLine(display, w, gc,
			(pagerDeskWidth + 1) * x - 1 + xoffset, 0,
			(pagerDeskWidth + 1) * x - 1 + xoffset, trayHeight - 1);
	}

	JXDrawRectangle(display, w, gc, xoffset, 0,
		(pagerDeskWidth + 1) * desktopCount - 1, trayHeight - 1);

}

/****************************************************************************
 ****************************************************************************/
void DrawPagerClient(Window w, GC gc, int xoffset,
	const ClientNode *np) {

	int x, y, width, height;
	int deskOffset;

	/* Determine if this client should be drawn. */
	if(!(np->statusFlags & STAT_MAPPED)) {
		return;
	}

	/* Determine the destop on which to draw it. */
	if(np->statusFlags & STAT_STICKY) {
		deskOffset = (pagerDeskWidth + 1) * currentDesktop;
	} else {
		deskOffset = (pagerDeskWidth + 1) * np->desktop;
	}

	/* Calculate the scaled coordinates to draw. */
	x = (int)((double)np->x * scalex + 1.0);
	y = (int)((double)np->y * scaley + 1.0);
	width = (int)((double)np->width * scalex);
	height = (int)((double)np->height * scaley);

	/* Check bounds. */
	if(x + width > pagerDeskWidth) {
		width = pagerDeskWidth - x;
	}
	if(x < 0) {
		width += x;
		x = 0;
	}
	if(width <= 0 || height <= 0) {
		return;
	}

	x += deskOffset + xoffset;

	/* Draw. */
	JXSetForeground(display, gc, colors[COLOR_PAGER_OUTLINE]);
	JXDrawRectangle(display, w, gc, x, y, width, height);

	if(width > 1 && height > 1) {
		if(np->statusFlags & STAT_ACTIVE
			&& (np->desktop == currentDesktop
			|| (np->statusFlags & STAT_STICKY))) {
			JXSetForeground(display, gc, colors[COLOR_PAGER_ACTIVE_FG]);
		} else {
			JXSetForeground(display, gc, colors[COLOR_PAGER_FG]);
		}
		JXFillRectangle(display, w, gc, x + 1, y + 1,
			width - 1, height - 1);
	}

}

/****************************************************************************
 ****************************************************************************/
void SetPagerEnabled(int e) {
	pagerEnabled = e;
}

