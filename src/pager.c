/****************************************************************************
 * Functions for displaying the pager.
 * Copyright (C) 2004 Joe Wingbermuehle
 * TODO: Support vertical pagers (arbitrary rectangles?).
 ****************************************************************************/

#include "jwm.h"

typedef struct PagerType {

	void *owner;

	int width, height;
	int deskWidth;
	double scalex, scaley;

	Pixmap buffer;
	GC bufferGC;

	void (*Update)(void *owner);

	struct PagerType *next;

} PagerType;

static PagerType *pagers;

static void Create(void *object, void *owner, void (*Update)(void *owner),
	int width, int height);
static void Destroy(void *object);
static int GetWidth(void *object);
static int GetHeight(void *object);
static void SetSize(void *object, int width, int height);
static Pixmap GetPixmap(void *object);
static void ProcessPagerButtonEvent(void *object, int x, int y, int mask);

static void DrawPagerClient(const PagerType *pp, const ClientNode *np);

/****************************************************************************
 ****************************************************************************/
void InitializePager()
{
	pagers = NULL;
}

/****************************************************************************
 ****************************************************************************/
void StartupPager()
{
}

/****************************************************************************
 ****************************************************************************/
void ShutdownPager()
{

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
void DestroyPager()
{
}

/****************************************************************************
 ****************************************************************************/
TrayComponentType *CreatePager(int width, int height)
{

	TrayComponentType *cp;
	PagerType *pp;

	pp = Allocate(sizeof(PagerType));

	pp->owner = NULL;
	pp->Update = NULL;

	/* At this point we don't know for sure how many desktops are
	 * going to be used. So we just accept the default values.
	 * By the time GetWidth or GetHeight is called, the desktop count will
	 * be known.
	 */
	/* TODO: if the pager is to be vertical, this is backwards. */
	pp->width = 0;
	pp->height = height;

	pp->next = pagers;
	pagers = pp;

	cp = Allocate(sizeof(TrayComponentType));

	cp->object = pp;

	cp->Create = Create;
	cp->Destroy = Destroy;
	cp->GetWidth = GetWidth;
	cp->GetHeight = GetHeight;
	cp->SetSize = SetSize;
	cp->GetWindow = NULL;
	cp->GetPixmap = GetPixmap;

	cp->ProcessButtonEvent = ProcessPagerButtonEvent;

	cp->next = NULL;

	return cp;
}

/****************************************************************************
 ****************************************************************************/
void Create(void *object, void *owner, void (*Update)(void *owner),
	 int width, int height)
{

	PagerType *pp = (PagerType*)object;

	pp->owner = owner;
	pp->Update = Update;

	pp->height = height;
	pp->deskWidth = (pp->height * rootWidth) / rootHeight;
	pp->width = (pp->deskWidth + 1) * desktopCount;
	pp->scalex = (double)(pp->deskWidth - 2) / rootWidth;
	pp->scaley = (double)(pp->height - 2) / rootHeight;

	pp->buffer = JXCreatePixmap(display, rootWindow, pp->width,
		pp->height, rootDepth);
	pp->bufferGC = JXCreateGC(display, pp->buffer, 0, NULL);

}

/****************************************************************************
 ****************************************************************************/
void Destroy(void *object)
{
	/* Handled in ShutdownPager. */
}

/****************************************************************************
 ****************************************************************************/
int GetWidth(void *object)
{

	PagerType *pp = (PagerType*)object;

	Assert(pp);

	if(pp->width == 0) {
		pp->deskWidth = (pp->height * rootWidth) / rootHeight;
		pp->width = (pp->deskWidth + 1) * desktopCount;
	}

	return pp->width;

}

/****************************************************************************
 ****************************************************************************/
int GetHeight(void *object)
{
	Assert(object);
	return ((PagerType*)object)->height;
}

/****************************************************************************
 ****************************************************************************/
void SetSize(void *object, int width, int height)
{

	PagerType *pp = (PagerType*)object;

	Assert(pp);

	if(width) {
		pp->width = width;
		GetHeight(object);
	} else {
		pp->height = height;
		GetWidth(object);
	}

}

/****************************************************************************
 ****************************************************************************/
Pixmap GetPixmap(void *object)
{
	Assert(object);
	return ((PagerType*)object)->buffer;
}

/****************************************************************************
 ****************************************************************************/
void ProcessPagerButtonEvent(void *object, int x, int y, int mask)
{

	PagerType *pp = (PagerType*)object;

	Assert(pp);

	switch(mask)
	{
	case Button1:
	case Button2:
	case Button3:
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
void UpdatePager()
{

	PagerType *pp;
	ClientNode *np;
	Pixmap buffer;
	GC gc;
	int width, height;
	int deskWidth;
	int x;

	for(pp = pagers; pp; pp = pp->next) {

		buffer = pp->buffer;
		gc = pp->bufferGC;
		width = pp->width;
		height = pp->height;
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

		if(pp->Update) {
			(pp->Update)(pp->owner);
		}

	}

}

/****************************************************************************
 ****************************************************************************/
void DrawPagerClient(const PagerType *pp, const ClientNode *np)
{

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
	JXDrawRectangle(display, pp->buffer, pp->bufferGC, x, y, width, height);

	if(width > 1 && height > 1) {
		if((np->statusFlags & STAT_ACTIVE)
			&& (np->desktop == currentDesktop
			|| (np->statusFlags & STAT_STICKY))) {
			JXSetForeground(display, pp->bufferGC, colors[COLOR_PAGER_ACTIVE_FG]);
		} else {
			JXSetForeground(display, pp->bufferGC, colors[COLOR_PAGER_FG]);
		}
	}

	JXFillRectangle(display, pp->buffer, pp->bufferGC, x + 1, y + 1,
		width - 1, height - 1);

}

