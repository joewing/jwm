/****************************************************************************
 * Functions for displaying the pager.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"
#include "pager.h"
#include "tray.h"
#include "main.h"
#include "desktop.h"
#include "client.h"
#include "color.h"

typedef struct PagerType {

	TrayComponentType *cp;

	int deskWidth;
	int deskHeight;
	double scalex, scaley;
	LayoutType layout;

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

	for(pp = pagers; pp; pp = pp->next) {
		JXFreeGC(display, pp->bufferGC);
		JXFreePixmap(display, pp->buffer);
	}

}

/****************************************************************************
 ****************************************************************************/
void DestroyPager() {

	PagerType *pp;

	while(pagers) {
		pp = pagers->next;
		Release(pagers);
		pagers = pp;
	}

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

	Assert(cp->width > 0);
	Assert(cp->height > 0);

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
		pp->deskWidth = width;
		pp->deskHeight = (cp->width * rootHeight) / rootWidth;
		cp->height = (pp->deskHeight + 1) * desktopCount;
		pp->layout = LAYOUT_VERTICAL;

	} else if(height) {

		/* Horizontal pager, compute width from height. */
		cp->height = height;
		pp->deskHeight = height;
		pp->deskWidth = (cp->height * rootWidth) / rootHeight;
		cp->width = (pp->deskWidth + 1) * desktopCount;
		pp->layout = LAYOUT_HORIZONTAL;

	} else {
		Assert(0);
	}

	pp->scalex = (double)(pp->deskWidth - 2) / rootWidth;
	pp->scaley = (double)(pp->deskHeight - 2) / rootHeight;

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
		if(pp->layout == LAYOUT_HORIZONTAL) {
			ChangeDesktop(x / (pp->deskWidth + 1));
		} else {
			ChangeDesktop(y / (pp->deskHeight + 1));
		}
		break;
	case Button4:
		PreviousDesktop();
		break;
	case Button5:
		NextDesktop();
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
	int deskWidth, deskHeight;
	unsigned int x;

	if(shouldExit) {
		return;
	}

	for(pp = pagers; pp; pp = pp->next) {

		buffer = pp->cp->pixmap;
		gc = pp->bufferGC;
		width = pp->cp->width;
		height = pp->cp->height;
		deskWidth = pp->deskWidth;
		deskHeight = pp->deskHeight;

		/* Draw the background. */
		JXSetForeground(display, gc, colors[COLOR_PAGER_BG]);
		JXFillRectangle(display, buffer, gc, 0, 0, width, height);

		/* Highlight the current desktop. */
		JXSetForeground(display, gc, colors[COLOR_PAGER_ACTIVE_BG]);
		if(pp->layout == LAYOUT_HORIZONTAL) {
			JXFillRectangle(display, buffer, gc,
				currentDesktop * (deskWidth + 1), 0,
				deskWidth, height);
		} else {
			JXFillRectangle(display, buffer, gc,
				0, currentDesktop * (deskHeight + 1),
				width, deskHeight);
		}

		/* Draw the clients. */
		for(x = LAYER_BOTTOM; x <= LAYER_TOP; x++) {
			for(np = nodeTail[x]; np; np = np->prev) {
				DrawPagerClient(pp, np);
			}
		}

		/* Draw the desktop dividers. */
		JXSetForeground(display, gc, colors[COLOR_PAGER_FG]);
		for(x = 1; x < desktopCount; x++) {
			if(pp->layout == LAYOUT_HORIZONTAL) {
				JXDrawLine(display, buffer, gc,
					(deskWidth + 1) * x - 1, 0,
					(deskWidth + 1) * x - 1, height);
			} else {
				JXDrawLine(display, buffer, gc,
					0, (deskHeight + 1) * x - 1,
					width, (deskHeight + 1) * x - 1);
			}
		}

		/* Tell the tray to redraw. */
		UpdateSpecificTray(pp->cp->tray, pp->cp);

	}

}

/****************************************************************************
 ****************************************************************************/
void DrawPagerClient(const PagerType *pp, const ClientNode *np) {

	int x, y;
	int width, height;
	int deskOffset;

	if(!(np->state.status & STAT_MAPPED)) {
		return;
	}

	if(np->state.status & STAT_STICKY) {
		deskOffset = currentDesktop;
	} else {
		deskOffset = np->state.desktop;
	}
	if(pp->layout == LAYOUT_HORIZONTAL) {
		deskOffset *= pp->deskWidth + 1;
	} else {
		deskOffset *= pp->deskHeight + 1;
	}

	x = (int)((double)np->x * pp->scalex + 1.0);
	y = (int)((double)np->y * pp->scaley + 1.0);
	width = (int)((double)np->width * pp->scalex);
	height = (int)((double)np->height * pp->scaley);

	if(x + width > pp->deskWidth) {
		width = pp->deskWidth - x;
	}
	if(y + height > pp->deskHeight) {
		height = pp->deskHeight - y;
	}
	if(x < 0) {
		width += x;
		x = 0;
	}
	if(y < 0) {
		height += y;
		y = 0;
	}
	if(width <= 0 || height <= 0) {
		return;
	}

	if(pp->layout == LAYOUT_HORIZONTAL) {
		x += deskOffset;
	} else {
		y += deskOffset;
	}

	JXSetForeground(display, pp->bufferGC, colors[COLOR_PAGER_OUTLINE]);
	JXDrawRectangle(display, pp->cp->pixmap, pp->bufferGC,
		x, y, width, height);

	if(width > 1 && height > 1) {
		if((np->state.status & STAT_ACTIVE)
			&& (np->state.desktop == currentDesktop
			|| (np->state.status & STAT_STICKY))) {
			JXSetForeground(display, pp->bufferGC, colors[COLOR_PAGER_ACTIVE_FG]);
		} else {
			JXSetForeground(display, pp->bufferGC, colors[COLOR_PAGER_FG]);
		}
	}

	JXFillRectangle(display, pp->cp->pixmap, pp->bufferGC, x + 1, y + 1,
		width - 1, height - 1);

}

