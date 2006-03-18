/****************************************************************************
 * Functions to handle swallowing client windows in the tray.
 ****************************************************************************/

#include "jwm.h"
#include "swallow.h"
#include "main.h"
#include "tray.h"
#include "error.h"
#include "root.h"
#include "color.h"
#include "client.h"
#include "event.h"

typedef struct SwallowNode {

	TrayComponentType *cp;

	char *name;
	char *command;
	int border;
	int userWidth;
	int userHeight;

	struct SwallowNode *next;

} SwallowNode;

static SwallowNode *swallowNodes;

static void Destroy(TrayComponentType *cp);
static void Resize(TrayComponentType *cp);

/****************************************************************************
 ****************************************************************************/
void InitializeSwallow() {
	swallowNodes = NULL;
}

/****************************************************************************
 ****************************************************************************/
void StartupSwallow() {

	SwallowNode *np;

	for(np = swallowNodes; np; np = np->next) {
		if(np->command) {
			RunCommand(np->command);
		}
	}

}

/****************************************************************************
 ****************************************************************************/
void ShutdownSwallow() {
}

/****************************************************************************
 ****************************************************************************/
void DestroySwallow() {

	SwallowNode *np;

	while(swallowNodes) {

		np = swallowNodes->next;

		Assert(swallowNodes->name);
		Release(swallowNodes->name);

		if(swallowNodes->command) {
			Release(swallowNodes->command);
		}

		Release(swallowNodes);
		swallowNodes = np;

	}

}

/****************************************************************************
 ****************************************************************************/
TrayComponentType *CreateSwallow(const char *name, const char *command,
	int width, int height) {

	TrayComponentType *cp;
	SwallowNode *np;

	if(!name) {
		Warning("cannot swallow a client with no name");
		return NULL;
	}

	/* Make sure this name isn't already used. */
	for(np = swallowNodes; np; np = np->next) {
		if(!strcmp(np->name, name)) {
			Warning("cannot swallow the same client multiple times");
			return NULL;
		}
	}

	np = Allocate(sizeof(SwallowNode));
	np->name = Allocate(strlen(name) + 1);
	strcpy(np->name, name);
	if(command) {
		np->command = Allocate(strlen(command) + 1);
		strcpy(np->command, command);
	} else {
		np->command = NULL;
	}

	np->next = swallowNodes;
	swallowNodes = np;

	cp = CreateTrayComponent();
	np->cp = cp;
	cp->object = np;
	cp->Destroy = Destroy;
	cp->Resize = Resize;

	if(width) {
		cp->requestedWidth = width;
		np->userWidth = 1;
	} else {
		cp->requestedWidth = 1;
		np->userWidth = 0;
	}
	if(height) {
		cp->requestedHeight = height;
		np->userHeight = 1;
	} else {
		cp->requestedHeight = 1;
		np->userHeight = 0;
	}

	return cp;

}

/****************************************************************************
 ****************************************************************************/
int ProcessSwallowEvent(const XEvent *event) {

	SwallowNode *np;

	for(np = swallowNodes; np; np = np->next) {
		if(event->xany.window == np->cp->window) {
			switch(event->type) {
			case DestroyNotify:
				np->cp->window = None;
				np->cp->requestedWidth = 1;
				np->cp->requestedHeight = 1;
				ResizeTray(np->cp->tray);
				break;
			case ResizeRequest:
				np->cp->requestedWidth
					= event->xresizerequest.width + np->border * 2;
				np->cp->requestedHeight
					= event->xresizerequest.height + np->border * 2;
				ResizeTray(np->cp->tray);
				break;
			default:
				break;
			}
			return 1;
		}
	}

	return 0;

}

/****************************************************************************
 ****************************************************************************/
void Resize(TrayComponentType *cp) {

	int width, height;

	SwallowNode *np = (SwallowNode*)cp->object;

	if(cp->window != None) {
		width = cp->width - np->border * 2;
		height = cp->height - np->border * 2;
		JXResizeWindow(display, cp->window, width, height);
	}

}

/****************************************************************************
 ****************************************************************************/
void Destroy(TrayComponentType *cp) {

	if(cp->window) {
		JXReparentWindow(display, cp->window, rootWindow, 0, 0);
		JXRemoveFromSaveSet(display, cp->window);
		SendClientMessage(cp->window, ATOM_WM_PROTOCOLS, ATOM_WM_DELETE_WINDOW);
	}

}

/****************************************************************************
 * Determine if this is a window to be swallowed, if it is, swallow it.
 ****************************************************************************/
int CheckSwallowMap(const XMapEvent *event) {

	SwallowNode *np;
	XClassHint hint;
	XWindowAttributes attr;

	for(np = swallowNodes; np; np = np->next) {

		if(np->cp->window != None) {
			continue;
		}

		Assert(np->cp->tray->window != None);

		if(JXGetClassHint(display, event->window, &hint)) {
			if(!strcmp(hint.res_name, np->name)) {

				/* Swallow the window. */
				JXAddToSaveSet(display, event->window);
				JXSelectInput(display, event->window,
					StructureNotifyMask | ResizeRedirectMask);
				JXSetWindowBorder(display, event->window, colors[COLOR_TRAY_BG]);
				JXReparentWindow(display, event->window,
					np->cp->tray->window, 0, 0);
				JXMapRaised(display, event->window);
				JXFree(hint.res_name);
				JXFree(hint.res_class);
				np->cp->window = event->window;

				/* Update the size. */
				JXGetWindowAttributes(display, event->window, &attr);
				np->border = attr.border_width;
				if(!np->userWidth) {
					np->cp->requestedWidth = attr.width + 2 * np->border;
				}
				if(!np->userHeight) {
					np->cp->requestedHeight = attr.height + 2 * np->border;
				}

				ResizeTray(np->cp->tray);

				return 1;

			}
		}

	}

	return 0;

}

