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
static void Destroy(TrayComponentType *cp);
static void ProcessClockButtonEvent(TrayComponentType *cp,
	int x, int y, int mask);
static void ProcessClockMotionEvent(TrayComponentType *cp,
	int x, int y, int mask);

static void DrawClock(ClockType *clock, TimeType *now);

/***************************************************************************
 ***************************************************************************/
void InitializeClock() {
	clocks = NULL;
}

/***************************************************************************
 ***************************************************************************/
void StartupClock() {

	ClockType *clock;

	for(clock = clocks; clock; clock = clock->next) {
		clock->cp->width = GetStringWidth(FONT_TASK, clock->format) + 4;
		clock->cp->height = GetStringHeight(FONT_TASK) + 4;
	}

}

/***************************************************************************
 ***************************************************************************/
void ShutdownClock() {

	ClockType *cp;

	while(clocks) {
		cp = clocks->next;

		Release(clocks->format);

		Release(clocks);
		clocks = cp;
	}

}

/***************************************************************************
 ***************************************************************************/
void DestroyClock() {
}

/***************************************************************************
 ***************************************************************************/
TrayComponentType *CreateClock(const char *format, const char *command) {

	TrayComponentType *cp;
	ClockType *clock;

	clock = Allocate(sizeof(ClockType));
	clock->next = clocks;
	clocks = clock;

	clock->mousex = 0;
	clock->mousey = 0;
	clock->mouseTime.seconds = 0;
	clock->mouseTime.ms = 0;

	if(format) {
		clock->format = Allocate(strlen(format) + 1);
		strcpy(clock->format, format);
	} else {
		clock->format = Allocate(strlen(DEFAULT_FORMAT) + 1);
		strcpy(clock->format, DEFAULT_FORMAT);
	}

	if(command) {
		clock->command = Allocate(strlen(command) + 1);
		strcpy(clock->command, command);
	} else {
		clock->command = NULL;
	}

	cp = CreateTrayComponent();
	cp->object = clock;
	clock->cp = cp;

	cp->Create = Create;
	cp->Destroy = Destroy;
	cp->ProcessButtonEvent = ProcessClockButtonEvent;
	cp->ProcessMotionEvent = ProcessClockMotionEvent;

	return cp;

}

/***************************************************************************
 ***************************************************************************/
void Create(TrayComponentType *cp) {

	ClockType *clock;
	TimeType now;

	Assert(cp);

	clock = (ClockType*)cp->object;

	Assert(clock);

	cp->pixmap = JXCreatePixmap(display, rootWindow, cp->width, cp->height,
		rootDepth);
	clock->buffer = cp->pixmap;
	clock->bufferGC = JXCreateGC(display, clock->buffer, 0, NULL);

	GetCurrentTime(&now);
	DrawClock(clock, &now);

}

/***************************************************************************
 ***************************************************************************/
void Destroy(TrayComponentType *cp) {
}

/***************************************************************************
 ***************************************************************************/
static void ProcessClockButtonEvent(TrayComponentType *cp,
	int x, int y, int mask) {

	ClockType *clock;

	Assert(cp);

	clock = (ClockType*)cp->object;

	Assert(clock);

	if(clock->command) {
		RunCommand(clock->command);
	}

}

/***************************************************************************
 ***************************************************************************/
static void ProcessClockMotionEvent(TrayComponentType *cp,
	int x, int y, int mask) {

	ClockType *clock = (ClockType*)cp->object;
	clock->mousex = cp->screenx + x;
	clock->mousey = cp->screeny + y;
	GetCurrentTime(&clock->mouseTime);

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
void DrawClock(ClockType *clock, TimeType *now) {

	TrayComponentType *cp;
	const char *str;
	int x, y;
	time_t t;

	cp = clock->cp;

	JXSetForeground(display, clock->bufferGC, colors[COLOR_TRAY_BG]);
	JXFillRectangle(display, clock->buffer, clock->bufferGC, 0, 0,
		cp->width, cp->height);

	str = GetTimeString(clock->format);

	RenderString(clock->buffer, clock->bufferGC, FONT_TASK,
		COLOR_TASK_FG,
		cp->width / 2 - GetStringWidth(FONT_TASK, str) / 2,
		cp->height / 2 - GetStringHeight(FONT_TASK) / 2,
		cp->width, str);

	UpdateSpecificTray(clock->cp->tray, clock->cp);

	GetMousePosition(&x, &y);

	if(abs(clock->mousex - x) < 2 && abs(clock->mousey - y) < 2) {
		if(GetTimeDifference(now, &clock->mouseTime) >= 2000) {
			time(&t);
			str = asctime(localtime(&t));
			ShowPopup(x, y - GetStringHeight(FONT_POPUP) - 4, str);
		}
	}

}


