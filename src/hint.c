/****************************************************************************
 * Functions to handle hints.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"

typedef struct {
	Atom *atom;
	const char *name;
} ProtocolNode;

typedef struct {
	Atom *atom;
	const char *name;
} AtomNode;

Atom atoms[ATOM_COUNT];

static const AtomNode atomList[] = {

	{ &atoms[ATOM_COMPOUND_TEXT],             "COMPOUND_TEXT"               },

	{ &atoms[ATOM_WM_STATE],                  "WM_STATE"                    },
	{ &atoms[ATOM_WM_PROTOCOLS],              "WM_PROTOCOLS"                },
	{ &atoms[ATOM_WM_DELETE_WINDOW],          "WM_DELETE_WINDOW"            },
	{ &atoms[ATOM_WM_TAKE_FOCUS],             "WM_TAKE_FOCUS"               },
	{ &atoms[ATOM_WM_LOCALE_NAME],            "WM_LOCALE_NAME"              },
	{ &atoms[ATOM_WM_CHANGE_STATE],           "WM_CHANGE_STATE"             },
	{ &atoms[ATOM_WM_COLORMAP_WINDOWS],       "WM_COLORMAP_WINDOWS"         },

	{ &atoms[ATOM_NET_SUPPORTED],             "_NET_SUPPORTED"              },
	{ &atoms[ATOM_NET_NUMBER_OF_DESKTOPS],    "_NET_NUMBER_OF_DESKTOPS"     },
	{ &atoms[ATOM_NET_DESKTOP_GEOMETRY],      "_NET_DESKTOP_GEOMETRY"       },
	{ &atoms[ATOM_NET_DESKTOP_VIEWPORT],      "_NET_DESKTOP_VIEWPORT"       },
	{ &atoms[ATOM_NET_CURRENT_DESKTOP],       "_NET_CURRENT_DESKTOP"        },
	{ &atoms[ATOM_NET_ACTIVE_WINDOW],         "_NET_ACTIVE_WINDOW"          },
	{ &atoms[ATOM_NET_WORKAREA],              "_NET_WORKAREA"               },
	{ &atoms[ATOM_NET_SUPPORTING_WM_CHECK],   "_NET_SUPPORTING_WM_CHECK"    },
	{ &atoms[ATOM_NET_WM_DESKTOP],            "_NET_WM_DESKTOP"             },
	{ &atoms[ATOM_NET_WM_STATE],              "_NET_WM_STATE"               },
	{ &atoms[ATOM_NET_WM_STATE_STICKY],       "_NET_WM_STATE_STICKY"        },
	{ &atoms[ATOM_NET_WM_NAME],               "_NET_WM_NAME"                },
	{ &atoms[ATOM_NET_WM_ICON],               "_NET_WM_ICON"                },
	{ &atoms[ATOM_NET_WM_WINDOW_TYPE],        "_NET_WM_WINDOW_TYPE"         },
	{ &atoms[ATOM_NET_WM_WINDOW_TYPE_DESKTOP],"_NET_WM_WINDOW_TYPE_DESKTOP" },

	{ &atoms[ATOM_WIN_LAYER],                 "_WIN_LAYER"                  },
	{ &atoms[ATOM_WIN_STATE],                 "_WIN_STATE"                  },
	{ &atoms[ATOM_WIN_WORKSPACE],             "_WIN_WORKSPACE"              },
	{ &atoms[ATOM_WIN_WORKSPACE_COUNT],       "_WIN_WORKSPACE_COUNT"        },
	{ &atoms[ATOM_WIN_SUPPORTING_WM_CHECK],   "_WIN_SUPPORTING_WM_CHECK"    },
	{ &atoms[ATOM_WIN_PROTOCOLS],             "_WIN_PROTOCOLS"              },

	{ &atoms[ATOM_MOTIF_WM_HINTS],            "_MOTIF_WM_HINTS"             },

	{ &atoms[ATOM_JWM_RESTART],               "_JWM_RESTART"                },
	{ &atoms[ATOM_JWM_EXIT],                  "_JWM_EXIT"                   }

};

/****************************************************************************
 ****************************************************************************/
void InitializeHints() {
}

/****************************************************************************
 ****************************************************************************/
void StartupHints() {
	long array[128];
	Atom supported[ATOM_COUNT];
	int x;

	/* Intern the atoms */
	for(x = 0; x < ATOM_COUNT; x++) {
		*atomList[x].atom = JXInternAtom(display, atomList[x].name, False);
	}

	/* Set _NET_SUPPORTED */
	for(x = FIRST_NET_ATOM; x <= LAST_NET_ATOM; x++) {
		supported[x - FIRST_NET_ATOM] = atoms[x];
	}
	JXChangeProperty(display, rootWindow, atoms[ATOM_NET_SUPPORTED],
		XA_ATOM, 32, PropModeReplace, (unsigned char*)supported,
		LAST_NET_ATOM - FIRST_NET_ATOM + 1);

	/* Set _WIN_PROTOCOLS */
	for(x = FIRST_WIN_ATOM; x <= LAST_WIN_ATOM; x++) {
		supported[x - FIRST_WIN_ATOM] = atoms[x];
	}
	JXChangeProperty(display, rootWindow, atoms[ATOM_WIN_PROTOCOLS],
		XA_ATOM, 32, PropModeReplace, (unsigned char*)supported,
		LAST_WIN_ATOM - FIRST_WIN_ATOM + 1);

	SetCardinalAtom(rootWindow, ATOM_NET_NUMBER_OF_DESKTOPS, desktopCount);

	array[0] = rootWidth;
	array[1] = rootHeight;
	JXChangeProperty(display, rootWindow, atoms[ATOM_NET_DESKTOP_GEOMETRY],
		XA_CARDINAL, 32, PropModeReplace,
		(unsigned char*)array, 2);

	array[0] = 0;
	array[1] = 0;
	JXChangeProperty(display, rootWindow, atoms[ATOM_NET_DESKTOP_VIEWPORT],
		XA_CARDINAL, 32, PropModeReplace,
		(unsigned char*)array, 2);

	for(x = 0; x < desktopCount; x++) {
		array[x * 4 + 0] = 0;
		array[x * 4 + 1] = 0;
		array[x * 4 + 2] = rootWidth;
		array[x * 4 + 3] = rootHeight;
	}
	JXChangeProperty(display, rootWindow, atoms[ATOM_NET_WORKAREA],
		XA_CARDINAL, 32, PropModeReplace,
		(unsigned char*)array, desktopCount * 4);

/*TODO: note, we may not have a tray
	SetWindowAtom(rootWindow, ATOM_NET_SUPPORTING_WM_CHECK, trayWindow);
	SetWindowAtom(trayWindow, ATOM_NET_SUPPORTING_WM_CHECK, trayWindow);

	SetCardinalAtom(rootWindow, ATOM_WIN_SUPPORTING_WM_CHECK, trayWindow);
	SetCardinalAtom(trayWindow, ATOM_WIN_SUPPORTING_WM_CHECK, trayWindow);
*/

	SetCardinalAtom(rootWindow, ATOM_WIN_WORKSPACE_COUNT, desktopCount);

}

/****************************************************************************
 ****************************************************************************/
void ShutdownHints() {
}

/****************************************************************************
 ****************************************************************************/
void DestroyHints() {
}

/****************************************************************************
 ****************************************************************************/
void ReadCurrentDesktop() {
	CARD32 temp;

	currentDesktop = 0;

	if(GetCardinalAtom(rootWindow, ATOM_NET_CURRENT_DESKTOP, &temp)) {
		ChangeDesktop(temp);
	} else if(GetCardinalAtom(rootWindow, ATOM_WIN_WORKSPACE, &temp)) {
		ChangeDesktop(temp);
	} else {
		ChangeDesktop(0);
	}

}

/****************************************************************************
 * Read client protocls/hints.
 * This is called while the client is being added to management.
 ****************************************************************************/
void ReadClientProtocols(ClientNode *np) {
	unsigned long count, x;
	Status status;
	unsigned long extra;
	Atom realType;
	int realFormat;
	unsigned char *temp;
	Atom *state;
	CARD32 card;

	/* Read the standard hints */
	ReadWMName(np);
	ReadWMClass(np);
	ReadWMNormalHints(np);
	ReadWMColormaps(np);

	status = JXGetTransientForHint(display, np->window, &np->owner);
	if(!status) {
		np->owner = None;
	}

	/* Read WM Spec hints */
	if(GetCardinalAtom(np->window, ATOM_NET_WM_DESKTOP, &card)) {
		if(card == 0xFFFFFFFF) {
			np->statusFlags |= STAT_STICKY;
		} else {
			np->desktop = card;
		}
	}

	status = JXGetWindowProperty(display, np->window,
		atoms[ATOM_NET_WM_STATE], 0, 32, False, XA_ATOM, &realType,
		&realFormat, &count, &extra, &temp);
	if(status == Success) {
		if(count > 0) {
			state = (unsigned long*)temp;
			for(x = 0; x < count; x++) {
				if(state[x] == atoms[ATOM_NET_WM_STATE_STICKY]) {
					np->statusFlags |= STAT_STICKY;
				}
			}
		}
		if(temp) {
			JXFree(temp);
		}
	}

	status = JXGetWindowProperty(display, np->window,
		atoms[ATOM_NET_WM_WINDOW_TYPE], 0, 32, False, XA_ATOM, &realType,
		&realFormat, &count, &extra, &temp);
	if(status == Success) {
		if(count > 0) {
			state = (unsigned long*)temp;
			for(x = 0; x < count; x++) {
				if(state[x] == atoms[ATOM_NET_WM_WINDOW_TYPE_DESKTOP]) {
					np->statusFlags |= STAT_STICKY | STAT_NOLIST;
					np->layer = 0;
					np->borderFlags = BORDER_NONE;
				}
			}
		}
		if(temp) {
			JXFree(temp);
		}
	}

	/* Read GNOME hints */
	if(GetCardinalAtom(np->window, ATOM_WIN_STATE, &card)) {
		if(card & WIN_STATE_STICKY) {
			np->statusFlags |= STAT_STICKY;
		}
		if(card & WIN_STATE_MINIMIZED) {
			np->statusFlags |= STAT_MINIMIZED;
		}
		if(card & WIN_STATE_HIDDEN) {
			np->statusFlags |= STAT_NOLIST;
		}
		if(card & WIN_STATE_SHADED) {
			np->statusFlags |= STAT_SHADED;
		}
	}
	if(GetCardinalAtom(np->window, ATOM_WIN_LAYER, &card)) {
		np->layer = card;
	}

}

/****************************************************************************
 ****************************************************************************/
void ReadNetWMDesktop(ClientNode *np) {
	CARD32 temp;

	Assert(np);

	if(GetCardinalAtom(np->window, ATOM_NET_WM_DESKTOP, &temp)) {
		if(temp == 0xFFFFFFFF) {
			SetClientSticky(np, 1);
		} else {
			np->statusFlags &= ~STAT_STICKY;
			SetClientDesktop(np, temp);
		}
	}

}

/****************************************************************************
 ****************************************************************************/
void WriteWinState(ClientNode *np) {
	CARD32 flags;

	Assert(np);

	flags = 0;
	if(np->statusFlags & STAT_STICKY) {
		flags |= WIN_STATE_STICKY;
	}
	if(np->statusFlags & STAT_MINIMIZED) {
		flags |= WIN_STATE_MINIMIZED;
	}
	if(np->statusFlags & STAT_MAXIMIZED) {
		flags |= WIN_STATE_MAXIMIZED_VERT;
		flags |= WIN_STATE_MAXIMIZED_HORIZ;
	}
	if(np->statusFlags & STAT_NOLIST) {
		flags |= WIN_STATE_HIDDEN;
	}
	if(np->statusFlags & STAT_SHADED) {
		flags |= WIN_STATE_SHADED;
	}

	SetCardinalAtom(np->window, ATOM_WIN_STATE, flags);

}

/****************************************************************************
 ****************************************************************************/
void ReadWinLayer(ClientNode *np) {
	CARD32 temp;

	Assert(np);

	if(GetCardinalAtom(np->window, ATOM_WIN_LAYER, &temp)) {
		np->layer = temp;
	}

}

/****************************************************************************
 ****************************************************************************/
void ReadWMName(ClientNode *np) {
	unsigned long count;
	int status;
	unsigned long extra;
	Atom realType;
	int realFormat;

	Assert(np);

	if(np->name) {
		JXFree(np->name);
	}
	if(JXFetchName(display, np->window, &np->name) == 0) {
		np->name = NULL;
	}

	if(!np->name) {
		status = JXGetWindowProperty(display, np->window, XA_WM_NAME,
			0, 1024, False, atoms[ATOM_COMPOUND_TEXT], &realType,
			&realFormat, &count, &extra, (unsigned char**)&np->name);
		if(status != Success) {
			np->name = NULL;
		}
	}

}

/****************************************************************************
 ****************************************************************************/
void ReadWMClass(ClientNode *np) {
	XClassHint hint;

	Assert(np);

	if(JXGetClassHint(display, np->window, &hint)) {
		np->instanceName = hint.res_name;
		np->className = hint.res_class;
	}

}

/****************************************************************************
 ****************************************************************************/
void ReadWMProtocols(ClientNode *np) {
	unsigned long count, x;
	int status;
	unsigned long extra;
	Atom realType;
	int realFormat;
	unsigned char *temp;
	Atom *p;

	Assert(np);

	status = JXGetWindowProperty(display, np->window, atoms[ATOM_WM_PROTOCOLS],
		0, 32, False, XA_ATOM, &realType, &realFormat, &count, &extra, &temp);
	p = (Atom*)temp;

	if(status != Success || !p || count < 0) {
		return;
	}

	np->protocols = PROT_NONE;
	for(x = 0; x < count; x++) {
		if(p[x] == atoms[ATOM_WM_DELETE_WINDOW]) {
			np->protocols |= PROT_DELETE;
		} else if(p[x] == atoms[ATOM_WM_TAKE_FOCUS]) {
			np->protocols |= PROT_TAKE_FOCUS;
		}
	}

	JXFree(p);

}

/****************************************************************************
 ****************************************************************************/
void ReadWMNormalHints(ClientNode *np) {
	XSizeHints hints;
	long temp;

	Assert(np);

	if(!JXGetWMNormalHints(display, np->window, &hints, &temp)) {
		np->sizeFlags = 0;
	} else {
		np->sizeFlags = hints.flags;
	}

	if(np->sizeFlags & PResizeInc) {
		np->xinc = Max(1, hints.width_inc);
		np->yinc = Max(1, hints.height_inc);
	} else {
		np->xinc = 1;
		np->yinc = 1;
	}

	if(np->sizeFlags & PMinSize) {
		np->minWidth = Max(0, hints.min_width);
		np->minHeight = Max(0, hints.min_height);
	} else {
		np->minWidth = 1;
		np->minHeight = 1;
	}

	if(np->sizeFlags & PMaxSize) {
		np->maxWidth =  hints.max_width;
		np->maxHeight = hints.max_height;
		if(np->maxWidth <= 0) {
			np->maxWidth = rootWidth;
		}
		if(np->maxHeight <= 0) {
			np->maxHeight = rootHeight;
		}
	} else {
		np->maxWidth = rootWidth;
		np->maxHeight = rootHeight;
	}

	if(np->sizeFlags & PBaseSize) {
		np->baseWidth = hints.base_width;
		np->baseHeight = hints.base_height;
	} else if(np->sizeFlags & PMinSize) {
		np->baseWidth = np->minWidth;
		np->baseHeight = np->minHeight;
	} else {
		np->baseWidth = 0;
		np->baseHeight = 0;
	}

	if(np->sizeFlags & PAspect) {
		np->aspect.minx = hints.min_aspect.x;
		np->aspect.miny = hints.min_aspect.y;
		np->aspect.maxx = hints.max_aspect.x;
		np->aspect.maxy = hints.max_aspect.y;
		if(np->aspect.minx < 1) {
			np->aspect.minx = 1;
		}
		if(np->aspect.miny < 1) {
			np->aspect.miny = 1;
		}
		if(np->aspect.maxx < 1) {
			np->aspect.maxx = 1;
		}
		if(np->aspect.maxy < 1) {
			np->aspect.maxy = 1;
		}
	}

	if(np->sizeFlags & PWinGravity) {
		np->gravity = hints.win_gravity;
	} else {
		np->gravity = 1;
	}
}

/****************************************************************************
 ****************************************************************************/
void ReadWMColormaps(ClientNode *np) {
	Window *windows;
	ColormapNode *cp;
	int count;
	int x;

	if(JXGetWMColormapWindows(display, np->window, &windows, &count)) {
		if(count > 0) {

			/* Free old colormaps. */
			while(np->colormaps) {
				cp = np->colormaps->next;
				Release(np->colormaps);
				np->colormaps = cp;
			}

			/* Put the maps in the list in order so they will come out in
			 * reverse order. This way they will be installed with the
			 * most important last.
			 * Keep track of at most colormapCount colormaps for each
			 * window to avoid doing extra work. */
			count = Min(colormapCount, count);
			for(x = 0; x < count; x++) {
				cp = Allocate(sizeof(ColormapNode));
				cp->window = windows[x];
				cp->next = np->colormaps;
				np->colormaps = cp;
			}

			JXFree(windows);

		}
	}

}

/****************************************************************************
 ****************************************************************************/
int GetCardinalAtom(Window window, AtomType atom, CARD32 *value) {
	unsigned long count;
	int status;
	unsigned long extra;
	Atom realType;
	int realFormat;
	unsigned char *data;
	int ret;

	Assert(value);

	status = JXGetWindowProperty(display, window, atoms[atom], 0, 1, False,
		XA_CARDINAL, &realType, &realFormat, &count, &extra, &data);

	ret = 0;
	if(status == Success && data) {
		if(count == 1) {
			*value = *(CARD32*)data;
			ret = 1;
		}
		JXFree(data);
	}

	return ret;

}

/****************************************************************************
 ****************************************************************************/
int GetWindowAtom(Window window, AtomType atom, Window *value) {
	unsigned long count;
	int status;
	unsigned long extra;
	Atom realType;
	int realFormat;
	unsigned char *data;
	int ret;

	Assert(value);

	status = JXGetWindowProperty(display, window, atoms[atom], 0, 1, False,
		XA_WINDOW, &realType, &realFormat, &count, &extra, &data);

	ret = 0;
	if(status == Success && data) {
		if(count == 1) {
			*value = *(CARD32*)data;
			ret = 1;
		}
		JXFree(data);
	}

	return ret;

}

/****************************************************************************
 ****************************************************************************/
void SetCardinalAtom(Window window, AtomType atom, CARD32 value) {

	JXChangeProperty(display, window, atoms[atom], XA_CARDINAL, 32,
		PropModeReplace, (unsigned char*)&value, 1);

}

/****************************************************************************
 ****************************************************************************/
void SetWindowAtom(Window window, AtomType atom, CARD32 value) {

	JXChangeProperty(display, window, atoms[atom], XA_WINDOW, 32,
		PropModeReplace, (unsigned char*)&value, 1);

}


