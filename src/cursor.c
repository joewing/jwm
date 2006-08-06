/****************************************************************************
 * Functions to handle the mouse cursor.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"
#include "cursor.h"
#include "main.h"
#include "error.h"

static Cursor defaultCursor;
static Cursor moveCursor;
static Cursor northCursor;
static Cursor southCursor;
static Cursor eastCursor;
static Cursor westCursor;
static Cursor northEastCursor;
static Cursor northWestCursor;
static Cursor southEastCursor;
static Cursor southWestCursor;

static Cursor GetResizeCursor(BorderActionType action);
static Cursor CreateCursor(unsigned int shape);

static int mousex;
static int mousey;

/****************************************************************************
 ****************************************************************************/
void InitializeCursors() {
}

/****************************************************************************
 ****************************************************************************/
void StartupCursors() {

	Window win1, win2;
	int winx, winy;
	unsigned int mask;

	defaultCursor = CreateCursor(XC_left_ptr);
	moveCursor = CreateCursor(XC_fleur);
	northCursor = CreateCursor(XC_top_side);
	southCursor = CreateCursor(XC_bottom_side);
	eastCursor = CreateCursor(XC_right_side);
	westCursor = CreateCursor(XC_left_side);
	northEastCursor = CreateCursor(XC_ur_angle);
	northWestCursor = CreateCursor(XC_ul_angle);
	southEastCursor = CreateCursor(XC_lr_angle);
	southWestCursor = CreateCursor(XC_ll_angle);

	JXQueryPointer(display, rootWindow, &win1, &win2,
		&mousex, &mousey, &winx, &winy, &mask);

}

/****************************************************************************
 ****************************************************************************/
Cursor CreateCursor(unsigned int shape) {
	return JXCreateFontCursor(display, shape);
}

/****************************************************************************
 ****************************************************************************/
void ShutdownCursors() {

	JXFreeCursor(display, defaultCursor);
	JXFreeCursor(display, moveCursor);
	JXFreeCursor(display, northCursor);
	JXFreeCursor(display, southCursor);
	JXFreeCursor(display, eastCursor);
	JXFreeCursor(display, westCursor);
	JXFreeCursor(display, northEastCursor);
	JXFreeCursor(display, northWestCursor);
	JXFreeCursor(display, southEastCursor);
	JXFreeCursor(display, southWestCursor);

}

/****************************************************************************
 ****************************************************************************/
void DestroyCursors() {
}

/****************************************************************************
 ****************************************************************************/
Cursor GetFrameCursor(BorderActionType action) {
	switch(action & 0x0F) {
	case BA_RESIZE:
		return GetResizeCursor(action);
	case BA_CLOSE:
		break;
	case BA_MAXIMIZE:
		break;
	case BA_MINIMIZE:
		break;
	case BA_MOVE:
		break;
	default:
		break;
	}
	return defaultCursor;
}

/****************************************************************************
 ****************************************************************************/
Cursor GetResizeCursor(BorderActionType action) {
	if(action & BA_RESIZE_N) {
		if(action & BA_RESIZE_E) {
			return northEastCursor;
		} else if(action & BA_RESIZE_W) {
			return northWestCursor;
		} else {
			return northCursor;
		}
	} else if(action & BA_RESIZE_S) {
		if(action & BA_RESIZE_E) {
			return southEastCursor;
		} else if(action & BA_RESIZE_W) {
			return southWestCursor;
		} else {
			return southCursor;
		}
	} else {
		if(action & BA_RESIZE_E) {
			return eastCursor;
		} else {
			return westCursor;
		}
	}
}

/****************************************************************************
 ****************************************************************************/
int GrabMouseForResize(BorderActionType action) {
	Cursor cur;
	int result;

	cur = GetFrameCursor(action);

	result = JXGrabPointer(display, rootWindow, False, ButtonPressMask
		| ButtonReleaseMask | PointerMotionMask, GrabModeAsync,
		GrabModeAsync, None, cur, CurrentTime);

	if(result == GrabSuccess) {
		return 1;
	} else {
		return 0;
	}

}

/****************************************************************************
 ****************************************************************************/
int GrabMouseForMove(Window w) {

	int result;

	result = JXGrabPointer(display, rootWindow, False,
		ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
		GrabModeAsync, GrabModeAsync, None, defaultCursor, CurrentTime);


	if(result == GrabSuccess) {

		JXDefineCursor(display, w, moveCursor);
		return 1;

	} else {

		return 0;

	}

}

/****************************************************************************
 ****************************************************************************/
int GrabMouseForMenu() {

	int result;

	result = JXGrabPointer(display, rootWindow, False,
		ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
		GrabModeAsync, GrabModeAsync, None, defaultCursor, CurrentTime);

	if(result == GrabSuccess) {
		return 1;
	} else {
		return 0;
	}

}

/****************************************************************************
 ****************************************************************************/
void SetDefaultCursor(Window w) {

	JXDefineCursor(display, w, defaultCursor);

}

/****************************************************************************
 ****************************************************************************/
void MoveMouse(Window win, int x, int y) {

	Window win1, win2;
	int winx, winy;
	unsigned int mask;

	JXWarpPointer(display, None, win, 0, 0, 0, 0, x, y);

	JXQueryPointer(display, rootWindow, &win1, &win2,
		&mousex, &mousey, &winx, &winy, &mask);

}

/****************************************************************************
 ****************************************************************************/
void SetMousePosition(int x, int y) {

	mousex = x;
	mousey = y;

}

/****************************************************************************
 ****************************************************************************/
void GetMousePosition(int *x, int *y) {

	*x = mousex;
	*y = mousey;

}

/****************************************************************************
 ****************************************************************************/
unsigned int GetMouseMask() {

	Window win1, win2;
	int winx, winy;
	unsigned int mask;

	JXQueryPointer(display, rootWindow, &win1, &win2,
		&mousex, &mousey, &winx, &winy, &mask);

	return mask;

}

/****************************************************************************
 ****************************************************************************/
void SetDoubleClickSpeed(const char *str) {
	int speed;

	speed = atoi(str);
	if(speed < MIN_DOUBLE_CLICK_SPEED || speed > MAX_DOUBLE_CLICK_SPEED) {
		Warning("invalid DoubleClickSpeed: %d", speed);
		doubleClickSpeed = DEFAULT_DOUBLE_CLICK_SPEED;
	} else {
		doubleClickSpeed = speed;
	}
}

/****************************************************************************
 ****************************************************************************/
void SetDoubleClickDelta(const char *str) {
	int delta;

	delta = atoi(str);
	if(delta < MIN_DOUBLE_CLICK_DELTA || delta > MAX_DOUBLE_CLICK_DELTA) {
		Warning("invalid DoubleClickDelta: %d", delta);
		doubleClickDelta = DEFAULT_DOUBLE_CLICK_DELTA;
	} else {
		doubleClickDelta = delta;
	}

}

