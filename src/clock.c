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

	Pixmap buffer;
	GC bufferGC;

	int mousex;
	int mousey;
	TimeType mouseTime;

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

static void DrawClock(ClockType *clk, TimeType *now);

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
		clk->cp->requestedWidth = GetStringWidth(FONT_CLOCK, clk->format) + 4;
		clk->cp->requestedHeight = GetStringHeight(FONT_CLOCK) + 4;
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

	if(format) {
		clk->format = Allocate(strlen(format) + 1);
		strcpy(clk->format, format);
	} else {
		clk->format = Allocate(strlen(DEFAULT_FORMAT) + 1);
		strcpy(clk->format, DEFAULT_FORMAT);
	}

	if(command) {
		clk->command = Allocate(strlen(command) + 1);
		strcpy(clk->command, command);
	} else {
		clk->command = NULL;
	}

	cp = CreateTrayComponent();
	cp->object = clk;
	clk->cp = cp;
	cp->requestedWidth = width;
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
	TimeType now;

	Assert(cp);

	clk = (ClockType*)cp->object;

	Assert(clk);

	cp->pixmap = JXCreatePixmap(display, rootWindow, cp->width, cp->height,
		rootDepth);
	clk->buffer = cp->pixmap;
	clk->bufferGC = JXCreateGC(display, clk->buffer, 0, NULL);

	GetCurrentTime(&now);
	DrawClock(clk, &now);

}

/***************************************************************************
 ***************************************************************************/
void Resize(TrayComponentType *cp) {

	ClockType *clk;
	TimeType now;

	Assert(cp);

	clk = (ClockType*)cp->object;

	Assert(clk);

	if(clk->buffer != None) {
		JXFreeGC(display, clk->bufferGC);
		JXFreePixmap(display, clk->buffer);
	}

	cp->pixmap = JXCreatePixmap(display, rootWindow, cp->width, cp->height,
		rootDepth);
	clk->buffer = cp->pixmap;
	clk->bufferGC = JXCreateGC(display, clk->buffer, 0, NULL);

	GetCurrentTime(&now);
	DrawClock(clk, &now);

}

/***************************************************************************
 ***************************************************************************/
void Destroy(TrayComponentType *cp) {

	ClockType *clk;

	Assert(cp);

	clk = (ClockType*)cp->object;

	Assert(clk);

	if(clk->buffer != None) {
		JXFreeGC(display, clk->bufferGC);
		JXFreePixmap(display, clk->buffer);
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
void UpdateClocks() {

	ClockType *cp;
	TimeType now;

	GetCurrentTime(&now);
	if(GetTimeDifference(&lastUpdate, &now) < 900) {
		return;
	}
	lastUpdate = now;

	for(cp = clocks; cp; cp = cp->next) {
		DrawClock(cp, &now);
	}

}

/***************************************************************************
 ***************************************************************************/
void DrawClock(ClockType *clk, TimeType *now) {

	TrayComponentType *cp;
	const char *shortTime;
	char *longTime;
	int x, y;
	time_t t;

	cp = clk->cp;

	JXSetForeground(display, clk->bufferGC, colors[COLOR_CLOCK_BG]);
	JXFillRectangle(display, clk->buffer, clk->bufferGC, 0, 0,
		cp->width, cp->height);

	shortTime = GetTimeString(clk->format);

	RenderString(clk->buffer, clk->bufferGC, FONT_CLOCK,
		COLOR_CLOCK_FG,
		cp->width / 2 - GetStringWidth(FONT_CLOCK, shortTime) / 2,
		cp->height / 2 - GetStringHeight(FONT_CLOCK) / 2,
		cp->width, shortTime);

	UpdateSpecificTray(clk->cp->tray, clk->cp);

	GetMousePosition(&x, &y);

	if(abs(clk->mousex - x) < 2 && abs(clk->mousey - y) < 2) {
		if(GetTimeDifference(now, &clk->mouseTime) >= 2000) {
			time(&t);
			longTime = asctime(localtime(&t));
			Trim(longTime);
			ShowPopup(x, y - GetStringHeight(FONT_POPUP) - 4, longTime);
		}
	}

}


