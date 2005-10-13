/****************************************************************************
 * Functions for displaying the pager.
 * Copyright (C) 2004 Joe Wingbermuehle
 * TODO: Support vertical pagers (arbitrary rectangles?).
 ****************************************************************************/

#include "jwm.h"

typedef struct PagerType {

	TrayComponentType *cp;

	int deskWidth;
	double scalex, scaley;

	Pixmap buffer;
	GC bufferGC;

	struct PagerType *next;

} PagerType;

static PagerType *pagers;

static void Create(TrayComponentType *cp);
static void Destroy(TrayComponentType *cp);

static void SetSize(TrayComponentType *cp, int width, int height);

static void ProcessPagerButtonEvent(TrayComponentType *cp,
	int x, int y, int mask);

static void DrawPagerClient(const PagerType *pp, const ClientNode *np);

/****************************************************************************
 ****************************************************************************/
void InitializePager() {
	pagers = NULL;
}

/****************************************************************************
 ****************************************************************************/
void StartupPager() {
}

/****************************************************************************
 ****************************************************************************/
void ShutdownPager() {

	PagerType *pp;

	while(pagers) {
		pp = pagers->next;

		JXFreeGC(display, pagers->bufferGC);
		JXFreePixmap(display, pagers->buffer);

		Release(pagers);
		pagers = pp;
	}

}

/****************************************************************************
 ****************************************************************************/
void DestroyPager() {
}

/****************************************************************************
 ****************************************************************************/
TrayComponentType *CreatePager() {

	TrayComponentType *cp;
	PagerType *pp;

	pp = Allocate(sizeof(PagerType));
	pp->next = pagers;
	pagers = pp;

	cp = CreateTrayComponent();
	cp->object = pp;
	pp->cp = cp;
	cp->Create = Create;
	cp->Destroy = Destroy;
	cp->SetSize = SetSize;
	cp->ProcessButtonEvent = ProcessPagerButtonEvent;

	return cp;
}

/****************************************************************************
 ****************************************************************************/
void Create(TrayComponentType *cp) {

	PagerType *pp;

	Assert(cp);

	pp = (PagerType*)cp->object;

	Assert(pp);

	pp->deskWidth = (cp->height * rootWidth) / rootHeight;
	cp->width = (pp->deskWidth + 1) * desktopCount;
	pp->scalex = (double)(pp->deskWidth - 2) / rootWidth;
	pp->scaley = (double)(cp->height - 2) / rootHeight;

	cp->pixmap = JXCreatePixmap(display, rootWindow, cp->width,
		cp->height, rootDepth);
	pp->bufferGC = JXCreateGC(display, cp->pixmap, 0, NULL);
	pp->buffer = cp->pixmap;

}

/****************************************************************************
 ****************************************************************************/
void Destroy(TrayComponentType *cp) {

}

/****************************************************************************
 ****************************************************************************/
void SetSize(TrayComponentType *cp, int width, int height) {

	PagerType *pp;

	Assert(cp);

	pp = (PagerType*)cp->object;

	Assert(pp);

	if(width) {
		/* Vertical pager, compute height from width. */
		cp->width = width;
Assert(0);
	} else if(height) {

		/* Horizontal pager, compute width from height. */
		cp->height = height;
		pp->deskWidth = (cp->height * rootWidth) / rootHeight;
		cp->width = (pp->deskWidth + 1) * desktopCount;

	} else {
		Assert(0);
	}

}

/****************************************************************************
 ****************************************************************************/
void ProcessPagerButtonEvent(TrayComponentType *cp, int x, int y, int mask) {

	PagerType *pp;

	switch(mask) {
	case Button1:
	case Button2:
	case Button3:
		pp = (PagerType*)cp->object;
		ChangeDesktop(x / (pp->deskWidth + 1));
		break;
	case Button4:
		NextDesktop();
		break;
	case Button5:
		PreviousDesktop();
		break;
	default:
		break;
	}
}

/****************************************************************************
 ****************************************************************************/
void UpdatePager() {

	PagerType *pp;
	ClientNode *np;
	Pixmap buffer;
	GC gc;
	int width, height;
	int deskWidth;
	int x;

	for(pp = pagers; pp; pp = pp->next) {

		buffer = pp->cp->pixmap;
		gc = pp->bufferGC;
		width = pp->cp->width;
		height = pp->cp->height;
		deskWidth = pp->deskWidth;

		JXSetForeground(display, gc, colors[COLOR_PAGER_BG]);
		JXFillRectangle(display, buffer, gc, 0, 0,
			(deskWidth + 1) * desktopCount, height);

		JXSetForeground(display, gc, colors[COLOR_PAGER_ACTIVE_BG]);
		JXFillRectangle(display, buffer, gc,
			currentDesktop * (deskWidth + 1), 0,
			deskWidth, height);

		for(x = LAYER_BOTTOM; x <= LAYER_TOP; x++) {
			for(np = nodeTail[x]; np; np = np->prev) {
				DrawPagerClient(pp, np);
			}
		}

		JXSetForeground(display, gc, colors[COLOR_PAGER_FG]);
		for(x = 1; x < desktopCount; x++) {
			JXDrawLine(display, buffer, gc,
				(deskWidth + 1) * x - 1, 0,
				(deskWidth + 1) * x - 1, height);
		}

		UpdateSpecificTray(pp->cp->tray, pp->cp);

	}

}

/****************************************************************************
 ****************************************************************************/
void DrawPagerClient(const PagerType *pp, const ClientNode *np) {

	int x, y;
	int width, height;
	int deskOffset;

	if(!(np->statusFlags & STAT_MAPPED)) {
		return;
	}

	if(np->statusFlags & STAT_STICKY) {
		deskOffset = (pp->deskWidth + 1) * currentDesktop;
	} else {
		deskOffset = (pp->deskWidth + 1) * np->desktop;
	}

	x = (int)((double)np->x * pp->scalex + 1.0);
	y = (int)((double)np->y * pp->scaley + 1.0);
	width = (int)((double)np->width * pp->scalex);
	height = (int)((double)np->height * pp->scaley);

	if(x + width > pp->deskWidth) {
		width = pp->deskWidth - x;
	}
	if(x < 0) {
		width += x;
		x = 0;
	}
	if(width <= 0 || height <= 0) {
		return;
	}

	x += deskOffset;

	JXSetForeground(display, pp->bufferGC, colors[COLOR_PAGER_OUTLINE]);
	JXDrawRectangle(display, pp->cp->pixmap, pp->bufferGC,
		x, y, width, height);

	if(width > 1 && height > 1) {
		if((np->statusFlags & STAT_ACTIVE)
			&& (np->desktop == currentDesktop
			|| (np->statusFlags & STAT_STICKY))) {
			JXSetForeground(display, pp->bufferGC, colors[COLOR_PAGER_ACTIVE_FG]);
		} else {
			JXSetForeground(display, pp->bufferGC, colors[COLOR_PAGER_FG]);
		}
	}

	JXFillRectangle(display, pp->cp->pixmap, pp->bufferGC, x + 1, y + 1,
		width - 1, height - 1);

}

