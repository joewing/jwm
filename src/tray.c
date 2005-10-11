/***************************************************************************
 * Functions to handle the tray.
 * Copyright (C) 2004 Joe Wingbermuehle
 ***************************************************************************/

#include "jwm.h"

static const int trayBorderSize = 2;

static TrayType *trays;

static void Update(void *object);

static void HandleTrayExpose(TrayType *tp, const XExposeEvent *event);
static void HandleTrayEnterNotify(TrayType *tp, const XCrossingEvent *event);
static void HandleTrayLeaveNotify(TrayType *tp, const XCrossingEvent *event);

static void HandleTrayButtonPress(TrayType *tp, const XButtonEvent *event);
static void HandleTrayMotionNotify(TrayType *tp, const XMotionEvent *event);

/***************************************************************************
 ***************************************************************************/
void InitializeTray()
{
	trays = NULL;
}

/***************************************************************************
 ***************************************************************************/
void StartupTray()
{

	XSetWindowAttributes attr;
	TrayType *tp;
	TrayComponentType *cp;
	Window w;
	int temp;
	int variableCount;
	int variableSize;
	int variableRemainder;
	int width, height;
	int xoffset, yoffset;

	for(tp = trays; tp; tp = tp->next) {

		GetTrayX(tp);
		GetTrayY(tp);
		GetTrayWidth(tp);
		GetTrayHeight(tp);

		/* Get the remaining size after setting fixed size components. */
		/* Also, keep track of the number of variable size components. */
		width = tp->width;
		height = tp->height;
		variableCount = 0;
		for(cp = tp->components; cp; cp = cp->next) {
			if(tp->layout == LAYOUT_HORIZONTAL) {
				if(cp->SetSize) {
					(cp->SetSize)(cp->object, 0, tp->height);
				}
				temp = (cp->GetWidth)(cp->object);
				if(temp < rootWidth) {
					width -= temp;
				} else {
					++variableCount;
				}
			} else {
				if(cp->SetSize) {
					(cp->SetSize)(cp->object, tp->width, 0);
				}
				temp = (cp->GetHeight)(cp->object);
				if(temp < rootHeight) {
					height -= temp;
				} else {
					++variableCount;
				}
			}
		}

		/* Distribute excess size amoung variable size components.
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
		attr.override_redirect = True;
		attr.event_mask = ButtonPressMask | ExposureMask | KeyPressMask;
		attr.background_pixel = colors[COLOR_TRAY_BG];

		tp->window = JXCreateWindow(display, rootWindow,
			tp->x - trayBorderSize, tp->y - trayBorderSize,
			tp->width + 2 * trayBorderSize, tp->height + 2 * trayBorderSize,
			0, rootDepth, InputOutput, rootVisual,
			CWOverrideRedirect | CWBackPixel | CWEventMask, &attr);

		JXSelectInput(display, tp->window, EnterWindowMask
			| LeaveWindowMask | ExposureMask | ButtonPressMask
			| PointerMotionMask);

		SetDefaultCursor(tp->window);

		tp->gc = JXCreateGC(display, tp->window, 0, NULL);

		/* Create and layout items on the tray. */
		xoffset = 0;
		yoffset = 0;
		for(cp = tp->components; cp; cp = cp->next) {
			if(cp->Create) {
				if(tp->layout == LAYOUT_HORIZONTAL) {
					width = (cp->GetWidth)(cp->object);
					if(width == rootWidth) {
						width = variableSize;
						if(variableRemainder) {
							++width;
							--variableRemainder;
						}
					}
					height = tp->height;
				} else {
					width = tp->width;
					height = (cp->GetHeight)(cp->object);
					if(height == rootHeight) {
						height = variableSize;
						if(variableRemainder) {
							++height;
							--variableRemainder;
						}
					}
				}
				(cp->Create)(cp->object, tp, Update, width, height);
			}
			if(cp->GetWindow) {
				w = (cp->GetWindow)(cp->object);
				if(w != None) {
					JXReparentWindow(display, w, tp->window, xoffset, yoffset);
				}
			}
			if(tp->layout == LAYOUT_HORIZONTAL) {
				xoffset += (cp->GetWidth)(cp->object);
			} else {
				yoffset += (cp->GetHeight)(cp->object);
			}
		}

		/* Show the tray. */
		JXMapWindow(display, tp->window);

	}

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
				(trays->components->Destroy)(trays->components->object);
			}
			Release(trays->components);
			trays->components = cp;
		}

		JXFreeGC(display, trays->gc);
		JXDestroyWindow(display, trays->window);

		Release(trays);

		trays = tp;
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
	tp->y = -32;
	tp->width = 0;
	tp->height = 32;
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
void AddTrayComponent(TrayType *tp, TrayComponentType *cp)
{

	Assert(tp);
	Assert(cp);

	if(tp->componentsTail) {
		tp->componentsTail->next = cp;
	} else {
		tp->components = cp;
	}
	tp->componentsTail = cp;
	cp->next = NULL;

}

/***************************************************************************
 ***************************************************************************/
int GetTrayWidth(TrayType *tp)
{

	TrayComponentType *cp;
	int temp;

	if(tp->width == 0) {
		if(tp->layout == LAYOUT_HORIZONTAL) {
			tp->width = rootWidth;
		} else {
			tp->width = 64;
			for(cp = tp->components; cp; cp = cp->next) {
				if(cp->GetWidth) {
					temp = (cp->GetWidth)(cp->object);
					if(temp > tp->width && temp < rootWidth) {
						tp->width = temp;
					}
				}
			}
		}
	}
	return tp->width;
}

/***************************************************************************
 ***************************************************************************/
int GetTrayHeight(TrayType *tp)
{

	TrayComponentType *cp;
	int temp;

	if(tp->height == 0) {
		if(tp->layout == LAYOUT_HORIZONTAL) {
			tp->height = 16;
			for(cp = tp->components; cp; cp = cp->next) {
				if(cp->GetHeight) {
					temp = (cp->GetHeight)(cp->object);
					if(temp > tp->height && temp < rootHeight) {
						tp->height = temp;
					}
				}
			}
		} else {
			tp->height = rootHeight;
		}
	}
	return tp->height;
}

/***************************************************************************
 ***************************************************************************/
int GetTrayX(TrayType *tp)
{
	if(tp->x < 0) {
		tp->x = rootWidth + tp->x;
	}
	return tp->x;
}

/***************************************************************************
 ***************************************************************************/
int GetTrayY(TrayType *tp)
{
	if(tp->y < 0) {
		tp->y = rootHeight + tp->y;
	}
	return tp->y;
}

/***************************************************************************
 ***************************************************************************/
int GetTrayIconSize(TrayType *tp)
{
#ifdef USE_ICONS
	return GetTrayHeight(tp) - 9;
#else
	return 0;
#endif
}

/***************************************************************************
 ***************************************************************************/
void ShowTray(TrayType *tp) {
	if(tp->hidden) {
/*
		JXMoveWindow(display, tp->window, 0,
			rootHeight - trayHeight - TRAY_BEVEL);
*/
		tp->hidden = 0;
	}
}

/***************************************************************************
 ***************************************************************************/
void HideTray(TrayType *tp) {

	if(tp->autoHide && !tp->hidden) {
		tp->hidden = 1;
/*
		JXMoveWindow(display, tp->window, 0, rootHeight - 1);
*/
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
void HandleTrayExpose(TrayType *tp, const XExposeEvent *event)
{
	DrawTray(tp);
}

/***************************************************************************
 ***************************************************************************/
void HandleTrayEnterNotify(TrayType *tp, const XCrossingEvent *event)
{
	ShowTray(tp);
}

/***************************************************************************
 ***************************************************************************/
void HandleTrayLeaveNotify(TrayType *tp, const XCrossingEvent *event)
{
}

/***************************************************************************
 ***************************************************************************/
void HandleTrayButtonPress(TrayType *tp, const XButtonEvent *event)
{
	TrayComponentType *cp;
	int xoffset, yoffset;
	int width, height;
	int x, y;
	int mask;

	xoffset = trayBorderSize;
	yoffset = trayBorderSize;
	for(cp = tp->components; cp; cp = cp->next) {
		width = (cp->GetWidth)(cp->object);
		height = (cp->GetHeight)(cp->object);
		if(event->x >= xoffset && event->x < xoffset + width) {
			if(event->y >= yoffset && event->y < yoffset + height) {
				if(cp->ProcessButtonEvent) {
					x = event->x - xoffset;
					y = event->y - yoffset;
					mask = event->button;
					(cp->ProcessButtonEvent)(cp->object, x, y, mask);
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
void HandleTrayMotionNotify(TrayType *tp, const XMotionEvent *event)
{
	TrayComponentType *cp;
	int xoffset, yoffset;
	int width, height;

	xoffset = trayBorderSize;
	yoffset = trayBorderSize;
	for(cp = tp->components; cp; cp = cp->next) {
		width = (cp->GetWidth)(cp->object);
		height = (cp->GetHeight)(cp->object);
		if(event->x >= xoffset && event->x < xoffset + width) {
			if(event->y >= yoffset && event->y < yoffset + height) {
/*
				if(cp->ProcessEvent) {
					(cp->ProcessEvent)(cp->object, event);
				}
*/
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
void Update(void *object)
{
	DrawSpecificTray((TrayType*)object);
}

/***************************************************************************
 ***************************************************************************/
void DrawTray()
{

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
void DrawSpecificTray(const TrayType *tp)
{

	TrayComponentType *cp;
	int width, height;
	int xoffset, yoffset;

	Assert(tp);

	/* Draw components. */
	xoffset = trayBorderSize;
	yoffset = trayBorderSize;
	for(cp = tp->components; cp; cp = cp->next) {

		width = (cp->GetWidth)(cp->object);
		height = (cp->GetHeight)(cp->object);

		if(cp->GetPixmap) {
			JXCopyArea(display, (cp->GetPixmap)(cp->object),
				tp->window, tp->gc, 0, 0, width, height,
				xoffset, yoffset);
		}

		if(tp->layout == LAYOUT_HORIZONTAL) {
			xoffset += width;
		} else {
			yoffset += height;
		}

	}

	/* Top border. */
	JXSetForeground(display, tp->gc, colors[COLOR_TRAY_UP]);
	JXFillRectangle(display, tp->window, tp->gc, 0, 0,
		tp->width + 2 * trayBorderSize, trayBorderSize);

	/* Bottom border. */
	JXSetForeground(display, tp->gc, colors[COLOR_TRAY_DOWN]);
	JXFillRectangle(display, tp->window, tp->gc,
		0, tp->height + trayBorderSize,
		tp->width + 2 * trayBorderSize, trayBorderSize);

	/* Right border. */
	JXDrawLine(display, tp->window, tp->gc,
		tp->width + 3, 1, tp->width + 3, tp->height + 2);
	JXDrawLine(display, tp->window, tp->gc,
		tp->width + 2, 2, tp->width + 2, tp->height + 3);

	/* Left border. */
	JXSetForeground(display, tp->gc, colors[COLOR_TRAY_UP]);
	JXDrawLine(display, tp->window, tp->gc,
		0, 2, 0, tp->height + 3);
	JXDrawLine(display, tp->window, tp->gc,
		1, 2, 1, tp->height + 2);


}

/***************************************************************************
 ***************************************************************************/
void SetAutoHideTray(TrayType *tp, int v)
{
	Assert(tp);
	tp->autoHide = v;
}

/***************************************************************************
 ***************************************************************************/
void SetTrayX(TrayType *tp, const char *str)
{
	Assert(tp);
	Assert(str);
	tp->x = atoi(str);
}

/***************************************************************************
 ***************************************************************************/
void SetTrayY(TrayType *tp, const char *str)
{
	Assert(tp);
	Assert(str);
	tp->y = atoi(str);
}

/***************************************************************************
 ***************************************************************************/
void SetTrayWidth(TrayType *tp, const char *str)
{

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
void SetTrayHeight(TrayType *tp, const char *str)
{

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
void SetTrayLayout(TrayType *tp, const char *str)
{

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
	} else {
		tp->layer = temp;
	}

}

