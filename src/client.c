/****************************************************************************
 * Functions to handle client windows.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"
#include "client.h"
#include "main.h"
#include "icon.h"
#include "hint.h"
#include "group.h"
#include "tray.h"
#include "confirm.h"
#include "key.h"
#include "cursor.h"
#include "taskbar.h"
#include "screen.h"
#include "pager.h"
#include "color.h"
#include "error.h"
#include "place.h"

static const int STACK_BLOCK_SIZE = 8;

ClientNode *nodes[LAYER_COUNT];
ClientNode *nodeTail[LAYER_COUNT];

static ClientNode *activeClient;

static Window *stack;
static int stackSize;
static int clientCount;

static void LoadFocus();
static void ReparentClient(ClientNode *np, int notOwner);
 
static void MinimizeTransients(ClientNode *np);
static void CheckShape(ClientNode *np);

static void RestoreTransients(ClientNode *np);

static void KillClientHandler(ClientNode *np);

/****************************************************************************
 ****************************************************************************/
void InitializeClients() {
}

/****************************************************************************
 * Load windows that are already mapped.
 ****************************************************************************/
void StartupClients() {
	XWindowAttributes attr;
	Window rootReturn, parentReturn, *childrenReturn;
	unsigned int childrenCount;
	unsigned int x;

	JXSync(display, False);
	JXGrabServer(display);

	stackSize = STACK_BLOCK_SIZE;
	stack = Allocate(sizeof(Window) * stackSize);
	clientCount = 0;
	activeClient = NULL;
	currentDesktop = 0;

	for(x = 0; x < LAYER_COUNT; x++) {
		nodes[x] = NULL;
		nodeTail[x] = NULL;
	}

	JXQueryTree(display, rootWindow, &rootReturn, &parentReturn,
		&childrenReturn, &childrenCount);

	for(x = 0; x < childrenCount; x++) {
		if(JXGetWindowAttributes(display, childrenReturn[x], &attr)) {
			if(attr.override_redirect == False
				&& attr.map_state == IsViewable) {
				AddClientWindow(childrenReturn[x], 1, 1);
			}
		}
	}

	JXFree(childrenReturn);

	JXSync(display, True);
	JXUngrabServer(display);

	LoadFocus();

	ExposeCurrentDesktop();

	UpdateTaskBar();
	UpdatePager();

}

/****************************************************************************
 ****************************************************************************/
void ShutdownClients() {
	int x;

	for(x = 0; x < LAYER_COUNT; x++) {
		while(nodeTail[x]) {
			RemoveClient(nodeTail[x]);
		}
	}
	Release(stack);
	stack = NULL;

}

/****************************************************************************
 ****************************************************************************/
void DestroyClients() {
}

/****************************************************************************
 * Set the focus to the window currently under the mouse pointer.
 ****************************************************************************/
void LoadFocus() {
	ClientNode *np;
	Window rootReturn, childReturn;
	int rootx, rooty;
	int winx, winy;
	unsigned int mask;

	JXQueryPointer(display, rootWindow, &rootReturn, &childReturn,
		&rootx, &rooty, &winx, &winy, &mask);

	np = FindClientByWindow(childReturn);
	if(np) {
		FocusClient(np);
	}

}

/****************************************************************************
 * Add a window to management.
 ****************************************************************************/
ClientNode *AddClientWindow(Window w, int alreadyMapped, int notOwner) {

	XWindowAttributes attr;
	ClientNode *np;

	Assert(w != None);

	if(JXGetWindowAttributes(display, w, &attr) == 0) {
		return NULL;
	}

	if(attr.override_redirect == True) {
		return NULL;
	}
	if(attr.class == InputOnly) {
		return NULL;
	}

	np = Allocate(sizeof(ClientNode));
	memset(np, 0, sizeof(ClientNode));

	np->window = w;
	np->owner = None;
	np->state.desktop = currentDesktop;
	np->controller = NULL;
	np->name = NULL;
	np->colormaps = NULL;

	np->x = attr.x;
	np->y = attr.y;
	np->width = attr.width;
	np->height = attr.height;
	np->cmap = attr.colormap;
	np->colormaps = NULL;
	np->state.status = STAT_NONE;
	np->state.layer = LAYER_NORMAL;

	np->state.border = BORDER_DEFAULT;
	np->borderAction = BA_NONE;

	ReadClientProtocols(np);

	if(!notOwner) {
		np->state.border = BORDER_OUTLINE | BORDER_TITLE | BORDER_MOVE;
		np->state.status |= STAT_WMDIALOG | STAT_STICKY;
	}

	/* We now know the layer, so insert */
	np->prev = NULL;
	np->next = nodes[np->state.layer];
	if(np->next) {
		np->next->prev = np;
	} else {
		nodeTail[np->state.layer] = np;
	}
	nodes[np->state.layer] = np;

	LoadIcon(np);

	ApplyGroups(np);

	SetDefaultCursor(np->window);
	ReparentClient(np, notOwner);
	PlaceClient(np, alreadyMapped);

	/* If one of these fails we are SOL, so who cares. */
	XSaveContext(display, np->window, clientContext, (void*)np);
	XSaveContext(display, np->parent, frameContext, (void*)np);

	if(np->state.status & STAT_MAPPED) {
		JXMapWindow(display, np->window);
		JXMapWindow(display, np->parent);
	}

	DrawBorder(np);

	AddClientToTaskBar(np);

	if(!alreadyMapped) {
		RaiseClient(np);
	}

	++clientCount;

	if(np->state.status & STAT_STICKY) {
		SetCardinalAtom(np->window, ATOM_NET_WM_DESKTOP, ~0UL);
	} else {
		SetCardinalAtom(np->window, ATOM_NET_WM_DESKTOP, np->state.desktop);
	}

	if(np->state.status & STAT_SHADED) {
		ShadeClient(np);
	}

	if(np->state.status & STAT_MINIMIZED) {
		np->state.status &= ~STAT_MINIMIZED;
		MinimizeClient(np);
	}

	if(np->state.status & STAT_MAXIMIZED) {
		np->state.status &= ~STAT_MAXIMIZED;
		MaximizeClient(np);
	}

	/* Make sure we're still in sync */
	WriteState(np);
	SendConfigureEvent(np);

	if(np->state.desktop != currentDesktop
		&& !(np->state.status & STAT_STICKY)) {
		HideClient(np);
	}

	ReadClientStrut(np);

	return np;

}

/****************************************************************************
 * Minimize a client window and all of its transients.
 ****************************************************************************/
void MinimizeClient(ClientNode *np) {

	Assert(np);

	if(focusModel == FOCUS_CLICK && np == activeClient) {
		FocusNextStacked(np);
	}

	MinimizeTransients(np);

	UpdateTaskBar();
	UpdatePager();

}

/****************************************************************************
 * Minimize all transients as well as the specified client.
 ****************************************************************************/
void MinimizeTransients(ClientNode *np) {
	ClientNode *tp;
	int x;

	Assert(np);

	if(activeClient == np) {
		activeClient = NULL;
		np->state.status &= ~STAT_ACTIVE;
	}

	if(np->state.status & STAT_SHADED) {
		UnshadeClient(np);
	}

	if(np->state.status & STAT_MAPPED) {
		JXUnmapWindow(display, np->window);
		JXUnmapWindow(display, np->parent);
	}
	np->state.status |= STAT_MINIMIZED;
	np->state.status &= ~STAT_MAPPED;
	WriteState(np);

	for(x = 0; x < LAYER_COUNT; x++) {
		for(tp = nodes[x]; tp; tp = tp->next) {
			if(tp->owner == np->window && (tp->state.status & STAT_MAPPED)
				&& !(tp->state.status & STAT_MINIMIZED)) {
				MinimizeTransients(tp);
			}
		}
	}
}

/****************************************************************************
 * Shade a client.
 ****************************************************************************/
void ShadeClient(ClientNode *np) {

	int bsize;

	Assert(np);

	if(!(np->state.border & BORDER_TITLE)) {
		return;
	}
	if(np->state.border & BORDER_OUTLINE) {
		bsize = borderWidth;
	} else {
		bsize = 0;
	}

	if(np->state.status & STAT_MAPPED) {
		JXUnmapWindow(display, np->window);
	}
	np->state.status |= STAT_SHADED;
	np->state.status &= ~STAT_MINIMIZED;
	np->state.status &= ~STAT_MAPPED;

	JXResizeWindow(display, np->parent, np->width + 2 * bsize,
		titleHeight + 2 * bsize);

	WriteState(np);

	if(np->state.status & STAT_SHAPE) {
		SetShape(np);
	}

}

/****************************************************************************
 * Unshade a client.
 ****************************************************************************/
void UnshadeClient(ClientNode *np) {

	int bsize;

	Assert(np);

	if(!(np->state.border & BORDER_TITLE)) {
		return;
	}
	if(np->state.border & BORDER_OUTLINE) {
		bsize = borderWidth;
	} else {
		bsize = 0;
	}

	if(np->state.status & STAT_SHADED) {
		JXMapWindow(display, np->window);
		np->state.status |= STAT_MAPPED;
		np->state.status &= ~STAT_SHADED;
	}

	JXResizeWindow(display, np->parent, np->width + 2 * bsize,
		np->height + titleHeight + 2 * bsize);

	WriteState(np);

	if(np->state.status & STAT_SHAPE) {
		SetShape(np);
	}

	RefocusClient();
	RestackClients();

}

/****************************************************************************
 * Set a client's state to withdrawn.
 ****************************************************************************/
void SetClientWithdrawn(ClientNode *np) {

	Assert(np);

	if(activeClient == np) {
		activeClient = NULL;
		np->state.status &= ~STAT_ACTIVE;
		FocusNextStacked(np);
	}

	if(np->state.status & STAT_MAPPED) {
		JXUnmapWindow(display, np->window);
		JXUnmapWindow(display, np->parent);
		WriteState(np);
	}

	np->state.status &= ~STAT_MAPPED;
	np->state.status &= ~STAT_MINIMIZED;

	UpdateTaskBar();
	UpdatePager();

}

/****************************************************************************
 ****************************************************************************/
void RestoreTransients(ClientNode *np) {

	ClientNode *tp;
	int x;

	Assert(np);

	if(!(np->state.status & STAT_MAPPED)) {
		JXMapWindow(display, np->window);
		JXMapWindow(display, np->parent);
	}
	np->state.status |= STAT_MAPPED;
	np->state.status &= ~STAT_MINIMIZED;

	WriteState(np);

	for(x = 0; x < LAYER_COUNT; x++) {
		for(tp = nodes[x]; tp; tp = tp->next) {
			if(tp->owner == np->window
				&& !(tp->state.status & STAT_MAPPED)
				&& (tp->state.status & STAT_MINIMIZED)) {
				RestoreTransients(tp);
			}
		}
	}

	RaiseClient(np);

}

/****************************************************************************
 ****************************************************************************/
void RestoreClient(ClientNode *np) {

	Assert(np);

	RestoreTransients(np);

	RestackClients();
	UpdateTaskBar();
	UpdatePager();

}

/****************************************************************************
 * Set the client layer. This will affect transients.
 ****************************************************************************/
void SetClientLayer(ClientNode *np, unsigned int layer) {

	ClientNode *tp, *next;
	int x;

	Assert(np);

	if(layer > LAYER_TOP) {
		Warning("Client %s requested an invalid layer: %d", np->name, layer);
		return;
	}

	if(np->state.layer != layer) {

		/* Loop through all clients so we get transients. */
		for(x = 0; x < LAYER_COUNT; x++) {
			tp = nodes[x];
			while(tp) {
				if(tp == np || tp->owner == np->window) {

					next = tp->next;

					/* Remove from the old node list */
					if(next) {
						next->prev = tp->prev;
					} else {
						nodeTail[tp->state.layer] = tp->prev;
					}
					if(tp->prev) {
						tp->prev->next = next;
					} else {
						nodes[tp->state.layer] = next;
					}

					/* Insert into the new node list */
					tp->prev = NULL;
					tp->next = nodes[layer];
					if(nodes[layer]) {
						nodes[layer]->prev = tp;
					} else {
						nodeTail[layer] = tp;
					}
					nodes[layer] = tp;

					/* Set the new layer */
					tp->state.layer = layer;
					SetCardinalAtom(tp->window, ATOM_WIN_LAYER, layer);

					/* Make sure we continue on the correct layer list. */
					tp = next;

				} else {
					tp = tp->next;
				}
			}
		}

		RestackClients();

	}

}

/****************************************************************************
 * Set a client's sticky status. This will update transients.
 ****************************************************************************/
void SetClientSticky(ClientNode *np, int isSticky) {
	ClientNode *tp;
	int old;
	int x;

	Assert(np);

	if(np->state.status & STAT_STICKY) {
		old = 1;
	} else {
		old = 0;
	}

	if(isSticky && !old) {
		for(x = 0; x < LAYER_COUNT; x++) {
			for(tp = nodes[x]; tp; tp = tp->next) {
				if(tp == np || tp->owner == np->window) {
					tp->state.status |= STAT_STICKY;
					SetCardinalAtom(tp->window, ATOM_NET_WM_DESKTOP, ~0UL);
					WriteState(tp);
				}
			}
		}
	} else if(!isSticky && old) {
		for(x = 0; x < LAYER_COUNT; x++) {
			for(tp = nodes[x]; tp; tp = tp->next) {
				if(tp == np || tp->owner == np->window) {
					tp->state.status &= ~STAT_STICKY;
					WriteState(tp);
				}
			}
		}
		SetClientDesktop(np, currentDesktop);
	}

}

/****************************************************************************
 * Set a client's desktop. This will update transients.
 ****************************************************************************/
void SetClientDesktop(ClientNode *np, unsigned int desktop) {
	ClientNode *tp;
	int x;

	Assert(np);

	if(desktop >= desktopCount) {
		return;
	}

	if(!(np->state.status & STAT_STICKY)) {
		for(x = 0; x < LAYER_COUNT; x++) {
			for(tp = nodes[x]; tp; tp = tp->next) {
				if(tp == np || tp->owner == np->window) {

					tp->state.desktop = desktop;

					if(desktop == currentDesktop) {
						ShowClient(tp);
					} else {
						HideClient(tp);
					}

					SetCardinalAtom(tp->window, ATOM_NET_WM_DESKTOP,
						tp->state.desktop);
				}
			}
		}
		UpdatePager();
		UpdateTaskBar();
	}

}

/****************************************************************************
 * Hide a client without unmapping. This will NOT update transients.
 ****************************************************************************/
void HideClient(ClientNode *np) {

	Assert(np);

	if(activeClient == np) {
		activeClient = NULL;
	}
	np->state.status |= STAT_HIDDEN;
	if(np->state.status & (STAT_MAPPED | STAT_SHADED)) {
		JXUnmapWindow(display, np->parent);
	}
}

/****************************************************************************
 * Show a hidden client. This will NOT update transients.
 ****************************************************************************/
void ShowClient(ClientNode *np) {

	Assert(np);

	if(np->state.status & STAT_HIDDEN) {
		np->state.status &= ~STAT_HIDDEN;
		if(np->state.status & (STAT_MAPPED | STAT_SHADED)) {
			JXMapWindow(display, np->parent);
			if(np->state.status & STAT_ACTIVE) {
				FocusClient(np);
			}
		}
	}
}

/****************************************************************************
 ****************************************************************************/
void MaximizeClient(ClientNode *np) {

	int north, west;

	Assert(np);

	if(np->state.status & STAT_SHADED) {
		UnshadeClient(np);
	}

	if(np->state.border & BORDER_OUTLINE) {
		west = borderWidth;
	} else {
		west = 0;
	}
	if(np->state.border & BORDER_TITLE) {
		north = west + titleHeight;
	} else {
		north = west;
	}

	if(np->state.status & STAT_MAXIMIZED) {
		np->x = np->oldx;
		np->y = np->oldy;
		np->width = np->oldWidth;
		np->height = np->oldHeight;
		np->state.status &= ~STAT_MAXIMIZED;
	} else {
		PlaceMaximizedClient(np);
	}

	JXMoveResizeWindow(display, np->parent,
		np->x - west, np->y - north,
		np->width + 2 * west,
		np->height + north + west);
	JXMoveResizeWindow(display, np->window, west,
		north, np->width, np->height);

	WriteState(np);
	SendConfigureEvent(np);

}

/****************************************************************************
 ****************************************************************************/
void FocusClient(ClientNode *np) {

	Assert(np);

	if(!(np->state.status & STAT_MAPPED)) {
		return;
	}
	if(np->state.status & STAT_HIDDEN) {
		return;
	}

	if(activeClient != np) {
		if(activeClient) {
			activeClient->state.status &= ~STAT_ACTIVE;
			DrawBorder(activeClient);
		}
		np->state.status |= STAT_ACTIVE;
		activeClient = np;

		if(!(np->state.status & STAT_SHADED)) {
			UpdateClientColormap(np);
			if(np->protocols & PROT_TAKE_FOCUS) {
				SendClientMessage(np->window, ATOM_WM_PROTOCOLS,
					ATOM_WM_TAKE_FOCUS);
			}
			SetWindowAtom(rootWindow, ATOM_NET_ACTIVE_WINDOW, np->window);
		}

		DrawBorder(np);
		UpdatePager();
		UpdateTaskBar();

	}

	if(np->state.status & STAT_MAPPED && !(np->state.status & STAT_HIDDEN)) {
		JXSetInputFocus(display, np->window, RevertToPointerRoot, CurrentTime);
	} else {
		JXSetInputFocus(display, rootWindow, RevertToPointerRoot, CurrentTime);
	}

}

/****************************************************************************
 ****************************************************************************/
void FocusNextStacked(ClientNode *np) {

	int x;
	ClientNode *tp;

	Assert(np);

	for(tp = np->next; tp; tp = tp->next) {
		if((tp->state.status & STAT_MAPPED)
			&& !(tp->state.status & STAT_HIDDEN)) {
			FocusClient(tp);
			return;
		}
	}
	for(x = np->state.layer - 1; x >= LAYER_BOTTOM; x--) {
		for(tp = nodes[x]; tp; tp = tp->next) {
			if((tp->state.status & STAT_MAPPED)
				&& !(tp->state.status & STAT_HIDDEN)) {
				FocusClient(tp);
				return;
			}
		}
	}

}

/****************************************************************************
 * Refocus the active client, if existent.
 ****************************************************************************/
void RefocusClient() {
	if(activeClient) {
		FocusClient(activeClient);
	}
}

/****************************************************************************
 ****************************************************************************/
void DeleteClient(ClientNode *np) {

	Assert(np);

	ReadWMProtocols(np);
	if(np->protocols & PROT_DELETE) {
		SendClientMessage(np->window, ATOM_WM_PROTOCOLS,
			ATOM_WM_DELETE_WINDOW);
	} else {
		KillClient(np);
	}

}

/****************************************************************************
 ****************************************************************************/
void KillClientHandler(ClientNode *np) {

	Assert(np);

	if(np == activeClient) {
		FocusNextStacked(np);
	}

	JXGrabServer(display);
	JXSync(display, False);

	JXKillClient(display, np->window);

	JXSync(display, True);
	JXUngrabServer(display);

	np->window = None;
	RemoveClient(np);

}

/****************************************************************************
 ****************************************************************************/
void KillClient(ClientNode *np) {

	Assert(np);

	ShowConfirmDialog(np, KillClientHandler,
		"Kill this window?",
		"This may cause data to be lost!",
		NULL);
}

/****************************************************************************
 * Raise the client. This will affect transients.
 ****************************************************************************/
void RaiseClient(ClientNode *np) {
	ClientNode *tp, *next;
	int x;

	Assert(np);

	if(np->state.layer > LAYER_BOTTOM && nodes[np->state.layer] != np) {

		/* Raise the window */
		Assert(np->prev);
		np->prev->next = np->next;
		if(np->next) {
			np->next->prev = np->prev;
		} else {
			nodeTail[np->state.layer] = np->prev;
		}
		np->next = nodes[np->state.layer];
		nodes[np->state.layer]->prev = np;
		np->prev = NULL;
		nodes[np->state.layer] = np;

		/* Place any transient windows on top of the owner */
		for(x = 0; x < LAYER_COUNT; x++) {
			for(tp = nodes[x]; tp; tp = tp->next) {
				if(tp->owner == np->window && tp->prev) {

					next = tp->next;

					tp->prev->next = tp->next;
					if(tp->next) {
						tp->next->prev = tp->prev;
					} else {
						nodeTail[tp->state.layer] = tp->prev;
					}
					tp->next = nodes[tp->state.layer];
					nodes[tp->state.layer]->prev = tp;
					tp->prev = NULL;
					nodes[tp->state.layer] = tp;

					tp = next;

				}

				/* tp will be tp->next if the above code is executed. */
				/* Thus, if it is NULL, we are done with this layer. */
				if(!tp) {
					break;
				}
			}
		}

		RestackClients();
	}
}

/****************************************************************************
 ****************************************************************************/
void RestackClients() {

	TrayType *tp;
	ClientNode *np;
	unsigned int layer, index;
	int trayCount;

	if(!stack) {
		return;
	}

	/* Determine how many tray windows exist. */
	trayCount = 0;
	for(tp = GetTrays(); tp; tp = tp->next) {
		++trayCount;
	}

	/* Make sure an appropriate amount of memory is allocated. */
	if(clientCount + trayCount >= stackSize
		|| clientCount + trayCount < stackSize - STACK_BLOCK_SIZE) {

		stackSize = (clientCount + trayCount) + STACK_BLOCK_SIZE
			- ((clientCount + trayCount) % STACK_BLOCK_SIZE);
		stack = Reallocate(stack, sizeof(Window) * stackSize);

	}

	/* Prepare the stacking array. */
	index = 0;
	layer = LAYER_TOP;
	for(;;) {

		for(np = nodes[layer]; np; np = np->next) {
			if((np->state.status & STAT_MAPPED)
				&& !(np->state.status & STAT_HIDDEN)) {
				stack[index++] = np->parent;
			}
		}

		for(tp = GetTrays(); tp; tp = tp->next) {
			if(layer == tp->layer) {
				stack[index++] = tp->window;
			}
		}

		if(layer == 0) {
			break;
		}
		--layer;

	}

	JXRestackWindows(display, stack, index);

	UpdateNetClientList();

}

/****************************************************************************
 ****************************************************************************/
void SendClientMessage(Window w, AtomType type, AtomType message) {

	XEvent event;
	int status;

	memset(&event, 0, sizeof(event));
	event.xclient.type = ClientMessage;
	event.xclient.window = w;
	event.xclient.message_type = atoms[type];
	event.xclient.format = 32;
	event.xclient.data.l[0] = atoms[message];
	event.xclient.data.l[1] = CurrentTime;

	status = JXSendEvent(display, w, False, 0, &event);
	if(status == False) {
		Debug("SendClientMessage failed");
	}
}

/****************************************************************************
 ****************************************************************************/
#ifdef USE_SHAPE
void SetShape(ClientNode *np) {

	XRectangle rect[4];
	int north, west;

	Assert(np);

	np->state.status |= STAT_SHAPE;

	if(np->state.border & BORDER_OUTLINE) {
		west = borderWidth;
	} else {
		west = 0;
	}
	if(np->state.border & BORDER_TITLE) {
		north = west + titleHeight;
	} else {
		north = west;
	}

	if(np->state.status & STAT_SHADED) {

		rect[0].x = 0;
		rect[0].y = 0;
		rect[0].width = np->width + west * 2;
		rect[0].height = west * 2 + north;

		JXShapeCombineRectangles(display, np->parent, ShapeBounding,
			0, 0, rect, 1, ShapeSet, Unsorted);

		return;
	}

	JXShapeCombineShape(display, np->parent, ShapeBounding, west, north,
		np->window, ShapeBounding, ShapeSet);

	if(north > 0) {

		/* Top */
		rect[0].x = 0;
		rect[0].y = 0;
		rect[0].width = np->width + west * 2;
		rect[0].height = north;

		/* Left */
		rect[1].x = 0;
		rect[1].y = 0;
		rect[1].width = west;
		rect[1].height = np->height + west + north;

		/* Right */
		rect[2].x = np->width + west;
		rect[2].y = 0;
		rect[2].width = west;
		rect[2].height = np->height + west + north;

		/* Bottom */
		rect[3].x = 0;
		rect[3].y = np->height + north;
		rect[3].width = np->width + west * 2;
		rect[3].height = west;

		JXShapeCombineRectangles(display, np->parent, ShapeBounding,
			0, 0, rect, 4, ShapeUnion, Unsorted);

	}
}
#endif /* USE_SHAPE */

/****************************************************************************
 ****************************************************************************/
void RemoveClient(ClientNode *np) {

	ColormapNode *cp;

	Assert(np);

	JXGrabServer(display);

	/* Remove this client from the client list */
	if(np->next) {
		np->next->prev = np->prev;
	} else {
		nodeTail[np->state.layer] = np->prev;
	}
	if(np->prev) {
		np->prev->next = np->next;
	} else {
		nodes[np->state.layer] = np->next;
	}
	--clientCount;
	XDeleteContext(display, np->window, clientContext);
	XDeleteContext(display, np->parent, frameContext);

	/* Make sure this client isn't active */
	if(activeClient == np && !shouldExit) {
		FocusNextStacked(np);
	}
	if(activeClient == np) {
		SetWindowAtom(rootWindow, ATOM_NET_ACTIVE_WINDOW, None);
		activeClient = NULL;
	}

	/* If the window manager is exiting (ie, not the client), then
	 * reparent etc. */
	if(shouldExit && !(np->state.status & STAT_WMDIALOG)) {
		if(np->state.status & STAT_MAXIMIZED) {
			np->x = np->oldx;
			np->y = np->oldy;
			np->width = np->oldWidth;
			np->height = np->oldHeight;
			JXMoveResizeWindow(display, np->window,
				np->x, np->y, np->width, np->height);
		}
		GravitateClient(np, 1);
		if(!(np->state.status & STAT_MAPPED)
			&& (np->state.status & (STAT_MINIMIZED | STAT_SHADED))) {
			JXMapWindow(display, np->window);
		}
		JXUngrabButton(display, AnyButton, AnyModifier, np->window);
		JXReparentWindow(display, np->window, rootWindow, np->x, np->y);
		JXRemoveFromSaveSet(display, np->window);
	}

	/* Destroy the parent */
	if(np->parent) {
		JXFreeGC(display, np->parentGC);
		JXDestroyWindow(display, np->parent);
	}

	if(np->name) {
		JXFree(np->name);
	}
	if(np->instanceName) {
		JXFree(np->instanceName);
	}
	if(np->className) {
		JXFree(np->className);
	}

	RemoveClientFromTaskBar(np);
	RemoveClientStrut(np);
	UpdatePager();

	while(np->colormaps) {
		cp = np->colormaps->next;
		Release(np->colormaps);
		np->colormaps = cp;
	}

	DestroyIcon(np->icon);

	Release(np);

	JXUngrabServer(display);

}

/****************************************************************************
 ****************************************************************************/
ClientNode *GetActiveClient() {
	return activeClient;
}

/****************************************************************************
 ****************************************************************************/
ClientNode *FindClientByWindow(Window w) {
	ClientNode *np;

	if(!XFindContext(display, w, clientContext, (void*)&np)) {
		return np;
	} else {
		return FindClientByParent(w);
	}

}

/****************************************************************************
 ****************************************************************************/
ClientNode *FindClientByParent(Window p) {
	ClientNode *np;

	if(!XFindContext(display, p, frameContext, (void*)&np)) {
		return np;
	} else {
		return NULL;
	}
}

/****************************************************************************
 ****************************************************************************/
void ReparentClient(ClientNode *np, int notOwner) {

	XSetWindowAttributes attr;
	int attrMask;
	int x, y, width, height;

	Assert(np);

	if(notOwner) {
		JXAddToSaveSet(display, np->window);

		attr.event_mask = EnterWindowMask | ColormapChangeMask
			| PropertyChangeMask | StructureNotifyMask;
		attr.do_not_propagate_mask = NoEventMask;
		XChangeWindowAttributes(display, np->window,
			CWEventMask | CWDontPropagate, &attr);

	}
	JXGrabButton(display, AnyButton, AnyModifier, np->window,
		True, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
	GrabKeys(np);

	attrMask = 0;

	attrMask |= CWOverrideRedirect;
	attr.override_redirect = True;

	attrMask |= CWEventMask;
	attr.event_mask
		= ButtonPressMask
		| ButtonReleaseMask
		| ExposureMask
		| PointerMotionMask
		| SubstructureRedirectMask
		| SubstructureNotifyMask
		| EnterWindowMask
		| LeaveWindowMask
		| KeyPressMask;

	attrMask |= CWDontPropagate;
	attr.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask;

	attrMask |= CWBackPixel;
	attr.background_pixel = colors[COLOR_BORDER_BG];

	x = np->x;
	y = np->y;
	width = np->width;
	height = np->height;
	if(np->state.border & BORDER_OUTLINE) {
		x -= borderWidth;
		y -= borderWidth;
		width += borderWidth * 2;
		height += borderWidth * 2;
	}
	if(np->state.border & BORDER_TITLE) {
		y -= titleHeight;
		height += titleHeight;
	}

	np->parent = JXCreateWindow(display, rootWindow,
		x, y, width, height, 0, rootDepth, InputOutput,
		rootVisual, attrMask, &attr);
	np->parentGC = JXCreateGC(display, np->parent, 0, NULL);

	attrMask = CWDontPropagate;
	attr.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask;

	JXChangeWindowAttributes(display, np->window, attrMask, &attr);

	JXSetWindowBorderWidth(display, np->window, 0);
	if((np->state.border & BORDER_OUTLINE)
		&& (np->state.border & BORDER_TITLE)) {
		JXReparentWindow(display, np->window, np->parent,
			borderWidth, borderWidth + titleHeight);
	} else if(np->state.border & BORDER_OUTLINE) {
		JXReparentWindow(display, np->window, np->parent,
			borderWidth, borderWidth);
	} else if(np->state.border & BORDER_TITLE) {
		JXReparentWindow(display, np->window, np->parent,
			0, titleHeight);
	} else {
		JXReparentWindow(display, np->window, np->parent, 0, 0);
	}

#ifdef USE_SHAPE
	if(haveShape) {
		JXShapeSelectInput(display, np->window, ShapeNotifyMask);
		CheckShape(np);
	}
#endif

}

/****************************************************************************
 ****************************************************************************/
#ifdef USE_SHAPE
void CheckShape(ClientNode *np) {

	int xb, yb;
	int xc, yc;
	unsigned int wb, hb;
	unsigned int wc, hc;
	Bool boundingShaped, clipShaped;

	JXShapeQueryExtents(display, np->window, &boundingShaped,
		&xb, &yb, &wb, &hb, &clipShaped, &xc, &yc, &wc, &hc);

	if(boundingShaped == True) {
		SetShape(np);
	}

}
#endif

/****************************************************************************
 ****************************************************************************/
void SendConfigureEvent(ClientNode *np) {
	XConfigureEvent event;

	Assert(np);

	event.type = ConfigureNotify;
	event.event = np->window;
	event.window = np->window;
	event.x = np->x;
	event.y = np->y;
	event.width = np->width;
	event.height = np->height;
	event.border_width = 0;
	event.above = None;
	event.override_redirect = False;

	JXSendEvent(display, np->window, False, StructureNotifyMask,
		(XEvent*)&event);
}

/****************************************************************************
 * A call to this function indicates that the colormap(s) for the given
 * client changed. This will change the active colormap(s) if the given
 * client is active.
 ****************************************************************************/
void UpdateClientColormap(ClientNode *np) {

	XWindowAttributes attr;
	ColormapNode *cp;
	int wasInstalled;

	Assert(np);

	cp = np->colormaps;

	if(np == activeClient) {

		wasInstalled = 0;
		cp = np->colormaps;
		while(cp) {
			if(JXGetWindowAttributes(display, cp->window, &attr)) {
				if(attr.colormap != None) {
					if(attr.colormap == np->cmap) {
						wasInstalled = 1;
					}
					JXInstallColormap(display, attr.colormap);
				}
			}
			cp = cp->next;
		}

		if(!wasInstalled && np->cmap != None) {
			JXInstallColormap(display, np->cmap);
		}

	}

}

