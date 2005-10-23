/***************************************************************************
 * Functions to handle the tray.
 * Copyright (C) 2004 Joe Wingbermuehle
 ***************************************************************************/

#include "jwm.h"

static TrayType *trays;
static Window supportingWindow;

static void HandleTrayExpose(TrayType *tp, const XExposeEvent *event);
static void HandleTrayEnterNotify(TrayType *tp, const XCrossingEvent *event);
static void HandleTrayLeaveNotify(TrayType *tp, const XCrossingEvent *event);

static void HandleTrayButtonPress(TrayType *tp, const XButtonEvent *event);
static void HandleTrayMotionNotify(TrayType *tp, const XMotionEvent *event);

static void ComputeTraySize(TrayType *tp);
static int ComputeMaxWidth(TrayType *tp);
static int ComputeMaxHeight(TrayType *tp);
static int CheckHorizontalFill(TrayType *tp);
static int CheckVerticalFill(TrayType *tp);

/***************************************************************************
 ***************************************************************************/
void InitializeTray()
{
	trays = NULL;
	supportingWindow = None;
}

/***************************************************************************
 ***************************************************************************/
void StartupTray()
{

	XSetWindowAttributes attr;
	long attrMask;
	TrayType *tp;
	TrayComponentType *cp;
	int temp;
	int variableCount;
	int variableSize;
	int variableRemainder;
	int width, height;
	int xoffset, yoffset;

	for(tp = trays; tp; tp = tp->next) {

		ComputeTraySize(tp);

		/* Get the remaining size after setting fixed size components. */
		/* Also, keep track of the number of variable size components. */
		width = tp->width - 2 * tp->border;
		height = tp->height - 2 * tp->border;
		variableCount = 0;
		for(cp = tp->components; cp; cp = cp->next) {
			if(tp->layout == LAYOUT_HORIZONTAL) {
				temp = cp->width;
				if(temp > 0) {
					width -= temp;
				} else {
					++variableCount;
				}
			} else {
				temp = cp->height;
				if(temp > 0) {
					height -= temp;
				} else {
					++variableCount;
				}
			}
		}

		/* Distribute excess size among variable size components.
		 * If there are no variable size components, shrink the tray.
		 * If we are out of room, just give them a size of one.
		 */
		variableSize = 1;
		variableRemainder = 0;
		if(tp->layout == LAYOUT_HORIZONTAL) {
			if(variableCount) {
				if(width >= variableCount) {
					variableSize = width / variableCount;
					variableRemainder = width % variableCount;
				}
			} else if(width > 0) {
				tp->width = tp->width - width;
			}
		} else {
			if(variableCount) {
				if(height >= variableCount) {
					variableSize = height / variableCount;
					variableRemainder = height % variableCount;
				}
			} else if(height > 0) {
				tp->height = tp->height - height;
			}
		}

		/* Create the tray window. */
		/* The window is created larger for a border. */
		attrMask = CWOverrideRedirect;
		attr.override_redirect = True;

		attrMask |= CWEventMask;
		attr.event_mask
			= ButtonPressMask
			| ExposureMask
			| KeyPressMask
			| LeaveWindowMask
			| EnterWindowMask
			| PointerMotionMask;

		attrMask |= CWBackPixel;
		attr.background_pixel = colors[COLOR_TRAY_BG];

		tp->window = JXCreateWindow(display, rootWindow,
			tp->x, tp->y, tp->width, tp->height,
			0, rootDepth, InputOutput, rootVisual, attrMask, &attr);

		SetDefaultCursor(tp->window);

		tp->gc = JXCreateGC(display, tp->window, 0, NULL);

		/* Create and layout items on the tray. */
		xoffset = tp->border;
		yoffset = tp->border;
		for(cp = tp->components; cp; cp = cp->next) {

			if(cp->Create) {
				if(tp->layout == LAYOUT_HORIZONTAL) {
					height = tp->height - 2 * tp->border;
					width = cp->width;
					if(width == 0) {
						width = variableSize;
						if(variableRemainder) {
							++width;
							--variableRemainder;
						}
					}
				} else {
					width = tp->width - 2 * tp->border;
					height = cp->height;
					if(height == 0) {
						height = variableSize;
						if(variableRemainder) {
							++height;
							--variableRemainder;
						}
					}
				}
				cp->width = width;
				cp->height = height;
				(cp->Create)(cp);
			}

			cp->x = xoffset;
			cp->y = yoffset;
			cp->screenx = tp->x + xoffset;
			cp->screeny = tp->y + yoffset;

			if(cp->window != None) {
				JXReparentWindow(display, cp->window, tp->window,
					xoffset, yoffset);
			}

			if(tp->layout == LAYOUT_HORIZONTAL) {
				xoffset += cp->width;
			} else {
				yoffset += cp->height;
			}
		}

		/* Show the tray. */
		JXMapWindow(display, tp->window);

		if(tp->autoHide) {
			HideTray(tp);
		}

	}

	UpdatePager();
	UpdateTaskBar();
	DrawTray();

}

/***************************************************************************
 ***************************************************************************/
void ShutdownTray()
{

	TrayType *tp;
	TrayComponentType *cp;

	while(trays) {
		tp = trays->next;

		while(trays->components) {
			cp = trays->components->next;
			if(trays->components->Destroy) {
				(trays->components->Destroy)(trays->components);
			}
			Release(trays->components);
			trays->components = cp;
		}

		JXFreeGC(display, trays->gc);
		JXDestroyWindow(display, trays->window);

		Release(trays);

		trays = tp;
	}

	if(supportingWindow != None) {
		XDestroyWindow(display, supportingWindow);
		supportingWindow = None;
	}

}


/***************************************************************************
 ***************************************************************************/
void DestroyTray()
{
}

/***************************************************************************
 ***************************************************************************/
TrayType *CreateTray()
{

	TrayType *tp;

	tp = Allocate(sizeof(TrayType));

	tp->x = 0;
	tp->y = -1;
	tp->width = 0;
	tp->height = 0;
	tp->border = 1;
	tp->layer = DEFAULT_TRAY_LAYER;
	tp->layout = LAYOUT_HORIZONTAL;

	tp->autoHide = 0;
	tp->hidden = 0;

	tp->window = None;
	tp->gc = None;

	tp->components = NULL;
	tp->componentsTail = NULL;

	tp->next = trays;
	trays = tp;

	return tp;

}

/***************************************************************************
 ***************************************************************************/
TrayComponentType *CreateTrayComponent() {

	TrayComponentType *cp;

	cp = Allocate(sizeof(TrayComponentType));

	cp->tray = NULL;
	cp->object = NULL;

	cp->x = 0;
	cp->y = 0;
	cp->width = 0;
	cp->height = 0;

	cp->window = None;
	cp->pixmap = None;

	cp->Create = NULL;
	cp->Destroy = NULL;

	cp->SetSize = NULL;

	cp->ProcessButtonEvent = NULL;
	cp->ProcessMotionEvent = NULL;

	cp->next = NULL;

	return cp;

}

/***************************************************************************
 ***************************************************************************/
void AddTrayComponent(TrayType *tp, TrayComponentType *cp)
{

	Assert(tp);
	Assert(cp);

	cp->tray = tp;

	if(tp->componentsTail) {
		tp->componentsTail->next = cp;
	} else {
		tp->components = cp;
	}
	tp->componentsTail = cp;
	cp->next = NULL;

}

/***************************************************************************
 * Compute the max component width.
 ***************************************************************************/
int ComputeMaxWidth(TrayType *tp) {

	TrayComponentType *cp;
	int result;
	int temp;

	result = 0;
	for(cp = tp->components; cp; cp = cp->next) {
		temp = cp->width;
		if(temp > 0) {
			temp += 2 * tp->border;
			if(temp > result) {
				result = temp;
			}
		}
	}

	return result;

}

/***************************************************************************
 * Compute the max component height.
 ***************************************************************************/
int ComputeMaxHeight(TrayType *tp) {

	TrayComponentType *cp;
	int result;
	int temp;

	result = 0;
	for(cp = tp->components; cp; cp = cp->next) {
		temp = cp->height;
		if(temp > 0) {
			temp += 2 * tp->border;
			if(temp > result) {
				result = temp;
			}
		}
	}

	return result;

}

/***************************************************************************
 ***************************************************************************/
int CheckHorizontalFill(TrayType *tp) {

	TrayComponentType *cp;

	for(cp = tp->components; cp; cp = cp->next) {
		if(cp->width == 0) {
			return 1;
		}
	}

	return 0;

}

/***************************************************************************
 ***************************************************************************/
int CheckVerticalFill(TrayType *tp) {

	TrayComponentType *cp;

	for(cp = tp->components; cp; cp = cp->next) {
		if(cp->height == 0) {
			return 1;
		}
	}

	return 0;

}

/***************************************************************************
 ***************************************************************************/
void ComputeTraySize(TrayType *tp) {

	TrayComponentType *cp;

	/* Determine the first dimension. */
	if(tp->layout == LAYOUT_HORIZONTAL) {

		if(tp->height == 0) {
			tp->height = ComputeMaxHeight(tp);
		}

		if(tp->height == 0) {
			tp->height = 32;
		}

	} else {

		if(tp->width == 0) {
			tp->width = ComputeMaxWidth(tp);
		}

		if(tp->width == 0) {
			tp->width = 32;
		}

	}

	/* Now at least one size is known. Inform the components. */
	for(cp = tp->components; cp; cp = cp->next) {
		if(cp->SetSize) {
			if(tp->layout == LAYOUT_HORIZONTAL) {
				(cp->SetSize)(cp, 0, tp->height - 2 * tp->border);
			} else {
				(cp->SetSize)(cp, tp->width - 2 * tp->border, 0);
			}
		}
	}

	/* Determine the missing dimension (if there is one). */
	if(tp->layout == LAYOUT_HORIZONTAL) {
		if(tp->width == 0) {
			if(CheckHorizontalFill(tp)) {
				tp->width = rootWidth;
			} else {
				tp->width = ComputeMaxWidth(tp);
			}
			if(tp->width == 0) {
				tp->width = 32;
			}
		}
	} else {
		if(tp->height == 0) {
			if(CheckVerticalFill(tp)) {
				tp->height = rootHeight;
			} else {
				tp->height = ComputeMaxHeight(tp);
			}
			if(tp->height == 0) {
				tp->height = 32;
			}
		}
	}

	/* Determine the screen offset. */
	if(tp->x < 0) {
		tp->x = rootWidth + tp->x - tp->width + 1;
	}
	if(tp->y < 0) {
		tp->y = rootHeight + tp->y - tp->height + 1;
	}

}

/***************************************************************************
 ***************************************************************************/
void ShowTray(TrayType *tp) {

	int x, y;
	int moveMouse;
	XEvent event;

	if(tp->hidden) {

		JXMoveWindow(display, tp->window, tp->x, tp->y);

		GetMousePosition(&x, &y);

		moveMouse = 0;

		if(x < tp->x || x >= tp->x + tp->width) {
			x = tp->x + tp->width / 2;
			moveMouse = 1;
		}

		if(y < tp->y || y >= tp->y + tp->height) {
			y = tp->y + tp->height / 2;
			moveMouse = 1;
		}

		if(moveMouse) {
			SetMousePosition(rootWindow, x, y);
			JXCheckMaskEvent(display, LeaveWindowMask, &event);
		}

		tp->hidden = 0;
	}

}

/***************************************************************************
 ***************************************************************************/
void HideTray(TrayType *tp) {

	int x, y;

	if(tp->autoHide && !tp->hidden) {
		tp->hidden = 1;

		/* Determine where to move the tray. */
		if(tp->layout == LAYOUT_HORIZONTAL) {

			x = tp->x;

			if(tp->y >= rootHeight / 2) {
				y = rootHeight - 1;
			} else {
				y = 1 - tp->height;
			}

		} else {

			y = tp->y;

			if(tp->x >= rootWidth / 2) {
				x = rootWidth - 1;
			} else {
				x = 1 - tp->width;
			}

		}

		/* Move it. */
		JXMoveWindow(display, tp->window, x, y);
	}

}

/***************************************************************************
 ***************************************************************************/
int ProcessTrayEvent(const XEvent *event) {

	TrayType *tp;

	for(tp = trays; tp; tp = tp->next) {
		if(event->xany.window == tp->window) {
			switch(event->type) {
			case Expose:
				HandleTrayExpose(tp, &event->xexpose);
				break;
			case EnterNotify:
				HandleTrayEnterNotify(tp, &event->xcrossing);
				break;
			case LeaveNotify:
				HandleTrayLeaveNotify(tp, &event->xcrossing);
				break;
			case ButtonPress:
				HandleTrayButtonPress(tp, &event->xbutton);
				break;
			case MotionNotify:
				HandleTrayMotionNotify(tp, &event->xmotion);
				break;
			default:
				break;
			}
			return 1;
		}
	}

	return 0;

}

/***************************************************************************
 ***************************************************************************/
void HandleTrayExpose(TrayType *tp, const XExposeEvent *event) {

	DrawTray(tp);

}

/***************************************************************************
 ***************************************************************************/
void HandleTrayEnterNotify(TrayType *tp, const XCrossingEvent *event) {

	ShowTray(tp);

}

/***************************************************************************
 ***************************************************************************/
void HandleTrayLeaveNotify(TrayType *tp, const XCrossingEvent *event) {

	HideTray(tp);

}

/***************************************************************************
 ***************************************************************************/
void HandleTrayButtonPress(TrayType *tp, const XButtonEvent *event) {

	TrayComponentType *cp;
	int xoffset, yoffset;
	int width, height;
	int x, y;
	int mask;

	xoffset = tp->border;
	yoffset = tp->border;
	for(cp = tp->components; cp; cp = cp->next) {
		width = cp->width;
		height = cp->height;
		if(event->x >= xoffset && event->x < xoffset + width) {
			if(event->y >= yoffset && event->y < yoffset + height) {
				if(cp->ProcessButtonEvent) {
					x = event->x - xoffset;
					y = event->y - yoffset;
					mask = event->button;
					(cp->ProcessButtonEvent)(cp, x, y, mask);
				}
				break;
			}
		}
		if(tp->layout == LAYOUT_HORIZONTAL) {
			xoffset += width;
		} else {
			yoffset += height;
		}
	}
}

/***************************************************************************
 ***************************************************************************/
void HandleTrayMotionNotify(TrayType *tp, const XMotionEvent *event) {

	TrayComponentType *cp;
	int xoffset, yoffset;
	int width, height;
	int x, y;
	int mask;

	xoffset = tp->border;
	yoffset = tp->border;
	for(cp = tp->components; cp; cp = cp->next) {
		width = cp->width;
		height = cp->height;
		if(event->x >= xoffset && event->x < xoffset + width) {
			if(event->y >= yoffset && event->y < yoffset + height) {
				if(cp->ProcessMotionEvent) {
					x = event->x - xoffset;
					y = event->y - yoffset;
					mask = event->state;
					(cp->ProcessMotionEvent)(cp, x, y, mask);
				}
				break;
			}
		}
		if(tp->layout == LAYOUT_HORIZONTAL) {
			xoffset += width;
		} else {
			yoffset += height;
		}
	}
}

/***************************************************************************
 ***************************************************************************/
void DrawTray() {

	TrayType *tp;

	if(shouldExit) {
		return;
	}

	for(tp = trays; tp; tp = tp->next) {
		DrawSpecificTray(tp);
	}

}

/***************************************************************************
 ***************************************************************************/
void DrawSpecificTray(const TrayType *tp) {

	TrayComponentType *cp;
	int x;

	Assert(tp);

	/* Draw components. */
	for(cp = tp->components; cp; cp = cp->next) {
		UpdateSpecificTray(tp, cp);
	}

	/* Draw the border. */
	for(x = 0; x < tp->border; x++) {

		/* Top */
		JXSetForeground(display, tp->gc, colors[COLOR_TRAY_UP]);
		JXDrawLine(display, tp->window, tp->gc,
			0, x,
			tp->width - x - 1, x);

		/* Bottom */
		JXSetForeground(display, tp->gc, colors[COLOR_TRAY_DOWN]);
		JXDrawLine(display, tp->window, tp->gc,
			x + 1, tp->height - x - 1,
			tp->width - x - 2, tp->height - x - 1);

		/* Left */
		JXSetForeground(display, tp->gc, colors[COLOR_TRAY_UP]);
		JXDrawLine(display, tp->window, tp->gc,
			x, x,
			x, tp->height - x - 1);

		/* Right */
		JXSetForeground(display, tp->gc, colors[COLOR_TRAY_DOWN]);
		JXDrawLine(display, tp->window, tp->gc, 
			tp->width - x - 1, x + 1,
			tp->width - x - 1, tp->height - x - 1);

	}

}

/***************************************************************************
 ***************************************************************************/
void UpdateSpecificTray(const TrayType *tp, const TrayComponentType *cp) {
	if(cp->pixmap != None && !shouldExit) {
		JXCopyArea(display, cp->pixmap, tp->window, tp->gc, 0, 0,
			cp->width, cp->height, cp->x, cp->y);
	}
}

/***************************************************************************
 ***************************************************************************/
TrayType *GetTrays() {
	return trays;
}

/***************************************************************************
 ***************************************************************************/
Window GetSupportingWindow() {

	if(trays) {
		return trays->window;
	} else if(supportingWindow != None) {
		return supportingWindow;
	} else {
		supportingWindow = JXCreateSimpleWindow(display, rootWindow,
			0, 0, 1, 1, 0, 0, 0);
		return supportingWindow;
	}

}

/***************************************************************************
 ***************************************************************************/
void SetAutoHideTray(TrayType *tp, int v) {
	Assert(tp);
	tp->autoHide = v;
}

/***************************************************************************
 ***************************************************************************/
void SetTrayX(TrayType *tp, const char *str) {
	Assert(tp);
	Assert(str);
	tp->x = atoi(str);
}

/***************************************************************************
 ***************************************************************************/
void SetTrayY(TrayType *tp, const char *str) {
	Assert(tp);
	Assert(str);
	tp->y = atoi(str);
}

/***************************************************************************
 ***************************************************************************/
void SetTrayWidth(TrayType *tp, const char *str) {

	int width;

	Assert(tp);
	Assert(str);

	width = atoi(str);

	if(width < 0) {
		Warning("invalid tray width: %d", width);
		tp->width = 0;
	} else {
		tp->width = width;
	}

}

/***************************************************************************
 ***************************************************************************/
void SetTrayHeight(TrayType *tp, const char *str) {

	int height;

	Assert(tp);
	Assert(str);

	height = atoi(str);

	if(height < 0) {
		Warning("invalid tray height: %d", height);
	} else {
		tp->height = height;
	}

}


/***************************************************************************
 ***************************************************************************/
void SetTrayLayout(TrayType *tp, const char *str) {

	Assert(tp);
	Assert(str);

	if(!strcmp(str, "horizontal")) {
		tp->layout = LAYOUT_HORIZONTAL;
	} else if(!strcmp(str, "vertical")) {
		tp->layout = LAYOUT_VERTICAL;
	} else {
		Warning("invalid tray layout: \"%s\"", str);
	}

}

/***************************************************************************
 ***************************************************************************/
void SetTrayLayer(TrayType *tp, const char *str) {

	int temp;

	Assert(tp);
	Assert(str);

	temp = atoi(str);
	if(temp < LAYER_BOTTOM || temp > LAYER_TOP) {
		Warning("invalid tray layer: %d", temp);
		tp->layer = DEFAULT_TRAY_LAYER;
	} else {
		tp->layer = temp;
	}

}

/***************************************************************************
 ***************************************************************************/
void SetTrayBorder(TrayType *tp, const char *str) {

	int temp;

	Assert(tp);
	Assert(str);

	temp = atoi(str);
	if(temp < MIN_TRAY_BORDER || temp > MAX_TRAY_BORDER) {
		Warning("invalid tray border: %d", temp);
		tp->border = DEFAULT_TRAY_BORDER;
	} else {
		tp->border = temp;
	}

}


