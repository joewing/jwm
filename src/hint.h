/**
 * @file hint.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for reading and writing X properties.
 *
 */

#ifndef HINT_H
#define HINT_H

struct ClientNode;

/** Enumeration of atoms. */
typedef enum {

   /* Misc */
   ATOM_COMPOUND_TEXT,
   ATOM_UTF8_STRING,
   ATOM_XSETROOT_ID,

   /* Standard atoms */
   ATOM_WM_STATE,
   ATOM_WM_PROTOCOLS,
   ATOM_WM_DELETE_WINDOW,
   ATOM_WM_TAKE_FOCUS,
   ATOM_WM_LOCALE_NAME,
   ATOM_WM_CHANGE_STATE,
   ATOM_WM_COLORMAP_WINDOWS,

   /* WM Spec atoms */
   ATOM_NET_SUPPORTED,
   ATOM_NET_NUMBER_OF_DESKTOPS,
   ATOM_NET_DESKTOP_NAMES,
   ATOM_NET_DESKTOP_GEOMETRY,
   ATOM_NET_DESKTOP_VIEWPORT,
   ATOM_NET_CURRENT_DESKTOP,
   ATOM_NET_ACTIVE_WINDOW,
   ATOM_NET_WORKAREA,
   ATOM_NET_SUPPORTING_WM_CHECK,
   ATOM_NET_FRAME_EXTENTS,
   ATOM_NET_WM_DESKTOP,

   ATOM_NET_WM_STATE,
   ATOM_NET_WM_STATE_STICKY,
   ATOM_NET_WM_STATE_MAXIMIZED_VERT,
   ATOM_NET_WM_STATE_MAXIMIZED_HORZ,
   ATOM_NET_WM_STATE_SHADED,
   ATOM_NET_WM_STATE_FULLSCREEN,
   ATOM_NET_WM_STATE_HIDDEN,
   ATOM_NET_WM_STATE_SKIP_TASKBAR,

   ATOM_NET_WM_ALLOWED_ACTIONS,
   ATOM_NET_WM_ACTION_MOVE,
   ATOM_NET_WM_ACTION_RESIZE,
   ATOM_NET_WM_ACTION_MINIMIZE,
   ATOM_NET_WM_ACTION_SHADE,
   ATOM_NET_WM_ACTION_STICK,
   ATOM_NET_WM_ACTION_MAXIMIZE_HORZ,
   ATOM_NET_WM_ACTION_MAXIMIZE_VERT,
   ATOM_NET_WM_ACTION_CHANGE_DESKTOP,
   ATOM_NET_WM_ACTION_CLOSE,

   ATOM_NET_CLOSE_WINDOW,
   ATOM_NET_MOVERESIZE_WINDOW,

   ATOM_NET_WM_NAME,
   ATOM_NET_WM_ICON,
   ATOM_NET_WM_WINDOW_TYPE,
   ATOM_NET_WM_WINDOW_TYPE_DESKTOP,
   ATOM_NET_WM_WINDOW_TYPE_DOCK,
   ATOM_NET_WM_WINDOW_TYPE_SPLASH,
   ATOM_NET_WM_WINDOW_TYPE_DIALOG,
   ATOM_NET_WM_WINDOW_TYPE_NORMAL,

   ATOM_NET_CLIENT_LIST,
   ATOM_NET_CLIENT_LIST_STACKING,

   ATOM_NET_WM_STRUT_PARTIAL,
   ATOM_NET_WM_STRUT,

   ATOM_NET_SYSTEM_TRAY_OPCODE,

   ATOM_NET_WM_WINDOW_OPACITY,

   /* GNOME atoms */
   ATOM_WIN_LAYER,
   ATOM_WIN_STATE,
   ATOM_WIN_WORKSPACE_COUNT,
   ATOM_WIN_WORKSPACE,
   ATOM_WIN_SUPPORTING_WM_CHECK,
   ATOM_WIN_PROTOCOLS,

   /* MWM atoms */
   ATOM_MOTIF_WM_HINTS,

   /* JWM-specific atoms. */
   ATOM_JWM_RESTART,
   ATOM_JWM_EXIT,

   ATOM_COUNT
} AtomType;

#define FIRST_NET_ATOM ATOM_NET_SUPPORTED
#define LAST_NET_ATOM  ATOM_NET_SYSTEM_TRAY_OPCODE

#define FIRST_WIN_ATOM ATOM_WIN_LAYER
#define LAST_WIN_ATOM  ATOM_WIN_PROTOCOLS

#define FIRST_MWM_ATOM ATOM_MOTIF_WM_HINTS
#define LAST_MWM_ATOM  ATOM_MOTIF_WM_HINTS

#define WIN_STATE_STICKY          (1UL << 0)
#define WIN_STATE_MINIMIZED       (1UL << 1)
#define WIN_STATE_MAXIMIZED_VERT  (1UL << 2)
#define WIN_STATE_MAXIMIZED_HORIZ (1UL << 3)
#define WIN_STATE_HIDDEN          (1UL << 4)
#define WIN_STATE_SHADED          (1UL << 5)
#define WIN_STATE_HIDE_WORKSPACE  (1UL << 6)
#define WIN_STATE_HIDE_TRANSIENT  (1UL << 7)
#define WIN_STATE_FIXED_POSITION  (1UL << 8)
#define WIN_STATE_ARRANGE_IGNORE  (1UL << 9)

#define WIN_HINT_SKIP_FOCUS      (1UL << 0)
#define WIN_HINT_SKIP_WINLIST    (1UL << 1)
#define WIN_HINT_SKIP_TASKBAR    (1UL << 2)
#define WIN_HINT_GROUP_TRANSIENT (1UL << 3)
#define WIN_HINT_FOCUS_ON_CLICK  (1UL << 4)

/** Enumeration of window layers. */
typedef enum {
   LAYER_BOTTOM              = 0,
   LAYER_NORMAL              = 4,
   DEFAULT_TRAY_LAYER        = 8,
   LAYER_TOP                 = 12,
   LAYER_COUNT               = 13
} WinLayerType;

/** Client state information. */
typedef struct ClientState {
   unsigned int status;    /**< Status bit mask. */
   unsigned int border;    /**< Border bit mask. */
   unsigned int layer;     /**< Window layer. */
   unsigned int desktop;   /**< Desktop. */
   unsigned int opacity;   /**< Opacity (0 - 0xFFFFFFFF). */
} ClientState;

/** Client protocols (to be used as a bit mask). */
typedef enum {
   PROT_NONE       = 0,
   PROT_DELETE     = 1,
   PROT_TAKE_FOCUS = 2
} ClientProtocolType;

extern Atom atoms[ATOM_COUNT];

void InitializeHints();
void StartupHints();
void ShutdownHints();
void DestroyHints();

/** Determine the current desktop. */
void ReadCurrentDesktop();

/** Read client protocols
 * @param np The client.
 */
void ReadClientProtocols(struct ClientNode *np);

/** Read a client's name.
 * @param np The client.
 */
void ReadWMName(struct ClientNode *np);

/** Read a client's class.
 * @param np The client.
 */
void ReadWMClass(struct ClientNode *np);

/** Read normal hints for a client.
 * @param np The client.
 */
void ReadWMNormalHints(struct ClientNode *np);

/** Read the WM_PROTOCOLS property for a window.
 * @param w The window.
 * @return The protocols.
 */
ClientProtocolType ReadWMProtocols(Window w);

/** Read colormap information for a client.
 * @param np The client.
 */
void ReadWMColormaps(struct ClientNode *np);

/** Determine the layer of a client.
 * @param np The client.
 */
void ReadWinLayer(struct ClientNode *np);

/** Read the current state of a window.
 * @param win The window.
 * @return The window state.
 */
ClientState ReadWindowState(Window win);

/** Set the state of a client window.
 * @param np The client.
 */
void WriteState(struct ClientNode *np);

/** Read a cardinal atom.
 * @param window The window.
 * @param atom The atom to read.
 * @param value A pointer to the location to save the atom.
 * @return 1 on success, 0 on failure.
 */
int GetCardinalAtom(Window window, AtomType atom, unsigned long *value);

/** Read a window atom.
 * @param window The window.
 * @param atom The atom to read.
 * @param value A pointer to the location to save the atom.
 * @return 1 on success, 0 on failure.
 */
int GetWindowAtom(Window window, AtomType atom, Window *value);

/** Set a cardinal atom.
 * @param window The window.
 * @param atom The atom to set.
 * @param value The value.
 */
void SetCardinalAtom(Window window, AtomType atom, unsigned long value);

/** Set a window atom.
 * @param window The window.
 * @param atom The atom to set.
 * @param value The value.
 */
void SetWindowAtom(Window window, AtomType atom, unsigned long value);

/** Set a pixmap atom.
 * @param window The window.
 * @param atom The atom to set.
 * @param value The value.
 */
void SetPixmapAtom(Window window, AtomType atom, Pixmap value);

#endif /* HINT_H */

