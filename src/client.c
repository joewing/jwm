/****************************************************************************
 * Functions to handle client windows.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"

static const int STACK_BLOCK_SIZE = 8;

ClientNode *nodes[LAYER_COUNT];
ClientNode *nodeTail[LAYER_COUNT];

static ClientNode *activeClient;

static Window *stack;
static int stackSize;
static int clientCount;

static unsigned int cascadeOffset;
static unsigned int cascadeStart;
static unsigned int cascadeStop;

static void LoadFocus();
static void ReparentClient(ClientNode *np, int notOwner);
 
static void SendClientMessage(ClientNode *np, AtomType type,
	AtomType message);
static void GetBorderOffsets(ClientNode *np, int *north, int *west);
static void PlaceWindow(ClientNode *np, int alreadyMapped);
static void Gravitate(ClientNode *np, int negate);
static void UpdateState(ClientNode *np);
static void MinimizeTransients(ClientNode *np);
static void CheckShape(ClientNode *np);

static void RestoreSingleClient(ClientNode *np);

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
	int x;

	JXSync(display, False);
	JXGrabServer(display);

	stackSize = STACK_BLOCK_SIZE;
	stack = Allocate(sizeof(Window) * stackSize);
	clientCount = 0;
	activeClient = NULL;
	currentDesktop = 0;

	cascadeStart = titleHeight + borderWidth;
	cascadeStop = rootHeight / 3;
	cascadeOffset = cascadeStart;

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
	XWMHints *wmhints;
	ClientNode *np;

	Assert(w != None);

	if(JXGetWindowAttributes(display, w, &attr) == 0) {
		return NULL;
	}

	if(attr.override_redirect == True && notOwner) {
		return NULL;
	}
	if(attr.class == InputOnly) {
		return NULL;
	}

	np = Allocate(sizeof(ClientNode));
	memset(np, 0, sizeof(ClientNode));

	np->window = w;
	np->owner = None;
	np->desktop = currentDesktop;
	np->controller = NULL;
	np->name = NULL;
	np->colormaps = NULL;

	np->x = attr.x;
	np->y = attr.y;
	np->width = attr.width;
	np->height = attr.height;
	np->cmap = attr.colormap;
	np->colormaps = NULL;
	np->statusFlags = STAT_NONE;
	np->layer = LAYER_NORMAL;

	if(notOwner) {
		np->borderFlags = BORDER_OUTLINE | BORDER_TITLE
			| BORDER_MIN | BORDER_MAX | BORDER_CLOSE
			| BORDER_MOVE | BORDER_RESIZE;
	} else {
		np->borderFlags = BORDER_OUTLINE | BORDER_TITLE | BORDER_MOVE;
		np->statusFlags |= STAT_WMDIALOG | STAT_STICKY;
	}
	np->borderAction = BA_NONE;

	ReadMotifHints(np);
	ReadClientProtocols(np);

	/* We now know the layer, so insert */
	np->prev = NULL;
	np->next = nodes[np->layer];
	if(np->next) {
		np->next->prev = np;
	} else {
		nodeTail[np->layer] = np;
	}
	nodes[np->layer] = np;

	wmhints = XGetWMHints(display, np->window);
	if(wmhints) {
		switch(wmhints->flags & StateHint) {
		case IconicState:
			np->statusFlags |= STAT_MINIMIZED;
			break;
		default:
			if(!(np->statusFlags & STAT_MINIMIZED)) {
				np->statusFlags |= STAT_MAPPED;
			}
			break;
		}
		JXFree(wmhints);
	} else {
		np->statusFlags |= STAT_MAPPED;
	}

	LoadIcon(np);

	ApplyGroups(np);

	SetDefaultCursor(np->window);
	ReparentClient(np, notOwner);
	PlaceWindow(np, alreadyMapped);

	/* If one of these fails we are SOL, so who cares. */
	XSaveContext(display, np->window, clientContext, (void*)np);
	XSaveContext(display, np->parent, frameContext, (void*)np);

	if(np->statusFlags & STAT_MAPPED) {
		JXMapWindow(display, np->window);
		JXMapWindow(display, np->parent);
	}

	DrawBorder(np);

	AddClientToTray(np);

	if(!alreadyMapped) {
		RaiseClient(np);
	}

	++clientCount;


	if(np->statusFlags & STAT_STICKY) {
		SetCardinalAtom(np->window, ATOM_NET_WM_DESKTOP, 0xFFFFFFFF);
	} else {
		SetCardinalAtom(np->window, ATOM_NET_WM_DESKTOP, np->desktop);
	}

	if(np->statusFlags & STAT_SHADED) {
		ShadeClient(np);
	}

	/* Make sure we're still in sync */
	UpdateState(np);
	SendConfigureEvent(np);
	WriteWinState(np);

	if(np->desktop != currentDesktop && !(np->statusFlags & STAT_STICKY)) {
		HideClient(np);
	}

	return np;

}

/****************************************************************************
 * Place a window on the screen.
 ****************************************************************************/
void PlaceWindow(ClientNode *np, int alreadyMapped) {

	int north, west;
	int index;
	int width, height;
	int x, y;

	Assert(np);

	GetBorderOffsets(np, &north, &west);

	if(!(np->sizeFlags & PPosition) && !alreadyMapped) {

		index = GetMouseScreen();
		width = GetScreenWidth(index);
		height = GetScreenHeight(index);
		x = GetScreenX(index);
		y = GetScreenY(index);

		if(np->width + cascadeOffset + west > width) {
			if(np->width + west * 2 < width) {
				np->x = west + width / 2 - np->width / 2 + x;
			} else {
				np->x = west + x;
			}
		} else {
			np->x = west + cascadeOffset + x;
		}

		if(np->height + cascadeOffset + north > height - trayHeight) {
			if(np->height + north + west < height) {
				np->y = north + width / 2 - np->width / 2 + y;
			} else {
				np->y = north + y;
			}
		} else {
			np->y = north + cascadeOffset + y;
		}

		if(cascadeOffset >= cascadeStop) {
			cascadeOffset = cascadeStart;
		} else {
			cascadeOffset += titleHeight + borderWidth;
		}

	} else {
		Gravitate(np, 0);
	}

	JXMoveWindow(display, np->parent, np->x - west, np->y - north);

}

/****************************************************************************
 * Get the offsets of the child window with respect to the parent.
 ****************************************************************************/
void GetBorderOffsets(ClientNode *np, int *north, int *west) {

	Assert(np);
	Assert(north);
	Assert(west);

	*north = 0;
	*west = 0;

	if(np->borderFlags & BORDER_OUTLINE) {
		*west = borderWidth;
		if(np->borderFlags & BORDER_TITLE) {
			*north = titleSize;
		} else {
			*north = borderWidth;
		}
	}

}

/****************************************************************************
 * Move the window in the specified direction for reparenting.
 ****************************************************************************/
void Gravitate(ClientNode *np, int negate) {
	int north, south, west;
	int northDelta, westDelta;

	Assert(np);

	GetBorderOffsets(np, &north, &west);
	if(north) {
		south = borderWidth;
	} else {
		south = 0;
	}

	northDelta = 0;
	westDelta = 0;
	switch(np->gravity) {
	case NorthWestGravity:
		northDelta = -north;
		westDelta = -west;
		break;
	case NorthGravity:
		northDelta = -north;
		break;
	case NorthEastGravity:
		northDelta = -north;
		westDelta = west;
		break;
	case WestGravity:
		westDelta = -west;
		break;
	case CenterGravity:
		northDelta = (north + south) / 2;
		westDelta = west;
		break;
	case EastGravity:
		westDelta = west;
		break;
	case SouthWestGravity:
		northDelta = south;
		westDelta = -west;
		break;
	case SouthGravity:
		northDelta = south;
		break;
	case SouthEastGravity:
		northDelta = south;
		westDelta = west;
		break;
	default: /* Static */
		break;
	}

	if(negate) {
		np->x += westDelta;
		np->y += northDelta;
	} else {
		np->x -= westDelta;
		np->y -= northDelta;
	}

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
	DrawTray();

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
		np->statusFlags &= ~STAT_ACTIVE;
	}

	if(np->statusFlags & STAT_SHADED) {
		UnshadeClient(np);
	}

	if(np->statusFlags & STAT_MAPPED) {
		JXUnmapWindow(display, np->window);
		JXUnmapWindow(display, np->parent);
	}
	np->statusFlags |= STAT_MINIMIZED;
	np->statusFlags &= ~STAT_MAPPED;
	np->statusFlags &= ~STAT_WITHDRAWN;
	UpdateState(np);

	for(x = 0; x < LAYER_COUNT; x++) {
		for(tp = nodes[x]; tp; tp = tp->next) {
			if(tp->owner == np->window && (tp->statusFlags & STAT_MAPPED)
				&& !(tp->statusFlags & STAT_MINIMIZED)) {
				MinimizeTransients(tp);
			}
		}
	}
}

/****************************************************************************
 * Shade a client.
 ****************************************************************************/
void ShadeClient(ClientNode *np) {

	Assert(np);

	if(!(np->borderFlags & BORDER_OUTLINE)
		|| !(np->borderFlags & BORDER_TITLE)) {
		return;
	}

	if(np->statusFlags & STAT_MAPPED) {
		JXUnmapWindow(display, np->window);
	}
	np->statusFlags |= STAT_SHADED;
	np->statusFlags &= ~STAT_MINIMIZED;
	np->statusFlags &= ~STAT_MAPPED;
	np->statusFlags &= ~STAT_WITHDRAWN;

	JXResizeWindow(display, np->parent, np->width + 2 * borderWidth,
		titleSize + borderWidth);

	WriteWinState(np);

}

/****************************************************************************
 * Unshade a client.
 ****************************************************************************/
void UnshadeClient(ClientNode *np) {

	Assert(np);

	if(!(np->borderFlags & BORDER_OUTLINE)
		|| !(np->borderFlags & BORDER_TITLE)) {
		return;
	}

	if(np->statusFlags & STAT_SHADED) {
		JXMapWindow(display, np->window);
		np->statusFlags |= STAT_MAPPED;
		np->statusFlags &= ~STAT_SHADED;
	}
	np->statusFlags &= ~STAT_WITHDRAWN;

	JXResizeWindow(display, np->parent, np->width + 2 * borderWidth,
		np->height + titleSize + borderWidth);

	WriteWinState(np);

	RefocusClient();

}

/****************************************************************************
 * Set a client's state to withdrawn.
 ****************************************************************************/
void SetClientWithdrawn(ClientNode *np, int isWithdrawn) {

	Assert(np);

	if(isWithdrawn) {

		if(activeClient == np) {
			activeClient = NULL;
			np->statusFlags &= ~STAT_ACTIVE;
		}
		np->statusFlags |= STAT_WITHDRAWN;
		if(np->statusFlags & STAT_MAPPED) {
			JXUnmapWindow(display, np->window);
			JXUnmapWindow(display, np->parent);
			UpdateState(np);
		}

	} else {
		np->statusFlags &= ~STAT_WITHDRAWN;
		if(np->statusFlags & STAT_MAPPED) {
			JXMapWindow(display, np->window);
			JXMapWindow(display, np->parent);
			UpdateState(np);
		}
	}

	DrawTray();

}

/****************************************************************************
 ****************************************************************************/
void RestoreSingleClient(ClientNode *np) {

	Assert(np);

	if(np->statusFlags & STAT_MINIMIZED) {
		JXMapWindow(display, np->window);
		JXMapWindow(display, np->parent);
		np->statusFlags |= STAT_MAPPED;
		np->statusFlags &= ~STAT_MINIMIZED;
	}
	RaiseClient(np);
	UpdateState(np);
	WriteWinState(np);
}

/****************************************************************************
 ****************************************************************************/
void RestoreClient(ClientNode *np) {
	ClientNode *tp;
	int x;

	Assert(np);

	RestoreSingleClient(np);

	for(x = 0; x < LAYER_COUNT; x++) {
		for(tp = nodes[x]; tp; tp = tp->next) {
			if(tp->owner == np->window) {
				RestoreSingleClient(tp);
			}
		}
	}

	RestackClients();

}

/****************************************************************************
 * Set the client layer. This will affect transients.
 ****************************************************************************/
void SetClientLayer(ClientNode *np, int layer) {
	ClientNode *tp, *next;
	int x;

	Assert(np);

	if(layer < LAYER_BOTTOM || layer > LAYER_TOP) {
		return;
	}

	if(np->layer != layer) {

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
						nodeTail[tp->layer] = tp->prev;
					}
					if(tp->prev) {
						tp->prev->next = next;
					} else {
						nodes[tp->layer] = next;
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
					tp->layer = layer;
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

	if(np->statusFlags & STAT_STICKY) {
		old = 1;
	} else {
		old = 0;
	}

	if(isSticky && !old) {
		for(x = 0; x < LAYER_COUNT; x++) {
			for(tp = nodes[x]; tp; tp = tp->next) {
				if(tp == np || tp->owner == np->window) {
					tp->statusFlags |= STAT_STICKY;
					SetCardinalAtom(tp->window, ATOM_NET_WM_DESKTOP, 0xFFFFFFFF);
					WriteWinState(np);
				}
			}
		}
	} else if(!isSticky && old) {
		for(x = 0; x < LAYER_COUNT; x++) {
			for(tp = nodes[x]; tp; tp = tp->next) {
				if(tp == np || tp->owner == np->window) {
					tp->statusFlags &= ~STAT_STICKY;
					WriteWinState(tp);
				}
			}
		}
		SetClientDesktop(np, currentDesktop);
	}

}

/****************************************************************************
 * Set a client's desktop. This will update transients.
 ****************************************************************************/
void SetClientDesktop(ClientNode *np, int desktop) {
	ClientNode *tp;
	int x;

	Assert(np);

	if(desktop < 0 || desktop >= desktopCount) {
		return;
	}

	if(!(np->statusFlags & STAT_STICKY)) {
		for(x = 0; x < LAYER_COUNT; x++) {
			for(tp = nodes[x]; tp; tp = tp->next) {
				if(tp == np || tp->owner == np->window) {

					tp->desktop = desktop;

					if(desktop == currentDesktop) {
						ShowClient(tp);
					} else {
						HideClient(tp);
					}

					SetCardinalAtom(tp->window, ATOM_NET_WM_DESKTOP, tp->desktop);
				}
			}
		}
		DrawTray();
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
	np->statusFlags |= STAT_HIDDEN;
	if(np->statusFlags & (STAT_MAPPED | STAT_SHADED)) {
		JXUnmapWindow(display, np->parent);
	}
}

/****************************************************************************
 * Show a hidden client. This will NOT update transients.
 ****************************************************************************/
void ShowClient(ClientNode *np) {

	Assert(np);

	if(np->statusFlags & STAT_HIDDEN) {
		np->statusFlags &= ~STAT_HIDDEN;
		if(np->statusFlags & (STAT_MAPPED | STAT_SHADED)) {
			JXMapWindow(display, np->parent);
			if(np->statusFlags & STAT_ACTIVE) {
				FocusClient(np);
			}
		}
	}
}

/****************************************************************************
 ****************************************************************************/
void MaximizeClient(ClientNode *np) {
	int north, west;
	int screen;

	Assert(np);

	if(np->statusFlags & STAT_SHADED) {
		UnshadeClient(np);
	}

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

	if(np->statusFlags & STAT_MAXIMIZED) {
		np->x = np->oldx;
		np->y = np->oldy;
		np->width = np->oldWidth;
		np->height = np->oldHeight;
		np->statusFlags &= ~STAT_MAXIMIZED;
	} else {
		np->oldx = np->x;
		np->oldy = np->y;
		np->oldWidth = np->width;
		np->oldHeight = np->height;

		screen = GetCurrentScreen(np->x, np->y);

		np->x = GetScreenX(screen) + west;
		np->y = GetScreenY(screen) + north;
		np->width = GetScreenWidth(screen) - 2 * borderWidth;
		np->height = GetScreenHeight(screen) - titleSize - borderWidth;
		if(!autoHideTray) {
			np->height -= trayHeight;
		}

		if(np->width > np->maxWidth) {
			np->width = np->maxWidth;
		}
		if(np->height > np->maxHeight) {
			np->height = np->maxHeight;
		}

		if(np->sizeFlags & PAspect) {
			if((float)np->width / np->height
				< (float)np->aspect.minx / np->aspect.miny) {
				np->height = np->width * np->aspect.miny / np->aspect.minx;
			}
			if((float)np->width / np->height
				> (float)np->aspect.maxx / np->aspect.maxy) {
				np->width = np->height * np->aspect.maxx / np->aspect.maxy;
			}
		}

		np->width -= np->width % np->xinc;
		np->height -= np->height % np->yinc;

		np->statusFlags |= STAT_MAXIMIZED;
	}

	JXMoveResizeWindow(display, np->parent,
		np->x - west, np->y - north,
		np->width + 2 * west,
		np->height + north + west);
	JXMoveResizeWindow(display, np->window, west,
		north, np->width, np->height);

	SendConfigureEvent(np);
	WriteWinState(np);

}

/****************************************************************************
 ****************************************************************************/
void FocusClient(ClientNode *np) {

	Assert(np);

	if(np->statusFlags & (STAT_MINIMIZED | STAT_WITHDRAWN | STAT_HIDDEN)) {
		return;
	}

	if(activeClient != np) {
		if(activeClient) {
			activeClient->statusFlags &= ~STAT_ACTIVE;
			DrawBorder(activeClient);
		}
		np->statusFlags |= STAT_ACTIVE;
		activeClient = np;

		if(!(np->statusFlags & STAT_SHADED)) {
			UpdateClientColormap(np);
			if(np->protocols & PROT_TAKE_FOCUS) {
				SendClientMessage(np, ATOM_WM_PROTOCOLS, ATOM_WM_TAKE_FOCUS);
			}
			SetWindowAtom(rootWindow, ATOM_NET_ACTIVE_WINDOW, np->window);
		}

		DrawBorder(np);
		DrawTray();

	}

	if(np->statusFlags & STAT_MAPPED && !(np->statusFlags & STAT_HIDDEN)) {
		JXSetInputFocus(display, np->window, RevertToPointerRoot, CurrentTime);
	} else {
		JXSetInputFocus(display, rootWindow, RevertToPointerRoot, CurrentTime);
	}

}

/****************************************************************************
 ****************************************************************************/
void FocusNextStacked(ClientNode *np) {

	const int FLAGS = STAT_MINIMIZED | STAT_WITHDRAWN | STAT_HIDDEN;
	int x;
	ClientNode *tp;

	Assert(np);

	for(tp = np->next; tp; tp = tp->next) {
		if(!(tp->statusFlags & FLAGS)) {
			FocusClient(tp);
			return;
		}
	}
	for(x = np->layer - 1; x >= LAYER_BOTTOM; x--) {
		for(tp = nodes[x]; tp; tp = tp->next) {
			if(!(tp->statusFlags & FLAGS)) {
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
		SendClientMessage(np, ATOM_WM_PROTOCOLS, ATOM_WM_DELETE_WINDOW);
	} else {
		KillClient(np);
	}
}

/****************************************************************************
 ****************************************************************************/
void KillClientHandler(ClientNode *np) {

	Assert(np);

	if(np == activeClient) {
		activeClient = NULL;
	}

	JXKillClient(display, np->window);
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

	if(np->layer > LAYER_BOTTOM && nodes[np->layer] != np) {

		/* Raise the window */
		Assert(np->prev);
		np->prev->next = np->next;
		if(np->next) {
			np->next->prev = np->prev;
		} else {
			nodeTail[np->layer] = np->prev;
		}
		np->next = nodes[np->layer];
		nodes[np->layer]->prev = np;
		np->prev = NULL;
		nodes[np->layer] = np;

		/* Place any transient windows on top of the owner */
		for(x = 0; x < LAYER_COUNT; x++) {
			for(tp = nodes[x]; tp; tp = tp->next) {
				if(tp->owner == np->window && tp->prev) {

					next = tp->next;

					tp->prev->next = tp->next;
					if(tp->next) {
						tp->next->prev = tp->prev;
					} else {
						nodeTail[tp->layer] = tp->prev;
					}
					tp->next = nodes[tp->layer];
					nodes[tp->layer]->prev = tp;
					tp->prev = NULL;
					nodes[tp->layer] = tp;

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
	ClientNode *np;
	int layer, index;

	if(!stack) {
		return;
	}

	/* Make sure an appropriate amount of memory is allocated. */
	if(clientCount + 1 >= stackSize
		|| clientCount + 1 < stackSize - STACK_BLOCK_SIZE) {

		stackSize = (clientCount + 1) + STACK_BLOCK_SIZE
			- ((clientCount + 1) % STACK_BLOCK_SIZE);
		stack = Reallocate(stack, sizeof(Window) * stackSize);

	}

	/* Prepare the stacking array. */
	index = 0;
	for(layer = LAYER_TOP; layer >= LAYER_BOTTOM; layer--) {
		for(np = nodes[layer]; np; np = np->next) {
			if(!(np->statusFlags
				& (STAT_MINIMIZED | STAT_HIDDEN | STAT_WITHDRAWN))) {
				stack[index++] = np->parent;
			}
		}
		if(layer == trayLayer) {
			stack[index++] = trayWindow;
		}
	}

	JXRestackWindows(display, stack, index);

}

/****************************************************************************
 ****************************************************************************/
void SendClientMessage(ClientNode *np, AtomType type, AtomType message) {
	XEvent event;
	int status;

	Assert(np);

	memset(&event, 0, sizeof(event));
	event.xclient.type = ClientMessage;
	event.xclient.window = np->window;
	event.xclient.message_type = atoms[type];
	event.xclient.format = 32;
	event.xclient.data.l[0] = atoms[message];
	event.xclient.data.l[1] = CurrentTime;

	status = JXSendEvent(display, np->window, False, 0, &event);
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

	/* Remove this client from the client list */
	if(np->next) {
		np->next->prev = np->prev;
	} else {
		nodeTail[np->layer] = np->prev;
	}
	if(np->prev) {
		np->prev->next = np->next;
	} else {
		nodes[np->layer] = np->next;
	}
	--clientCount;
	XDeleteContext(display, np->window, clientContext);
	XDeleteContext(display, np->parent, frameContext);

	/* Make sure this client isn't active */
	if(activeClient == np) {
		SetWindowAtom(rootWindow, ATOM_NET_ACTIVE_WINDOW, None);
		activeClient = NULL;
	}

	/* If the window manager is exiting (ie, not the client), then
	 * reparent etc. */
	if(shouldExit && !(np->statusFlags & STAT_WMDIALOG)) {
		Gravitate(np, 1);
		if(!(np->statusFlags & STAT_MAPPED)
			&& (np->statusFlags & (STAT_MINIMIZED | STAT_SHADED))) {
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

	RemoveClientFromTray(np);

	while(np->colormaps) {
		cp = np->colormaps->next;
		Release(np->colormaps);
		np->colormaps = cp;
	}

	DestroyIcon(np->titleIcon);
	DestroyIcon(np->trayIcon);

	Release(np);

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

	attr.override_redirect = True;
	attr.event_mask = ButtonPressMask | ButtonReleaseMask | ExposureMask
		| PointerMotionMask | SubstructureRedirectMask | SubstructureNotifyMask
		| EnterWindowMask | LeaveWindowMask | KeyPressMask;
	attr.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask;
	attr.background_pixel = colors[COLOR_BORDER_BG];

	if(np->borderFlags & BORDER_OUTLINE) {
		x = np->x - borderWidth;
		width = np->width + borderWidth + borderWidth;
		if(np->borderFlags & BORDER_TITLE) {
			y = np->y - titleSize;
			height = np->height + titleSize + borderWidth;
		} else {
			y = np->y - borderWidth;
			height = np->height + 2 * borderWidth;
		}
	} else {
		x = np->x;
		y = np->y;
		width = np->width;
		height = np->height;
	}

	np->parent = JXCreateWindow(display, rootWindow,
		x, y, width, height, 0, rootDepth, InputOutput,
		rootVisual,
		CWOverrideRedirect | CWBackPixel | CWEventMask | CWDontPropagate,
		&attr);
	np->parentGC = JXCreateGC(display, np->parent, 0, NULL);

	JXSetWindowBorderWidth(display, np->window, 0);
	if(np->borderFlags & BORDER_OUTLINE) {
		if(np->borderFlags & BORDER_TITLE) {
			JXReparentWindow(display, np->window, np->parent, borderWidth,
				titleSize);
		} else {
			JXReparentWindow(display, np->window, np->parent, borderWidth,
				borderWidth);
		}
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
		np->statusFlags |= STAT_USESHAPE;
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

/****************************************************************************
 ****************************************************************************/
void UpdateState(ClientNode *np) {
	unsigned long data[2];

	Assert(np);

	if(np->statusFlags & STAT_WITHDRAWN) {
		data[0] = WithdrawnState;
	} else if(np->statusFlags & STAT_MINIMIZED) {
		data[0] = IconicState;
	} else {
		data[0] = NormalState;
	}
	data[1] = None;

	JXChangeProperty(display, np->window, atoms[ATOM_WM_STATE],
		atoms[ATOM_WM_STATE], 32, PropModeReplace,
		(unsigned char*)data, 2);

}

/****************************************************************************
 * Read Motif hints.
 ****************************************************************************/
void ReadMotifHints(ClientNode *np) {
	PropMwmHints *mhints;
	Atom type;
	unsigned long itemCount, bytesLeft;
	unsigned char *data;
	int format;

	Assert(np);

	if(JXGetWindowProperty(display, np->window, atoms[ATOM_MOTIF_WM_HINTS],
		0L, 20L, False, atoms[ATOM_MOTIF_WM_HINTS], &type, &format,
		&itemCount, &bytesLeft, &data) != Success) {
		return;
	}

	mhints = (PropMwmHints*)data;
	if(mhints) {

		if((mhints->flags & MWM_HINTS_FUNCTIONS)
			&& !(mhints->functions & MWM_FUNC_ALL)) {

			if(!(mhints->functions & MWM_FUNC_RESIZE)) {
				np->borderFlags &= ~BORDER_RESIZE;
			}
			if(!(mhints->functions & MWM_FUNC_MOVE)) {
				np->borderFlags &= ~BORDER_MOVE;
			}
			if(!(mhints->functions & MWM_FUNC_MINIMIZE)) {
				np->borderFlags &= ~BORDER_MIN;
			}
			if(!(mhints->functions & MWM_FUNC_MAXIMIZE)) {
				np->borderFlags &= ~BORDER_MAX;
			}
			if(!(mhints->functions & MWM_FUNC_CLOSE)) {
				np->borderFlags &= ~BORDER_CLOSE;
			}
		}

		if((mhints->flags & MWM_HINTS_DECORATIONS)
			&& !(mhints->decorations & MWM_DECOR_ALL)) {

			if(!(mhints->decorations & MWM_DECOR_BORDER)) {
				np->borderFlags &= ~BORDER_OUTLINE;
			}
			if(!(mhints->decorations & MWM_DECOR_TITLE)) {
				np->borderFlags &= ~BORDER_TITLE;
			}
			if(!(mhints->decorations & MWM_DECOR_MINIMIZE)) {
				np->borderFlags &= ~BORDER_MIN;
			}
			if(!(mhints->decorations & MWM_DECOR_MAXIMIZE)) {
				np->borderFlags &= ~BORDER_MAX;
			}
		}

		JXFree(mhints);
	}
}

