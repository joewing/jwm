/****************************************************************************
 * Clock tray component.
 * Copyright (C) 2005 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"
#include "clock.h"
#include "tray.h"
#include "color.h"
#include "font.h"
#include "timing.h"
#include "main.h"
#include "root.h"
#include "cursor.h"
#include "popup.h"
#include "misc.h"

typedef struct ClockType {

	TrayComponentType *cp;

	char *format;
	char *command;
	char shortTime[80];

	GC bufferGC;

	int mousex;
	int mousey;
	TimeType mouseTime;

	int userWidth;

	struct ClockType *next;

} ClockType;

static const char *DEFAULT_FORMAT = "%I:%M %p";

static ClockType *clocks;
static TimeType lastUpdate = ZERO_TIME;

static void Create(TrayComponentType *cp);
static void Resize(TrayComponentType *cp);
static void Destroy(TrayComponentType *cp);
static void ProcessClockButtonEvent(TrayComponentType *cp,
	int x, int y, int mask);
static void ProcessClockMotionEvent(TrayComponentType *cp,
	int x, int y, int mask);

static void DrawClock(ClockType *clk, const TimeType *now, int x, int y);

/***************************************************************************
 ***************************************************************************/
void InitializeClock() {
	clocks = NULL;
}

/***************************************************************************
 ***************************************************************************/
void StartupClock() {

	ClockType *clk;

	for(clk = clocks; clk; clk = clk->next) {
		if(clk->cp->requestedWidth == 0) {
			clk->cp->requestedWidth = GetStringWidth(FONT_CLOCK, clk->format) + 4;
		}
		if(clk->cp->requestedHeight == 0) {
			clk->cp->requestedHeight = GetStringHeight(FONT_CLOCK) + 4;
		}
	}

}

/***************************************************************************
 ***************************************************************************/
void ShutdownClock() {
}

/***************************************************************************
 ***************************************************************************/
void DestroyClock() {

	ClockType *cp;

	while(clocks) {
		cp = clocks->next;

		if(clocks->format) {
			Release(clocks->format);
		}
		if(clocks->command) {
			Release(clocks->command);
		}

		Release(clocks);
		clocks = cp;
	}

}

/***************************************************************************
 ***************************************************************************/
TrayComponentType *CreateClock(const char *format, const char *command,
	int width, int height) {

	TrayComponentType *cp;
	ClockType *clk;

	clk = Allocate(sizeof(ClockType));
	clk->next = clocks;
	clocks = clk;

	clk->mousex = 0;
	clk->mousey = 0;
	clk->mouseTime.seconds = 0;
	clk->mouseTime.ms = 0;
	clk->userWidth = 0;

	if(!format) {
		format = DEFAULT_FORMAT;
	}
	clk->format = CopyString(format);

	clk->command = CopyString(command);

	clk->shortTime[0] = 0;

	cp = CreateTrayComponent();
	cp->object = clk;
	clk->cp = cp;
	if(width > 0) {
		cp->requestedWidth = width;
		clk->userWidth = 1;
	} else {
		cp->requestedWidth = 0;
		clk->userWidth = 0;
	}
	cp->requestedHeight = height;

	cp->Create = Create;
	cp->Resize = Resize;
	cp->Destroy = Destroy;
	cp->ProcessButtonEvent = ProcessClockButtonEvent;
	cp->ProcessMotionEvent = ProcessClockMotionEvent;

	return cp;

}

/***************************************************************************
 ***************************************************************************/
void Create(TrayComponentType *cp) {

	ClockType *clk;

	Assert(cp);

	clk = (ClockType*)cp->object;

	Assert(clk);

	cp->pixmap = JXCreatePixmap(display, rootWindow, cp->width, cp->height,
		rootDepth);
	clk->bufferGC = JXCreateGC(display, cp->pixmap, 0, NULL);

	JXSetForeground(display, clk->bufferGC, colors[COLOR_CLOCK_BG]);
	JXFillRectangle(display, cp->pixmap, clk->bufferGC, 0, 0,
		cp->width, cp->height);

}

/***************************************************************************
 ***************************************************************************/
void Resize(TrayComponentType *cp) {

	ClockType *clk;
	TimeType now;
	int x, y;

	Assert(cp);

	clk = (ClockType*)cp->object;

	Assert(clk);

	if(cp->pixmap != None) {
		JXFreeGC(display, clk->bufferGC);
		JXFreePixmap(display, cp->pixmap);
	}

	cp->pixmap = JXCreatePixmap(display, rootWindow, cp->width, cp->height,
		rootDepth);
	clk->bufferGC = JXCreateGC(display, cp->pixmap, 0, NULL);

	clk->shortTime[0] = 0;

	GetCurrentTime(&now);
	GetMousePosition(&x, &y);
	DrawClock(clk, &now, x, y);

}

/***************************************************************************
 ***************************************************************************/
void Destroy(TrayComponentType *cp) {

	ClockType *clk;

	Assert(cp);

	clk = (ClockType*)cp->object;

	Assert(clk);

	if(cp->pixmap != None) {
		JXFreeGC(display, clk->bufferGC);
		JXFreePixmap(display, cp->pixmap);
	}
}

/***************************************************************************
 ***************************************************************************/
static void ProcessClockButtonEvent(TrayComponentType *cp,
	int x, int y, int mask) {

	ClockType *clk;

	Assert(cp);

	clk = (ClockType*)cp->object;

	Assert(clk);

	if(clk->command) {
		RunCommand(clk->command);
	}

}

/***************************************************************************
 ***************************************************************************/
static void ProcessClockMotionEvent(TrayComponentType *cp,
	int x, int y, int mask) {

	ClockType *clk = (ClockType*)cp->object;
	clk->mousex = cp->screenx + x;
	clk->mousey = cp->screeny + y;
	GetCurrentTime(&clk->mouseTime);

}

/***************************************************************************
 ***************************************************************************/
void SignalClock(const TimeType *now, int x, int y) {

	ClockType *cp;
	int shouldDraw;
	char *longTime;
	time_t t;

	if(GetTimeDifference(&lastUpdate, now) > 900) {
		shouldDraw = 1;
		lastUpdate = *now;
	} else {
		shouldDraw = 0;
	}

	for(cp = clocks; cp; cp = cp->next) {

		if(shouldDraw) {
			DrawClock(cp, now, x, y);
		}

		if(abs(cp->mousex - x) < POPUP_DELTA
			&& abs(cp->mousey - y) < POPUP_DELTA) {
			if(GetTimeDifference(now, &cp->mouseTime) >= popupDelay) {
				time(&t);
				longTime = asctime(localtime(&t));
				Trim(longTime);
				ShowPopup(x, y, longTime);
			}
		}

	}

}

/***************************************************************************
 ***************************************************************************/
void DrawClock(ClockType *clk, const TimeType *now, int x, int y) {

	TrayComponentType *cp;
	const char *shortTime;
	int width;
	int rwidth;

	shortTime = GetTimeString(clk->format);
	if(!strcmp(clk->shortTime, shortTime)) {
		return;
	}
	strcpy(clk->shortTime, shortTime);

	cp = clk->cp;

	JXSetForeground(display, clk->bufferGC, colors[COLOR_CLOCK_BG]);
	JXFillRectangle(display, cp->pixmap, clk->bufferGC, 0, 0,
		cp->width, cp->height);

	width = GetStringWidth(FONT_CLOCK, shortTime);
	rwidth = width + 4;
	if(rwidth == clk->cp->requestedWidth || clk->userWidth) {

		RenderString(cp->pixmap, clk->bufferGC, FONT_CLOCK,
			COLOR_CLOCK_FG,
			cp->width / 2 - width / 2,
			cp->height / 2 - GetStringHeight(FONT_CLOCK) / 2,
			cp->width, NULL, shortTime);

		UpdateSpecificTray(clk->cp->tray, clk->cp);

	} else {

		clk->cp->requestedWidth = rwidth;
		ResizeTray(clk->cp->tray);

	}

}


