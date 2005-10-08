/****************************************************************************
 * Functions to handle resizing client windows.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"

static ResizeModeType resizeMode = RESIZE_OPAQUE;

static int shouldStopResize;

static void StopResize(ClientNode *np, int north);
static void ResizeController(int wasDestroyed);
static void FixWidth(ClientNode *np);
static void FixHeight(ClientNode *np);

/****************************************************************************
 ****************************************************************************/
void SetResizeMode(ResizeModeType mode) {
	resizeMode = mode;
}

/****************************************************************************
 ****************************************************************************/
void ResizeController(int wasDestroyed) {
	if(resizeMode == RESIZE_OUTLINE) {
		ClearOutline();
	}
	JXUngrabPointer(display, CurrentTime);
	JXUngrabKeyboard(display, CurrentTime);
	DestroyResizeWindow();
	shouldStopResize = 1;
}

/****************************************************************************
 ****************************************************************************/
void ResizeClient(ClientNode *np, BorderActionType action,
	int startx, int starty) {

	XEvent event;
	int oldx, oldy;
	int oldw, oldh;
	int gwidth, gheight;
	int lastgwidth, lastgheight;
	int delta;
	int north;

	Assert(np);

	if(!(np->borderFlags & BORDER_OUTLINE)
		|| !(np->borderFlags & BORDER_RESIZE)
		|| (np->statusFlags & STAT_MAXIMIZED)) {
		return;
	}

	if(!GrabMouseForResize(action)) {
		Debug("ResizeClient: could not grab mouse");
		return;
	}

	if(np->statusFlags & STAT_SHADED) {
		action &= ~(BA_RESIZE_N | BA_RESIZE_S);
	}

	np->controller = ResizeController;
	shouldStopResize = 0;

	oldx = np->x;
	oldy = np->y;
	oldw = np->width;
	oldh = np->height;

	gwidth = (np->width - np->baseWidth) / np->xinc;
	gheight = (np->height - np->baseHeight) / np->yinc;

	if(np->borderFlags & BORDER_TITLE) {
		north = titleSize;
	} else {
		north = borderWidth;
	}
	startx += np->x - borderWidth;
	starty += np->y - north;

	CreateResizeWindow();
	UpdateResizeWindow(gwidth, gheight);

	if(!(GetMouseMask() & Button1Mask)) {
		StopResize(np, north);
		return;
	}

	for(;;) {

		WaitForEvent(&event);

		if(shouldStopResize) {
			np->controller = NULL;
			return;
		}

		switch(event.type) {
		case ButtonRelease:
			if(event.xbutton.button == Button1) {
				StopResize(np, north);
				return;
			}
			break;
		case MotionNotify:

			while(JXCheckTypedEvent(display, MotionNotify, &event));

			if(action & BA_RESIZE_N) {
				delta = (event.xmotion.y - starty) / np->yinc;
				delta *= np->yinc;
				if(oldh - delta >= np->minHeight
					&& (oldh - delta <= np->maxHeight || delta > 0)) {
					np->height = oldh - delta;
					np->y = oldy + delta;
				}
				if(!(action & (BA_RESIZE_E | BA_RESIZE_W))) {
					FixWidth(np);
				}
			}
			if(action & BA_RESIZE_S) {
				delta = (event.xmotion.y - starty) / np->yinc;
				delta *= np->yinc;
				np->height = oldh + delta;
				np->height = Max(np->height, np->minHeight);
				np->height = Min(np->height, np->maxHeight);
				if(!(action & (BA_RESIZE_E | BA_RESIZE_W))) {
					FixWidth(np);
				}
			}
			if(action & BA_RESIZE_E) {
				delta = (event.xmotion.x - startx) / np->xinc;
				delta *= np->xinc;
				np->width = oldw + delta;
				np->width = Max(np->width, np->minWidth);
				np->width = Min(np->width, np->maxWidth);
				if(!(action & (BA_RESIZE_N | BA_RESIZE_S))) {
					FixHeight(np);
				}
			}
			if(action & BA_RESIZE_W) {
				delta = (event.xmotion.x - startx) / np->xinc;
				delta *= np->xinc;
				if(oldw - delta >= np->minWidth
					&& (oldw - delta <= np->maxWidth || delta > 0)) {
					np->width = oldw - delta;
					np->x = oldx + delta;
				}
				if(!(action & (BA_RESIZE_N | BA_RESIZE_S))) {
					FixHeight(np);
				}
			}

			if(np->sizeFlags & PAspect) {
				if((action & (BA_RESIZE_N | BA_RESIZE_S)) &&
					(action & (BA_RESIZE_E | BA_RESIZE_W))) {

					if((float)np->width / np->height
						< (float)np->aspect.minx / np->aspect.miny) {
						np->width = np->height * np->aspect.minx
							/ np->aspect.miny;
					}
					if((float)np->width / np->height
						> (float)np->aspect.maxx / np->aspect.maxy) {
						np->height = np->width * np->aspect.maxy
							/ np->aspect.maxx;
					}

				}
			}

			lastgwidth = gwidth;
			lastgheight = gheight;

			gwidth = (np->width - np->baseWidth) / np->xinc;
			gheight = (np->height - np->baseHeight) / np->yinc;

			if(lastgheight != gheight || lastgwidth != gwidth) {

				UpdateResizeWindow(gwidth, gheight);

				if(resizeMode == RESIZE_OUTLINE) {
					ClearOutline();
					if(np->statusFlags & STAT_SHADED) {
						DrawOutline(np->x - borderWidth, np->y - north,
							np->width + borderWidth * 2,
							north + borderWidth);
					} else {
						DrawOutline(np->x - borderWidth, np->y - north,
							np->width + borderWidth * 2,
							np->height + north + borderWidth);
					}
				} else {
					if(np->statusFlags & STAT_SHADED) {
						JXMoveResizeWindow(display, np->parent,
							np->x - borderWidth, np->y - north,
							np->width + borderWidth * 2, north + borderWidth);
					} else {
						JXMoveResizeWindow(display, np->parent,
							np->x - borderWidth, np->y - north,
							np->width + borderWidth + borderWidth,
							np->height + north + borderWidth);
					}
					JXMoveResizeWindow(display, np->window, borderWidth,
						north, np->width, np->height);
					SendConfigureEvent(np);
				}

				UpdatePager();

			}

			break;
		default:
			break;
		}
	}

}

/****************************************************************************
 ****************************************************************************/
void ResizeClientKeyboard(ClientNode *np) {
	XEvent event;
	int gwidth, gheight;
	int lastgwidth, lastgheight;
	int north, west;
	int deltax, deltay;

	Assert(np);

	if(!(np->borderFlags & BORDER_RESIZE)
		|| (np->statusFlags & STAT_MAXIMIZED)) {
		return;
	}

	if(JXGrabKeyboard(display, np->window, True, GrabModeAsync,
		GrabModeAsync, CurrentTime) != GrabSuccess) {
		Debug("ResizeClientKeyboard: could not grab keyboard");
		return;
	}
	GrabMouseForResize(BA_RESIZE_S | BA_RESIZE_E | BA_RESIZE);

	np->controller = ResizeController;
	shouldStopResize = 0;

	gwidth = (np->width - np->baseWidth) / np->xinc;
	gheight = (np->height - np->baseHeight) / np->yinc;

	if(np->borderFlags & BORDER_OUTLINE) {
		west = borderWidth;
	} else {
		west = 0;
	}
	if(np->borderFlags & BORDER_TITLE) {
		north = titleSize;
	} else {
		north = west;
	}

	CreateResizeWindow();
	UpdateResizeWindow(gwidth, gheight);

	JXWarpPointer(display, None, rootWindow, 0, 0, 0, 0,
		np->x + np->width, np->y + np->height);
	JXCheckTypedEvent(display, MotionNotify, &event);

	for(;;) {

		WaitForEvent(&event);

		if(shouldStopResize) {
			np->controller = NULL;
			return;
		}

		deltax = 0;
		deltay = 0;

		if(event.type == KeyPress) {

			while(JXCheckTypedEvent(display, KeyPress, &event));

			switch(GetKey(&event.xkey) & 0xFF) {
			case KEY_UP:
				deltay = Min(-np->yinc, -10);
				break;
			case KEY_DOWN:
				deltay = Max(np->yinc, 10);
				break;
			case KEY_RIGHT:
				deltax = Max(np->xinc, 10);
				break;
			case KEY_LEFT:
				deltax = Min(-np->xinc, -10);
				break;
			default:
				StopResize(np, north);
				return;
			}

		} else if(event.type == MotionNotify) {

			while(JXCheckTypedEvent(display, MotionNotify, &event));

			deltax = event.xmotion.x - (np->x + np->width);
			deltay = event.xmotion.y - (np->y + np->height);

		} else if(event.type == ButtonRelease) {

			StopResize(np, north);
			return;

		}

		if(abs(deltax) < np->xinc && abs(deltay) < np->yinc) {
			continue;
		}

		deltay -= deltay % np->yinc;
		np->height += deltay;
		np->height = Max(np->height, np->minHeight);
		np->height = Min(np->height, np->maxHeight);
		deltax -= deltax % np->xinc;
		np->width += deltax;
		np->width = Max(np->width, np->minWidth);
		np->width = Min(np->width, np->maxWidth);

		if(np->sizeFlags & PAspect) {
			if((float)np->width / np->height
				< (float)np->aspect.minx / np->aspect.miny) {
				np->width = np->height * np->aspect.minx / np->aspect.miny;
			}
			if((float)np->width / np->height
				> (float)np->aspect.maxx / np->aspect.maxy) {
				np->height = np->width * np->aspect.maxy / np->aspect.maxx;
			}
		}

		lastgwidth = gwidth;
		lastgheight = gheight;
		gwidth = (np->width - np->baseWidth) / np->xinc;
		gheight = (np->height - np->baseHeight) / np->yinc;

		if(lastgwidth != gwidth || lastgheight != gheight) {

			UpdateResizeWindow(gwidth, gheight);

			if(resizeMode == RESIZE_OUTLINE) {
				ClearOutline();
				if(np->statusFlags & STAT_SHADED) {
					DrawOutline(np->x - borderWidth, np->y - north,
						np->width + borderWidth * 2,
						north + borderWidth);
				} else {
					DrawOutline(np->x - borderWidth, np->y - north,
						np->width + borderWidth * 2,
						np->height + north + borderWidth);
				}
			} else {
				if(np->statusFlags & STAT_SHADED) {
					JXResizeWindow(display, np->parent,
						np->width + west * 2, north + west);
				} else {
					JXResizeWindow(display, np->parent,
					np->width + west * 2, np->height + north + west);
				}
				JXResizeWindow(display, np->window, np->width, np->height);
				SendConfigureEvent(np);
			}

			UpdatePager();

		}

	}

}

/****************************************************************************
 ****************************************************************************/
void StopResize(ClientNode *np, int north) {

	np->controller = NULL;

	if(resizeMode == RESIZE_OUTLINE) {
		ClearOutline();
	}

	JXUngrabPointer(display, CurrentTime);
	JXUngrabKeyboard(display, CurrentTime);

	DestroyResizeWindow();

	if(np->statusFlags & STAT_SHADED) {
		JXMoveResizeWindow(display, np->parent,
			np->x - borderWidth, np->y - north,
			np->width + borderWidth * 2, north + borderWidth);
	} else {
		JXMoveResizeWindow(display, np->parent,
			np->x - borderWidth, np->y - north,
			np->width + borderWidth + borderWidth,
			np->height + north + borderWidth);
	}
	JXMoveResizeWindow(display, np->window, borderWidth,
		north, np->width, np->height);
	SendConfigureEvent(np);

}

/****************************************************************************
 ****************************************************************************/
void FixWidth(ClientNode *np) {

	Assert(np);

	if((np->sizeFlags & PAspect) && np->height > 0) {
		if((float)np->width / np->height
			< (float)np->aspect.minx / np->aspect.miny) {
			np->width = np->height * np->aspect.minx / np->aspect.miny;
		}
		if((float)np->width / np->height
			> (float)np->aspect.maxx / np->aspect.maxy) {
			np->width = np->height * np->aspect.maxx / np->aspect.maxy;
		}
	}
}

/****************************************************************************
 ****************************************************************************/
void FixHeight(ClientNode *np) {

	Assert(np);

	if((np->sizeFlags & PAspect) && np->height > 0) {
		if((float)np->width / np->height
			< (float)np->aspect.minx / np->aspect.miny) {
			np->height = np->width * np->aspect.miny / np->aspect.minx;
		}
		if((float)np->width / np->height
			> (float)np->aspect.maxx / np->aspect.maxy) {
			np->height = np->width * np->aspect.maxy / np->aspect.maxx;
		}
	}
}

