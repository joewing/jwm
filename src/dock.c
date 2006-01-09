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

	DockNode *nodes;

} DockType;

static const char *BASE_SELECTION_NAME = "_NET_SYSTEM_TRAY_S%d";

static DockType *dock;
static int owner;
static Atom dockAtom;
static unsigned long orientation;

static void Create(TrayComponentType *cp);
static void Destroy(TrayComponentType *cp);
static void Resize(TrayComponentType *cp);

static void DockWindow(Window win);
static int UndockWindow(Window win);

static void UpdateDock();

/***************************************************************************
 ***************************************************************************/
void InitializeDock() {
	dock = NULL;
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
	dock->cp->window = JXCreateSimpleWindow(display, rootWindow,
		/* x, y, width, height */ 0, 0, 1, 1,
		/* border_size, border_color */ 0, 0,
		/* background */ colors[COLOR_TRAY_BG]);

}

/***************************************************************************
 ***************************************************************************/
void ShutdownDock() {

	DockNode *np;

	if(dock) {

		while(dock->nodes) {
			np = dock->nodes->next;
			JXReparentWindow(display, dock->nodes->window, rootWindow, 0, 0);
			Release(dock->nodes);
			dock->nodes = np;
		}

		if(owner) {
			JXSetSelectionOwner(display, dockAtom, None, CurrentTime);
		}

		JXDestroyWindow(display, dock->cp->window);

		Release(dock);

	}

}

/***************************************************************************
 ***************************************************************************/
void DestroyDock() {
}

/***************************************************************************
 ***************************************************************************/
TrayComponentType *CreateDock() {

	TrayComponentType *cp;

	if(dock != NULL) {
		Warning("only one Dock allowed");
		return NULL;
	}

	dock = Allocate(sizeof(DockType));
	dock->nodes = NULL;

	cp = CreateTrayComponent();
	cp->object = dock;
	dock->cp = cp;
	cp->requestedWidth = 1;
	cp->requestedHeight = 1;

	cp->Create = Create;
	cp->Destroy = Destroy;
	cp->Resize = Resize;

	return cp;

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
int HandleDockDestroy(Window win) {
	if(dock) {
		return UndockWindow(win);
	} else {
		return 0;
	}
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
		JXReparentWindow(display, win, dock->cp->window, 0, 0);
		JXMapRaised(display, win);
		JXSelectInput(display, win, SubstructureNotifyMask | StructureNotifyMask);

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

	DockNode *np;
	int count;
	int itemWidth, itemHeight;
	int x, y;

	Assert(dock);

	count = 0;
	for(np = dock->nodes; np; np = np->next) {
		++count;
	}

	if(count > 0) {

		if(orientation == SYSTEM_TRAY_ORIENTATION_HORZ) {
			itemWidth = dock->cp->width / count;
			itemHeight = dock->cp->height;
			x = 0;
			y = 2;
		} else {
			itemHeight = dock->cp->height / count;
			itemWidth = dock->cp->width;
			x = 2;
			y = 0;
		}

		for(np = dock->nodes; np; np = np->next) {

			JXMoveResizeWindow(display, np->window, x, y, itemWidth, itemHeight);

			if(orientation == SYSTEM_TRAY_ORIENTATION_HORZ) {
				x += itemWidth;
			} else {
				y += itemHeight;
			}
		}

	}

}


