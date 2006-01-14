/***************************************************************************
 * Dock functions.
 * Copyright (C) 2006 Joe Wingbermuehle
 ***************************************************************************/

#include "jwm.h"
#include "dock.h"
#include "tray.h"
#include "main.h"
#include "error.h"
#include "color.h"

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

#define SYSTEM_TRAY_ORIENTATION_HORZ 0
#define SYSTEM_TRAY_ORIENTATION_VERT 1

typedef struct DockNode {

	Window window;

	struct DockNode *next;

} DockNode;

typedef struct DockType {

	TrayComponentType *cp;

	Window window;

	DockNode *nodes;

} DockType;

static const char *BASE_SELECTION_NAME = "_NET_SYSTEM_TRAY_S%d";

static DockType *dock = NULL;
static int owner;
static Atom dockAtom;
static unsigned long orientation;

static void SetSize(TrayComponentType *cp, int width, int height);
static void Create(TrayComponentType *cp);
static void Destroy(TrayComponentType *cp);
static void Resize(TrayComponentType *cp);

static void DockWindow(Window win);
static int UndockWindow(Window win);

static void UpdateDock();

/***************************************************************************
 ***************************************************************************/
void InitializeDock() {
}

/***************************************************************************
 ***************************************************************************/
void StartupDock() {

	char *selectionName;

	if(!dock) {
		return;
	}

	selectionName = Allocate(strlen(BASE_SELECTION_NAME) + 1);
	sprintf(selectionName, BASE_SELECTION_NAME, rootScreen);
	dockAtom = JXInternAtom(display, selectionName, False);
	Release(selectionName);

	/* The location and size of the window doesn't matter here. */
	if(dock->window == None) {
		dock->window = JXCreateSimpleWindow(display, rootWindow,
			/* x, y, width, height */ 0, 0, 1, 1,
			/* border_size, border_color */ 0, 0,
			/* background */ colors[COLOR_TRAY_BG]);
	}
	dock->cp->window = dock->window;

}

/***************************************************************************
 ***************************************************************************/
void ShutdownDock() {

	DockNode *np;

	if(dock) {

		if(shouldRestart) {

			JXReparentWindow(display, dock->window, rootWindow, 0, 0);
			JXUnmapWindow(display, dock->window);

		} else {

			while(dock->nodes) {
				np = dock->nodes->next;
				JXReparentWindow(display, dock->nodes->window, rootWindow, 0, 0);
				Release(dock->nodes);
				dock->nodes = np;
			}

			if(owner) {
				JXSetSelectionOwner(display, dockAtom, None, CurrentTime);
			}

			JXDestroyWindow(display, dock->window);

		}

	}

}

/***************************************************************************
 ***************************************************************************/
void DestroyDock() {

	if(dock) {
		if(shouldRestart) {
			dock->cp = NULL;
		} else {
			Release(dock);
		}
	}

}

/***************************************************************************
 ***************************************************************************/
TrayComponentType *CreateDock() {

	TrayComponentType *cp;

	if(dock != NULL && dock->cp != NULL) {
		Warning("only one Dock allowed");
		return NULL;
	} else if(dock == NULL) {
		dock = Allocate(sizeof(DockType));
		dock->nodes = NULL;
		dock->window = None;
	}

	cp = CreateTrayComponent();
	cp->object = dock;
	dock->cp = cp;
	cp->requestedWidth = 1;
	cp->requestedHeight = 1;

	cp->SetSize = SetSize;
	cp->Create = Create;
	cp->Destroy = Destroy;
	cp->Resize = Resize;

	return cp;

}

/***************************************************************************
 ***************************************************************************/
void SetSize(TrayComponentType *cp, int width, int height) {

	int count;
	DockNode *np;

	count = 0;
	for(np = dock->nodes; np; np = np->next) {
		++count;
	}

	if(width == 0) {
		if(count > 0) {
			cp->width = count * height;
			cp->requestedWidth = cp->width;
		} else {
			cp->width = 1;
			cp->requestedWidth = 1;
		}
	} else if(height == 0) {
		if(count > 0) {
			cp->height = count * width;
			cp->requestedHeight = cp->height;
		} else {
			cp->height = 1;
			cp->requestedHeight = 1;
		}
	}

}

/***************************************************************************
 ***************************************************************************/
void Create(TrayComponentType *cp) {

	XEvent event;
	Atom orientationAtom;

	Assert(cp);

	if(cp->window != None) {
		JXResizeWindow(display, cp->window, cp->width, cp->height);
		JXMapRaised(display, cp->window);
	}

	orientationAtom = JXInternAtom(display, "_NET_SYSTEM_TRAY_ORIENTATION",
		False);
	if(cp->height == 1) {
		orientation = SYSTEM_TRAY_ORIENTATION_VERT;
	} else {
		orientation = SYSTEM_TRAY_ORIENTATION_HORZ;
	}
	JXChangeProperty(display, dock->cp->window, orientationAtom,
		XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&orientation, 1);

	owner = 1;
	JXSetSelectionOwner(display, dockAtom, dock->cp->window, CurrentTime);
	if(JXGetSelectionOwner(display, dockAtom) != dock->cp->window) {

		owner = 0;
		Warning("could not acquire system tray selection");

	} else {

		memset(&event, 0, sizeof(event));
		event.xclient.type = ClientMessage;
		event.xclient.window = rootWindow;
		event.xclient.message_type = JXInternAtom(display, "MANAGER", False);
		event.xclient.format = 32;
		event.xclient.data.l[0] = CurrentTime;
		event.xclient.data.l[1] = dockAtom;
		event.xclient.data.l[2] = dock->cp->window;

		JXSendEvent(display, rootWindow, False, StructureNotifyMask, &event);

	}

}

/***************************************************************************
 ***************************************************************************/
void Resize(TrayComponentType *cp) {

	JXResizeWindow(display, cp->window, cp->width, cp->height);
	UpdateDock();

}

/***************************************************************************
 ***************************************************************************/
void Destroy(TrayComponentType *cp) {
}

/***************************************************************************
 ***************************************************************************/
void HandleDockEvent(const XClientMessageEvent *event) {

	long opcode;
	Window win;

	Assert(event);

	opcode = event->data.l[1];
	win = event->data.l[2];

	switch(opcode) {
	case SYSTEM_TRAY_REQUEST_DOCK:
		DockWindow(win);
		break;
	case SYSTEM_TRAY_BEGIN_MESSAGE:
		break;
	case SYSTEM_TRAY_CANCEL_MESSAGE:
		break;
	default:
		Debug("invalid opcode in dock event");
		break;
	}

}

/***************************************************************************
 ***************************************************************************/
int HandleDockResizeRequest(const XResizeRequestEvent *event) {

	DockNode *np;

	for(np = dock->nodes; np; np = np->next) {
		if(np->window == event->window) {

			JXResizeWindow(display, np->window, event->width, event->height);
			UpdateDock();

			return 1;
		}
	}

	return 0;
}

/***************************************************************************
 ***************************************************************************/
int HandleDockDestroy(Window win) {
	if(dock) {
		return UndockWindow(win);
	} else {
		return 0;
	}
}

/***************************************************************************
 ***************************************************************************/
int HandleDockSelectionClear(const XSelectionClearEvent *event) {

	if(event->selection == dockAtom) {
		Debug("lost _NET_SYSTEM_TRAY selection");
		owner = 0;
	}

	return 0;

}

/***************************************************************************
 ***************************************************************************/
void DockWindow(Window win) {

	DockNode *np;

	Assert(dock);

	UndockWindow(win);

	if(win != None) {

		np = Allocate(sizeof(DockNode));
		np->window = win;
		np->next = dock->nodes;
		dock->nodes = np;

		if(orientation == SYSTEM_TRAY_ORIENTATION_HORZ) {
			if(dock->cp->requestedWidth > 1) {
				dock->cp->requestedWidth += dock->cp->height;
			} else {
				dock->cp->requestedWidth = dock->cp->height;
			}
		} else {
			if(dock->cp->requestedHeight > 1) {
				dock->cp->requestedHeight += dock->cp->width;
			} else {
				dock->cp->requestedHeight = dock->cp->width;
			}
		}

		JXAddToSaveSet(display, win);
		JXSelectInput(display, win,
			SubstructureNotifyMask | StructureNotifyMask | ResizeRedirectMask);
		JXReparentWindow(display, win, dock->cp->window, 0, 0);
		JXMapRaised(display, win);

	}

	ResizeTray(dock->cp->tray);

}

/***************************************************************************
 ***************************************************************************/
int UndockWindow(Window win) {

	DockNode *np;
	DockNode *last;

	Assert(dock);

	last = NULL;
	for(np = dock->nodes; np; np = np->next) {
		if(np->window == win) {
			if(last) {
				last->next = np->next;
			} else {
				dock->nodes = np->next;
			}

			Release(np);

			if(orientation == SYSTEM_TRAY_ORIENTATION_HORZ) {
				dock->cp->requestedWidth -= dock->cp->height;
				if(dock->cp->requestedWidth <= 0) {
					dock->cp->requestedWidth = 1;
				}
			} else {
				dock->cp->requestedHeight -= dock->cp->width;
				if(dock->cp->requestedHeight <= 0) {
					dock->cp->requestedHeight = 1;
				}
			}

			ResizeTray(dock->cp->tray);

			return 1;

		}
		last = np;
	}

	return 0;
}

/***************************************************************************
 ***************************************************************************/
void UpdateDock() {

	XWindowAttributes attr;
	DockNode *np;
	int x, y;
	int width, height;
	int xoffset, yoffset;
	int itemWidth, itemHeight;
	double ratio;

	Assert(dock);

	if(orientation == SYSTEM_TRAY_ORIENTATION_HORZ) {
		itemWidth = dock->cp->height;
		itemHeight = dock->cp->height;
	} else {
		itemHeight = dock->cp->width;
		itemWidth = dock->cp->width;
	}

	x = 0;
	y = 0;
	for(np = dock->nodes; np; np = np->next) {

		xoffset = 0;
		yoffset = 0;
		width = itemWidth;
		height = itemHeight;

		if(JXGetWindowAttributes(display, np->window, &attr)) {

			ratio = (double)attr.width / attr.height;

			if(ratio > 1.0) {
				if(width > attr.width) {
					width = attr.width;
				}
				height = width / ratio;
			} else {
				if(height > attr.height) {
					height = attr.height;
				}
				width = height * ratio;
			}

			xoffset = (itemWidth - width) / 2;
			yoffset = (itemHeight - height) / 2;

		}

		JXMoveResizeWindow(display, np->window, x + xoffset, y + yoffset,
			width, height);

		if(orientation == SYSTEM_TRAY_ORIENTATION_HORZ) {
			x += itemWidth;
		} else {
			y += itemHeight;
		}
	}

}


