/**
 * @file hint.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions for reading and writing X properties.
 *
 */

#include <X11/Xlibint.h>
#include "jwm.h"
#include "hint.h"
#include "client.h"
#include "main.h"
#include "tray.h"
#include "desktop.h"
#include "misc.h"

/* MWM Defines */
#define MWM_HINTS_FUNCTIONS   (1L << 0)
#define MWM_HINTS_DECORATIONS (1L << 1)
#define MWM_HINTS_INPUT_MODE  (1L << 2)
#define MWM_HINTS_STATUS      (1L << 3)

#define MWM_FUNC_ALL      (1L << 0)
#define MWM_FUNC_RESIZE   (1L << 1)
#define MWM_FUNC_MOVE     (1L << 2)
#define MWM_FUNC_MINIMIZE (1L << 3)
#define MWM_FUNC_MAXIMIZE (1L << 4)
#define MWM_FUNC_CLOSE    (1L << 5)

#define MWM_DECOR_ALL      (1L << 0)
#define MWM_DECOR_BORDER   (1L << 1)
#define MWM_DECOR_RESIZEH  (1L << 2)
#define MWM_DECOR_TITLE    (1L << 3)
#define MWM_DECOR_MENU     (1L << 4)
#define MWM_DECOR_MINIMIZE (1L << 5)
#define MWM_DECOR_MAXIMIZE (1L << 6)

#define MWM_INPUT_MODELESS                  0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL              2
#define MWM_INPUT_FULL_APPLICATION_MODAL    3

#define MWM_TEAROFF_WINDOW (1L << 0)

typedef struct {

   unsigned long flags;
   unsigned long functions;
   unsigned long decorations;
   long          inputMode;
   unsigned long status;

} PropMwmHints;

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
   { &atoms[ATOM_UTF8_STRING],               "UTF8_STRING"                 },
   { &atoms[ATOM_XSETROOT_ID],               "_XSETROOT_ID"                },

   { &atoms[ATOM_WM_STATE],                  "WM_STATE"                    },
   { &atoms[ATOM_WM_PROTOCOLS],              "WM_PROTOCOLS"                },
   { &atoms[ATOM_WM_DELETE_WINDOW],          "WM_DELETE_WINDOW"            },
   { &atoms[ATOM_WM_TAKE_FOCUS],             "WM_TAKE_FOCUS"               },
   { &atoms[ATOM_WM_LOCALE_NAME],            "WM_LOCALE_NAME"              },
   { &atoms[ATOM_WM_CHANGE_STATE],           "WM_CHANGE_STATE"             },
   { &atoms[ATOM_WM_COLORMAP_WINDOWS],       "WM_COLORMAP_WINDOWS"         },

   { &atoms[ATOM_NET_SUPPORTED],             "_NET_SUPPORTED"              },
   { &atoms[ATOM_NET_NUMBER_OF_DESKTOPS],    "_NET_NUMBER_OF_DESKTOPS"     },
   { &atoms[ATOM_NET_DESKTOP_NAMES],         "_NET_DESKTOP_NAMES"          },
   { &atoms[ATOM_NET_DESKTOP_GEOMETRY],      "_NET_DESKTOP_GEOMETRY"       },
   { &atoms[ATOM_NET_DESKTOP_VIEWPORT],      "_NET_DESKTOP_VIEWPORT"       },
   { &atoms[ATOM_NET_CURRENT_DESKTOP],       "_NET_CURRENT_DESKTOP"        },
   { &atoms[ATOM_NET_ACTIVE_WINDOW],         "_NET_ACTIVE_WINDOW"          },
   { &atoms[ATOM_NET_WORKAREA],              "_NET_WORKAREA"               },
   { &atoms[ATOM_NET_SUPPORTING_WM_CHECK],   "_NET_SUPPORTING_WM_CHECK"    },
   { &atoms[ATOM_NET_FRAME_EXTENTS],         "_NET_FRAME_EXTENTS"          },
   { &atoms[ATOM_NET_WM_DESKTOP],            "_NET_WM_DESKTOP"             },
   { &atoms[ATOM_NET_WM_STATE],              "_NET_WM_STATE"               },
   { &atoms[ATOM_NET_WM_STATE_STICKY],       "_NET_WM_STATE_STICKY"        },
   { &atoms[ATOM_NET_WM_STATE_MAXIMIZED_VERT], "_NET_WM_STATE_MAXIMIZED_VERT"},
   { &atoms[ATOM_NET_WM_STATE_MAXIMIZED_HORZ], "_NET_WM_STATE_MAXIMIZED_HORZ"},
   { &atoms[ATOM_NET_WM_STATE_SHADED],       "_NET_WM_STATE_SHADED"        },
   { &atoms[ATOM_NET_WM_STATE_FULLSCREEN],   "_NET_WM_STATE_FULLSCREEN"    },
   { &atoms[ATOM_NET_WM_STATE_HIDDEN],       "_NET_WM_STATE_HIDDEN"        },
   { &atoms[ATOM_NET_WM_STATE_SKIP_TASKBAR], "_NET_WM_STATE_SKIP_TASKBAR"  },
   { &atoms[ATOM_NET_WM_ALLOWED_ACTIONS],    "_NET_WM_ALLOWED_ACTIONS"     },
   { &atoms[ATOM_NET_WM_ACTION_MOVE],        "_NET_WM_ACTION_MOVE"         },
   { &atoms[ATOM_NET_WM_ACTION_RESIZE],      "_NET_WM_ACTION_RESIZE"       },
   { &atoms[ATOM_NET_WM_ACTION_MINIMIZE],    "_NET_WM_ACTION_MINIMIZE"     },
   { &atoms[ATOM_NET_WM_ACTION_SHADE],       "_NET_WM_ACTION_SHADE"        },
   { &atoms[ATOM_NET_WM_ACTION_STICK],       "_NET_WM_ACTION_STICK"        },
   { &atoms[ATOM_NET_WM_ACTION_MAXIMIZE_HORZ], "_NET_WM_ACTION_MAXIMIZE_HORZ"},
   { &atoms[ATOM_NET_WM_ACTION_MAXIMIZE_VERT], "_NET_WM_ACTION_MAXIMIZE_VERT"},
   { &atoms[ATOM_NET_WM_ACTION_CHANGE_DESKTOP],
      "_NET_WM_ACTION_CHANGE_DESKTOP"},
   { &atoms[ATOM_NET_WM_ACTION_CLOSE],       "_NET_WM_ACTION_CLOSE"        },
   { &atoms[ATOM_NET_CLOSE_WINDOW],          "_NET_CLOSE_WINDOW"           },
   { &atoms[ATOM_NET_MOVERESIZE_WINDOW],     "_NET_MOVERESIZE_WINDOW"      },
   { &atoms[ATOM_NET_WM_NAME],               "_NET_WM_NAME"                },
   { &atoms[ATOM_NET_WM_ICON],               "_NET_WM_ICON"                },
   { &atoms[ATOM_NET_WM_WINDOW_TYPE],        "_NET_WM_WINDOW_TYPE"         },
   { &atoms[ATOM_NET_WM_WINDOW_TYPE_DESKTOP],"_NET_WM_WINDOW_TYPE_DESKTOP" },
   { &atoms[ATOM_NET_WM_WINDOW_TYPE_DOCK],   "_NET_WM_WINDOW_TYPE_DOCK"    },
   { &atoms[ATOM_NET_WM_WINDOW_TYPE_SPLASH], "_NET_WM_WINDOW_TYPE_SPLASH"  },
   { &atoms[ATOM_NET_WM_WINDOW_TYPE_DIALOG], "_NET_WM_WINDOW_TYPE_DIALOG"  },
   { &atoms[ATOM_NET_WM_WINDOW_TYPE_NORMAL], "_NET_WM_WINDOW_TYPE_NORMAL"  },
   { &atoms[ATOM_NET_CLIENT_LIST],           "_NET_CLIENT_LIST"            },
   { &atoms[ATOM_NET_CLIENT_LIST_STACKING],  "_NET_CLIENT_LIST_STACKING"   },
   { &atoms[ATOM_NET_WM_STRUT_PARTIAL],      "_NET_WM_STRUT_PARTIAL"       },
   { &atoms[ATOM_NET_WM_STRUT],              "_NET_WM_STRUT"               },
   { &atoms[ATOM_NET_SYSTEM_TRAY_OPCODE],    "_NET_SYSTEM_TRAY_OPCODE"     },
   { &atoms[ATOM_NET_WM_WINDOW_OPACITY],     "_NET_WM_WINDOW_OPACITY"      },

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

static void WriteNetState(ClientNode *np);
static void WriteNetAllowed(ClientNode *np);
static void WriteWinState(ClientNode *np);
static void ReadWMHints(Window win, ClientState *state);
static void ReadMotifHints(Window win, ClientState *state);

/** Initialize hints data. */
void InitializeHints() {
}

/** Set root hints and intern atoms. */
void StartupHints() {

   unsigned long *array;
   char *data;
   Atom *supported;
   Window win;
   unsigned int x;
   unsigned int count;

   /* Determine how much space we will need on the stack and allocate it. */
   count = 0;
   for(x = 0; x < desktopCount; x++) {
      count += strlen(GetDesktopName(x)) + 1;
   }
   if(count < 4 * desktopCount * sizeof(unsigned long)) {
      count = 4 * desktopCount * sizeof(unsigned long);
   }
   if(count < 2 * sizeof(unsigned long)) {
      count = 2 * sizeof(unsigned long);
   }
   if(count < ATOM_COUNT * sizeof(Atom)) {
      count = ATOM_COUNT * sizeof(Atom);
   }
   data = AllocateStack(count);
   array = (unsigned long*)data;
   supported = (Atom*)data;

   /* Intern the atoms */
   for(x = 0; x < ATOM_COUNT; x++) {
      *atomList[x].atom = JXInternAtom(display, atomList[x].name, False);
   }

   /* _NET_SUPPORTED */
   for(x = FIRST_NET_ATOM; x <= LAST_NET_ATOM; x++) {
      supported[x - FIRST_NET_ATOM] = atoms[x];
   }
   JXChangeProperty(display, rootWindow, atoms[ATOM_NET_SUPPORTED],
      XA_ATOM, 32, PropModeReplace, (unsigned char*)supported,
      LAST_NET_ATOM - FIRST_NET_ATOM + 1);

   /* _WIN_PROTOCOLS */
   for(x = FIRST_WIN_ATOM; x <= LAST_WIN_ATOM; x++) {
      supported[x - FIRST_WIN_ATOM] = atoms[x];
   }
   JXChangeProperty(display, rootWindow, atoms[ATOM_WIN_PROTOCOLS],
      XA_ATOM, 32, PropModeReplace, (unsigned char*)supported,
      LAST_WIN_ATOM - FIRST_WIN_ATOM + 1);

   /* _NET_NUMBER_OF_DESKTOPS */
   SetCardinalAtom(rootWindow, ATOM_NET_NUMBER_OF_DESKTOPS, desktopCount);

   /* _NET_DESKTOP_NAMES */
   count = 0;
   for(x = 0; x < desktopCount; x++) {
      strcpy(data + count, GetDesktopName(x));
      count += strlen(GetDesktopName(x)) + 1;
   }
   JXChangeProperty(display, rootWindow, atoms[ATOM_NET_DESKTOP_NAMES],
      atoms[ATOM_UTF8_STRING], 8, PropModeReplace,
      (unsigned char*)data, count);

   /* _NET_WORKAREA */
   for(x = 0; x < desktopCount; x++) {
      array[x * 4 + 0] = 0;
      array[x * 4 + 1] = 0;
      array[x * 4 + 2] = rootWidth;
      array[x * 4 + 3] = rootHeight;
   }
   JXChangeProperty(display, rootWindow, atoms[ATOM_NET_WORKAREA],
      XA_CARDINAL, 32, PropModeReplace,
      (unsigned char*)array, desktopCount * 4);

   /* _NET_DESKTOP_GEOMETRY */
   array[0] = rootWidth;
   array[1] = rootHeight;
   JXChangeProperty(display, rootWindow, atoms[ATOM_NET_DESKTOP_GEOMETRY],
      XA_CARDINAL, 32, PropModeReplace,
      (unsigned char*)array, 2);

   /* _NET_DESKTOP_VIEWPORT */
   array[0] = 0;
   array[1] = 0;
   JXChangeProperty(display, rootWindow, atoms[ATOM_NET_DESKTOP_VIEWPORT],
      XA_CARDINAL, 32, PropModeReplace,
      (unsigned char*)array, 2);

   win = GetSupportingWindow();
   JXChangeProperty(display, win, atoms[ATOM_NET_WM_NAME],
      atoms[ATOM_UTF8_STRING], 8, PropModeReplace,
      (unsigned char*)"JWM", 3);

   SetWindowAtom(rootWindow, ATOM_NET_SUPPORTING_WM_CHECK, win);
   SetWindowAtom(win, ATOM_NET_SUPPORTING_WM_CHECK, win);

   SetWindowAtom(rootWindow, ATOM_WIN_SUPPORTING_WM_CHECK, win);
   SetWindowAtom(win, ATOM_WIN_SUPPORTING_WM_CHECK, win);

   SetCardinalAtom(rootWindow, ATOM_WIN_WORKSPACE_COUNT, desktopCount);

   ReleaseStack(data);

}

/** Shutdown hints. */
void ShutdownHints() {
}

/** Destroy hints data. */
void DestroyHints() {
}

/** Determine the current desktop. */
void ReadCurrentDesktop() {

   unsigned long temp;

   currentDesktop = 0;

   if(GetCardinalAtom(rootWindow, ATOM_NET_CURRENT_DESKTOP, &temp)) {
      ChangeDesktop(temp);
   } else if(GetCardinalAtom(rootWindow, ATOM_WIN_WORKSPACE, &temp)) {
      ChangeDesktop(temp);
   } else {
      ChangeDesktop(0);
   }

}

/** Read client protocls/hints.
 * This is called while the client is being added to management.
 */
void ReadClientProtocols(ClientNode *np) {

   Status status;
   ClientNode *pp;

   Assert(np);

   ReadWMName(np);
   ReadWMClass(np);
   ReadWMNormalHints(np);
   ReadWMColormaps(np);

   status = JXGetTransientForHint(display, np->window, &np->owner);
   if(!status) {
      np->owner = None;
   }

   np->state = ReadWindowState(np->window);
   if(np->minWidth == np->maxWidth && np->minHeight == np->maxHeight) {
      np->state.border &= ~BORDER_RESIZE;
   }

   /* Set the client to the same layer as its owner. */
   if(np->owner != None) {
      pp = FindClientByWindow(np->owner);
      if(pp) {
         np->state.layer = pp->state.layer;
      }
   }

}

/** Write the window state hint for a client. */
void WriteState(ClientNode *np) {

   unsigned long data[2];

   Assert(np);

   if(np->state.status & STAT_MAPPED) {
      data[0] = NormalState;
   } else if(np->state.status & STAT_MINIMIZED) {
      data[0] = IconicState;
   } else {
      data[0] = WithdrawnState;
   }
   data[1] = None;

   JXChangeProperty(display, np->window, atoms[ATOM_WM_STATE],
      atoms[ATOM_WM_STATE], 32, PropModeReplace,
      (unsigned char*)data, 2);

   WriteNetState(np);
   WriteNetAllowed(np);
   WriteWinState(np);

   /* Write the opacity. */
   if(np->state.opacity == UINT_MAX) {
      JXDeleteProperty(display, np->parent, atoms[ATOM_NET_WM_WINDOW_OPACITY]);
   } else {
      SetCardinalAtom(np->parent, ATOM_NET_WM_WINDOW_OPACITY,
         np->state.opacity);
   }

   /* Flush to the server. */
   JXSync(display, False);

}

/** Write the net state hint for a client. */
void WriteNetState(ClientNode *np) {

   unsigned long values[6];
   int north, south, east, west;
   int index;

   Assert(np);

   /* We remove the _NET_WM_STATE for withdrawn windows. */
   if(!(np->state.status & STAT_MAPPED)) {
      JXDeleteProperty(display, np->window, atoms[ATOM_NET_WM_STATE]);
      return;
   } 

   index = 0;
   if(np->state.status & STAT_HMAX) {
      values[index++] = atoms[ATOM_NET_WM_STATE_MAXIMIZED_HORZ];
   }
   if(np->state.status & STAT_VMAX) {
      values[index++] = atoms[ATOM_NET_WM_STATE_MAXIMIZED_VERT];
   }

   if(np->state.status & STAT_SHADED) {
      values[index++] = atoms[ATOM_NET_WM_STATE_SHADED];
   }

   if(np->state.status & STAT_STICKY) {
      values[index++] = atoms[ATOM_NET_WM_STATE_STICKY];
   }

   if(np->state.status & STAT_FULLSCREEN) {
      values[index++] = atoms[ATOM_NET_WM_STATE_FULLSCREEN];
   }

   if(np->state.status & STAT_NOLIST) {
      values[index++] = atoms[ATOM_NET_WM_STATE_SKIP_TASKBAR];
   }

   JXChangeProperty(display, np->window, atoms[ATOM_NET_WM_STATE],
      XA_ATOM, 32, PropModeReplace, (unsigned char*)values, index);

   GetBorderSize(np, &north, &south, &east, &west);

   /* left, right, top, bottom */
   values[0] = west;
   values[1] = east;
   values[2] = north;
   values[3] = south;

   JXChangeProperty(display, np->window, atoms[ATOM_NET_FRAME_EXTENTS],
      XA_CARDINAL, 32, PropModeReplace, (unsigned char*)values, 4);

}

/** Write the allowed action property. */
void WriteNetAllowed(ClientNode *np) {

   unsigned long values[10];
   int index;

   Assert(np);

   index = 0;

   if(np->state.border & BORDER_TITLE) {
      values[index++] = atoms[ATOM_NET_WM_ACTION_SHADE];
   }

   if(np->state.border & BORDER_MIN) {
      values[index++] = atoms[ATOM_NET_WM_ACTION_MINIMIZE];
   }

   if(np->state.border & BORDER_MAX) {
      values[index++] = atoms[ATOM_NET_WM_ACTION_MAXIMIZE_HORZ];
      values[index++] = atoms[ATOM_NET_WM_ACTION_MAXIMIZE_VERT];
   }

   if(np->state.border & BORDER_CLOSE) {
      values[index++] = atoms[ATOM_NET_WM_ACTION_CLOSE];
   }

   if(np->state.border & BORDER_RESIZE) {
      values[index++] = atoms[ATOM_NET_WM_ACTION_RESIZE];
   }

   if(np->state.border & BORDER_MOVE) {
      values[index++] = atoms[ATOM_NET_WM_ACTION_MOVE];
   }

   if(!(np->state.status & STAT_STICKY)) {
      values[index++] = atoms[ATOM_NET_WM_ACTION_CHANGE_DESKTOP];
   }

   values[index++] = atoms[ATOM_NET_WM_ACTION_STICK];

   JXChangeProperty(display, np->window, atoms[ATOM_NET_WM_ALLOWED_ACTIONS],
      XA_ATOM, 32, PropModeReplace, (unsigned char*)values, index);

}

/** Write the win state hint for a client (GNOME). */
void WriteWinState(ClientNode *np) {

   unsigned long flags;

   Assert(np);

   if(!(np->state.status & STAT_MAPPED)) {
      JXDeleteProperty(display, np->window, atoms[ATOM_WIN_STATE]);
      return;
   }

   flags = 0;
   if(np->state.status & STAT_STICKY) {
      flags |= WIN_STATE_STICKY;
   }
   if(np->state.status & STAT_MINIMIZED) {
      flags |= WIN_STATE_MINIMIZED;
   }
   if(np->state.status & STAT_HMAX) {
      flags |= WIN_STATE_MAXIMIZED_HORIZ;
   }
   if(np->state.status & STAT_VMAX) {
      flags |= WIN_STATE_MAXIMIZED_VERT;
   }
   if(np->state.status & STAT_NOLIST) {
      flags |= WIN_STATE_HIDDEN;
   }
   if(np->state.status & STAT_SHADED) {
      flags |= WIN_STATE_SHADED;
   }

   SetCardinalAtom(np->window, ATOM_WIN_STATE, flags);

}

/** Read all hints needed to determine the current window state. */
ClientState ReadWindowState(Window win) {

   ClientState result;
   Status status;
   unsigned long count, x;
   unsigned long extra;
   Atom realType;
   int realFormat;
   unsigned char *temp;
   Atom *state;
   unsigned long card;
   int maxVert, maxHorz;
   int fullScreen;

   Assert(win != None);

   result.status = STAT_NONE;
   result.border = BORDER_DEFAULT;
   result.layer = LAYER_NORMAL;
   result.desktop = currentDesktop;
   result.opacity = 0xFFFFFFFF;

   ReadWMHints(win, &result);
   ReadMotifHints(win, &result);

   /* _NET_WM_DESKTOP */
   if(GetCardinalAtom(win, ATOM_NET_WM_DESKTOP, &card)) {
      if(card == ~0UL) {
         result.status |= STAT_STICKY;
      } else if(card < desktopCount) {
         result.desktop = card;
      } else {
         result.desktop = desktopCount - 1;
      }
   }

   /* _NET_WM_STATE */
   status = JXGetWindowProperty(display, win,
      atoms[ATOM_NET_WM_STATE], 0, 32, False, XA_ATOM, &realType,
      &realFormat, &count, &extra, &temp);
   if(status == Success) {
      if(count > 0) {
         maxVert = 0;
         maxHorz = 0;
         fullScreen = 0;
         state = (Atom*)temp;
         for(x = 0; x < count; x++) {
            if(state[x] == atoms[ATOM_NET_WM_STATE_STICKY]) {
               result.status |= STAT_STICKY;
            } else if(state[x] == atoms[ATOM_NET_WM_STATE_SHADED]) {
               result.status |= STAT_SHADED;
            } else if(state[x] == atoms[ATOM_NET_WM_STATE_MAXIMIZED_VERT]) {
               maxVert = 1;
            } else if(state[x] == atoms[ATOM_NET_WM_STATE_MAXIMIZED_HORZ]) {
               maxHorz = 1;
            } else if(state[x] == atoms[ATOM_NET_WM_STATE_FULLSCREEN]) {
               fullScreen = 1;
               result.layer = LAYER_TOP;
            } else if(state[x] == atoms[ATOM_NET_WM_STATE_HIDDEN]) {
               result.status |= STAT_MINIMIZED;
            } else if(state[x] == atoms[ATOM_NET_WM_STATE_SKIP_TASKBAR]) {
               result.status |= STAT_NOLIST;
            }
         }
         if(maxHorz) {
            result.status |= STAT_HMAX;
         }
         if(maxVert) {
            result.status |= STAT_VMAX;
         }
         if(fullScreen) {
            result.status |= STAT_FULLSCREEN;
         }
      }
      if(temp) {
         JXFree(temp);
      }
   }

   /* _NET_WM_WINDOW_TYPE */
   status = JXGetWindowProperty(display, win, atoms[ATOM_NET_WM_WINDOW_TYPE],
                                0, 32, False, XA_ATOM, &realType, &realFormat,
                                &count, &extra, &temp);
   if(status == Success) {
      /* Loop until we hit a window type we recognize. */
      state = (Atom*)temp;
      for(x = 0; x < count; x++) {
         if(         state[x] == atoms[ATOM_NET_WM_WINDOW_TYPE_NORMAL]) {
            break;
         } else if(  state[x] == atoms[ATOM_NET_WM_WINDOW_TYPE_DESKTOP]) {
            result.layer   = LAYER_BOTTOM;
            result.border  = BORDER_NONE;
            result.status |= STAT_STICKY;
            result.status |= STAT_NOLIST;
            break;
         } else if(  state[x] == atoms[ATOM_NET_WM_WINDOW_TYPE_DOCK]) {
            result.border = BORDER_NONE;
            result.layer = LAYER_TOP;
            break;
         } else if(  state[x] == atoms[ATOM_NET_WM_WINDOW_TYPE_SPLASH]) {
            result.border = BORDER_NONE;
            break;
         } else if(  state[x] == atoms[ATOM_NET_WM_WINDOW_TYPE_DIALOG]) {
            result.border &= ~BORDER_MIN;
            result.status |= STAT_NOLIST;
            break;
         } else {
            Debug("Unknown _NET_WM_WINDOW_TYPE: %lu", state[x]);
         }
      }
      if(temp) {
         JXFree(temp);
      }
   }

   /* _NET_WM_WINDOW_OPACITY */
   if(GetCardinalAtom(win, ATOM_NET_WM_WINDOW_OPACITY, &card)) {
      result.opacity = card;
   }

   /* _WIN_STATE */
   if(GetCardinalAtom(win, ATOM_WIN_STATE, &card)) {
      if(card & WIN_STATE_STICKY) {
         result.status |= STAT_STICKY;
      }
      if(card & WIN_STATE_MINIMIZED) {
         result.status |= STAT_MINIMIZED;
      }
      if(card & WIN_STATE_HIDDEN) {
         result.status |= STAT_NOLIST;
      }
      if(card & WIN_STATE_SHADED) {
         result.status |= STAT_SHADED;
      }
      if(card & WIN_STATE_MAXIMIZED_HORIZ) {
         result.status |= STAT_HMAX;
      }
      if(card & WIN_STATE_MAXIMIZED_VERT) {
         result.status |= STAT_VMAX;
      }
   }

   /* _WIN_LAYER */
   if(GetCardinalAtom(win, ATOM_WIN_LAYER, &card)) {
      result.layer = card;
   }

   return result;

}

/** Determine the title to display for a client. */
void ReadWMName(ClientNode *np) {

   unsigned long count;
   int status;
   unsigned long extra;
   Atom realType;
   int realFormat;
   unsigned char *name;
   XTextProperty tprop;
   char **text_list;
   int tcount;

   Assert(np);

   if(np->name) {
      JXFree(np->name);
   }

   status = JXGetWindowProperty(display, np->window,
      atoms[ATOM_NET_WM_NAME], 0, 1024, False,
      atoms[ATOM_UTF8_STRING], &realType, &realFormat, &count, &extra, &name);
   if(status != Success) {
      np->name = NULL;
   } else {
      np->name = (char*)name;
   }

   if(!np->name) {
      status = JXGetWindowProperty(display, np->window,
            XA_WM_NAME, 0, 1024, False, atoms[ATOM_COMPOUND_TEXT],
            &realType, &realFormat, &count, &extra, &name);
      if(status == Success && realFormat == 8) {
         tprop.value = name;
         tprop.encoding = atoms[ATOM_COMPOUND_TEXT];
         tprop.format = realFormat;
         tprop.nitems = strlen((char *)name);
         if(Xutf8TextPropertyToTextList(display, &tprop, &text_list, &tcount)
            == Success && tcount > 0) {
            np->name = Xmalloc(strlen(text_list[0]) + 1);
            if(np->name) {
               strcpy(np->name, text_list[0]);
            }
            XFreeStringList(text_list);
         }
         JXFree(name);
      }
   }

   if(!np->name) {
      if(JXFetchName(display, np->window, &np->name) == 0) {
         np->name = NULL;
      }
   }

}

/** Read the window class for a client. */
void ReadWMClass(ClientNode *np) {

   XClassHint hint;

   Assert(np);

   if(JXGetClassHint(display, np->window, &hint)) {
      np->instanceName = hint.res_name;
      np->className = hint.res_class;
   }

}

/** Read the protocols hint for a window. */
ClientProtocolType ReadWMProtocols(Window w) {

   ClientProtocolType result;
   unsigned long count, x;
   int status;
   unsigned long extra;
   Atom realType;
   int realFormat;
   unsigned char *temp;
   Atom *p;

   Assert(w != None);

   result = PROT_NONE;
   status = JXGetWindowProperty(display, w, atoms[ATOM_WM_PROTOCOLS],
      0, 32, False, XA_ATOM, &realType, &realFormat, &count, &extra, &temp);
   p = (Atom*)temp;

   if(status != Success || !p) {
      return result;
   }

   for(x = 0; x < count; x++) {
      if(p[x] == atoms[ATOM_WM_DELETE_WINDOW]) {
         result |= PROT_DELETE;
      } else if(p[x] == atoms[ATOM_WM_TAKE_FOCUS]) {
         result |= PROT_TAKE_FOCUS;
      }
   }

   JXFree(p);

   return result;
   
}

/** Read the "normal hints" for a client. */
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

/** Read colormap information for a client. */
void ReadWMColormaps(ClientNode *np) {

   Window *windows;
   ColormapNode *cp;
   int count;
   int x;

   Assert(np);

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

/** Read the WM hints for a window. */
void ReadWMHints(Window win, ClientState *state) {

   XWMHints *wmhints;

   Assert(win != None);
   Assert(state);

   wmhints = JXGetWMHints(display, win);
   if(wmhints) {
      switch(wmhints->flags & StateHint) {
      case IconicState:
         state->status |= STAT_MINIMIZED;
         break;
      case WithdrawnState:
      default:
         if(!(state->status & (STAT_MINIMIZED | STAT_NOLIST))) {
            state->status |= STAT_MAPPED;
         }
         break;
      }
      JXFree(wmhints);
   } else {
      state->status |= STAT_MAPPED;
   }

}

/** Read _MOTIF_WM_HINTS */
void ReadMotifHints(Window win, ClientState *state) {

   PropMwmHints *mhints;
   Atom type;
   unsigned long itemCount, bytesLeft;
   unsigned char *data;
   int format;

   Assert(win != None);
   Assert(state);

   if(JXGetWindowProperty(display, win, atoms[ATOM_MOTIF_WM_HINTS],
      0L, 20L, False, atoms[ATOM_MOTIF_WM_HINTS], &type, &format,
      &itemCount, &bytesLeft, &data) != Success) {
      return;
   }

   mhints = (PropMwmHints*)data;
   if(mhints) {

      if((mhints->flags & MWM_HINTS_FUNCTIONS)
         && !(mhints->functions & MWM_FUNC_ALL)) {

         if(!(mhints->functions & MWM_FUNC_RESIZE)) {
            state->border &= ~BORDER_RESIZE;
         }
         if(!(mhints->functions & MWM_FUNC_MOVE)) {
            state->border &= ~BORDER_MOVE;
         }
         if(!(mhints->functions & MWM_FUNC_MINIMIZE)) {
            state->border &= ~BORDER_MIN;
         }
         if(!(mhints->functions & MWM_FUNC_MAXIMIZE)) {
            state->border &= ~BORDER_MAX;
         }
         if(!(mhints->functions & MWM_FUNC_CLOSE)) {
            state->border &= ~BORDER_CLOSE;
         }
      }

      if((mhints->flags & MWM_HINTS_DECORATIONS)
         && !(mhints->decorations & MWM_DECOR_ALL)) {

         if(!(mhints->decorations & MWM_DECOR_BORDER)) {
            state->border &= ~BORDER_OUTLINE;
         }
         if(!(mhints->decorations & MWM_DECOR_TITLE)) {
            state->border &= ~BORDER_TITLE;
         }
         if(!(mhints->decorations & MWM_DECOR_MINIMIZE)) {
            state->border &= ~BORDER_MIN;
         }
         if(!(mhints->decorations & MWM_DECOR_MAXIMIZE)) {
            state->border &= ~BORDER_MAX;
         }
      }

      JXFree(mhints);
   }
}

/** Read a cardinal atom. */
int GetCardinalAtom(Window window, AtomType atom, unsigned long *value) {

   unsigned long count;
   int status;
   unsigned long extra;
   Atom realType;
   int realFormat;
   unsigned char *data;
   int ret;

   Assert(window != None);
   Assert(value);

   status = JXGetWindowProperty(display, window, atoms[atom], 0, 1, False,
      XA_CARDINAL, &realType, &realFormat, &count, &extra, &data);

   ret = 0;
   if(status == Success && data) {
      if(count == 1) {
         *value = *(unsigned long*)data;
         ret = 1;
      }
      JXFree(data);
   }

   return ret;

}

/** Read a window atom. */
int GetWindowAtom(Window window, AtomType atom, Window *value) {
   unsigned long count;
   int status;
   unsigned long extra;
   Atom realType;
   int realFormat;
   unsigned char *data;
   int ret;

   Assert(window != None);
   Assert(value);

   status = JXGetWindowProperty(display, window, atoms[atom], 0, 1, False,
      XA_WINDOW, &realType, &realFormat, &count, &extra, &data);

   ret = 0;
   if(status == Success && data) {
      if(count == 1) {
         *value = *(Window*)data;
         ret = 1;
      }
      JXFree(data);
   }

   return ret;

}

/** Set a cardinal atom. */
void SetCardinalAtom(Window window, AtomType atom, unsigned long value) {

   Assert(window != None);

   JXChangeProperty(display, window, atoms[atom], XA_CARDINAL, 32,
      PropModeReplace, (unsigned char*)&value, 1);

}

/** Set a window atom. */
void SetWindowAtom(Window window, AtomType atom, unsigned long value) {

   Assert(window != None);

   JXChangeProperty(display, window, atoms[atom], XA_WINDOW, 32,
      PropModeReplace, (unsigned char*)&value, 1);

}

/** Set a pixmap atom. */
void SetPixmapAtom(Window window, AtomType atom, Pixmap value) {

   Assert(window != None);

   JXChangeProperty(display, window, atoms[atom], XA_PIXMAP, 32,
      PropModeReplace, (unsigned char*)&value, 1);

}

