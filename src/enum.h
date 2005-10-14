/****************************************************************************
 * Enumerations used in JWM.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef ENUM_H
#define ENUM_H

/****************************************************************************
 ****************************************************************************/
typedef enum {

	/* Misc */
	ATOM_COMPOUND_TEXT,

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
	ATOM_NET_DESKTOP_GEOMETRY,
	ATOM_NET_DESKTOP_VIEWPORT,
	ATOM_NET_CURRENT_DESKTOP,
	ATOM_NET_ACTIVE_WINDOW,
	ATOM_NET_WORKAREA,
	ATOM_NET_SUPPORTING_WM_CHECK,
	ATOM_NET_WM_DESKTOP,
	ATOM_NET_WM_STATE,
	ATOM_NET_WM_STATE_STICKY,
	ATOM_NET_WM_NAME,
	ATOM_NET_WM_ICON,
	ATOM_NET_WM_WINDOW_TYPE,
	ATOM_NET_WM_WINDOW_TYPE_DESKTOP,

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
#define LAST_NET_ATOM  ATOM_NET_WM_WINDOW_TYPE_DESKTOP

#define FIRST_WIN_ATOM ATOM_WIN_LAYER
#define LAST_WIN_ATOM  ATOM_WIN_PROTOCOLS

#define FIRST_MWM_ATOM ATOM_MOTIF_WM_HINTS
#define LAST_MWM_ATOM  ATOM_MOTIF_WM_HINTS

/****************************************************************************
 ****************************************************************************/
typedef enum {
	PROT_NONE       = 0,
	PROT_DELETE     = 1,
	PROT_TAKE_FOCUS = 2
} ClientProtocolType;

/****************************************************************************
 ****************************************************************************/
typedef enum {
	BORDER_NONE    = 0,
	BORDER_OUTLINE = 1,
	BORDER_TITLE   = 2,
	BORDER_MIN     = 4,
	BORDER_MAX     = 8,
	BORDER_CLOSE   = 16,
	BORDER_RESIZE  = 32,
	BORDER_MOVE    = 64
} BorderFlags;

/****************************************************************************
 ****************************************************************************/
typedef enum {
	STAT_NONE      = 0,
	STAT_ACTIVE    = 1 << 0,
	STAT_MAPPED    = 1 << 1,
	STAT_MAXIMIZED = 1 << 2,
	STAT_HIDDEN    = 1 << 3,
	STAT_STICKY    = 1 << 4,
	STAT_NOLIST    = 1 << 5,
	STAT_WITHDRAWN = 1 << 6,
	STAT_MINIMIZED = 1 << 7,
	STAT_SHADED    = 1 << 8,
	STAT_USESHAPE  = 1 << 9,
	STAT_WMDIALOG  = 1 << 10
} StatusFlags;

/****************************************************************************
 ****************************************************************************/
typedef enum {
	BA_NONE      = 0,
	BA_RESIZE    = 1,
	BA_MOVE      = 2,
	BA_CLOSE     = 3,
	BA_MAXIMIZE  = 4,
	BA_MINIMIZE  = 5,
	BA_RESIZE_N  = 0x10,
	BA_RESIZE_S  = 0x20,
	BA_RESIZE_E  = 0x40,
	BA_RESIZE_W  = 0x80
} BorderActionType;

/****************************************************************************
 * Note: Any change made to this typedef must be reflected in
 * TOKEN_MAP in lex.c.
 ****************************************************************************/
typedef enum {

	TOK_INVALID,

	TOK_ACTIVEBACKGROUND,
	TOK_ACTIVEFOREGROUND,
	TOK_BACKGROUND,
	TOK_BORDERSTYLE,
	TOK_CLASS,
	TOK_DESKTOPCOUNT,
	TOK_DOUBLECLICKSPEED,
	TOK_DOUBLECLICKDELTA,
	TOK_EXIT,
	TOK_FOCUSMODEL,
	TOK_FONT,
	TOK_FOREGROUND,
	TOK_GROUP,
	TOK_HEIGHT,
	TOK_ICONPATH,
	TOK_ICONS,
	TOK_INCLUDE,
	TOK_JWM,
	TOK_KEY,
	TOK_MENU,
	TOK_MENUSTYLE,
	TOK_MOVEMODE,
	TOK_NAME,
	TOK_OPTION,
	TOK_OUTLINE,
	TOK_PAGER,
	TOK_PAGERSTYLE,
	TOK_POPUP,
	TOK_POPUPSTYLE,
	TOK_PROGRAM,
	TOK_RESIZEMODE,
	TOK_RESTART,
	TOK_ROOTMENU,
	TOK_SEPARATOR,
	TOK_SHUTDOWNCOMMAND,
	TOK_SNAPMODE,
	TOK_STARTUPCOMMAND,
	TOK_SWALLOW,
	TOK_TASKLISTSTYLE,
	TOK_TASKLIST,
	TOK_TRAY,
	TOK_TRAYBUTTON,
	TOK_TRAYSTYLE,
	TOK_WIDTH

} TokenType;

/****************************************************************************
 ****************************************************************************/
typedef enum {

	COLOR_BORDER_BG,
	COLOR_BORDER_FG,
	COLOR_BORDER_ACTIVE_BG,
	COLOR_BORDER_ACTIVE_FG,

	COLOR_TRAY_BG,

	COLOR_TASK_BG,
	COLOR_TASK_FG,
	COLOR_TASK_ACTIVE_BG,
	COLOR_TASK_ACTIVE_FG,

	COLOR_PAGER_BG,
	COLOR_PAGER_FG,
	COLOR_PAGER_ACTIVE_BG,
	COLOR_PAGER_ACTIVE_FG,
	COLOR_PAGER_OUTLINE,

	COLOR_MENU_BG,
	COLOR_MENU_FG,
	COLOR_MENU_ACTIVE_BG,
	COLOR_MENU_ACTIVE_FG,

	COLOR_BORDER_UP,
	COLOR_BORDER_DOWN,
	COLOR_BORDER_ACTIVE_UP,
	COLOR_BORDER_ACTIVE_DOWN,

	COLOR_TRAY_UP,
	COLOR_TRAY_DOWN,

	COLOR_TASK_UP,
	COLOR_TASK_DOWN,
	COLOR_TASK_ACTIVE_UP,
	COLOR_TASK_ACTIVE_DOWN,

	COLOR_MENU_UP,
	COLOR_MENU_DOWN,
	COLOR_MENU_ACTIVE_UP,
	COLOR_MENU_ACTIVE_DOWN,

	COLOR_POPUP_BG,
	COLOR_POPUP_FG,
	COLOR_POPUP_OUTLINE,

	COLOR_COUNT

} ColorType;

/****************************************************************************
 ****************************************************************************/
typedef enum {

	FONT_BORDER,
	FONT_MENU,
	FONT_TASK,
	FONT_POPUP,

	FONT_COUNT

} FontType;

/****************************************************************************
 ****************************************************************************/
typedef enum {
	BUTTON_MENU,
	BUTTON_MENU_ACTIVE,
	BUTTON_TASK,
	BUTTON_TASK_ACTIVE
} ButtonType;

/****************************************************************************
 ****************************************************************************/
typedef enum {
	ALIGN_LEFT,
	ALIGN_CENTER,
	ALIGN_RIGHT
} AlignmentType;

/****************************************************************************
 ****************************************************************************/
typedef enum {
	WIN_STATE_STICKY          = 1 << 0,
	WIN_STATE_MINIMIZED       = 1 << 1,
	WIN_STATE_MAXIMIZED_VERT  = 1 << 2,
	WIN_STATE_MAXIMIZED_HORIZ = 1 << 3,
	WIN_STATE_HIDDEN          = 1 << 4,
	WIN_STATE_SHADED          = 1 << 5,
	WIN_STATE_HIDE_WORKSPACE  = 1 << 6,
	WIN_STATE_HIDE_TRANSIENT  = 1 << 7,
	WIN_STATE_FIXED_POSITION  = 1 << 8,
	WIN_STATE_ARRANGE_IGNORE  = 1 << 9
} WinStateType;

/****************************************************************************
 ****************************************************************************/
typedef enum {
	WIN_HINT_SKIP_FOCUS       = 1 << 0,
	WIN_HINT_SKIP_WINLIST     = 1 << 1,
	WIN_HINT_SKIP_TASKBAR     = 1 << 2,
	WIN_HINT_GROUP_TRANSIENT  = 1 << 3,
	WIN_HINT_FOCUS_ON_CLICK   = 1 << 4
} WinHintType;

/****************************************************************************
 ****************************************************************************/
typedef enum {
	LAYER_BOTTOM              = 0,
	LAYER_NORMAL              = 4,
	DEFAULT_TRAY_LAYER        = 8,
	LAYER_TOP                 = 12,
	LAYER_COUNT               = 13
} WinLayerType;

/****************************************************************************
 ****************************************************************************/
typedef enum {
	FOCUS_SLOPPY              = 0,
	FOCUS_CLICK               = 1
} FocusModelType;

/****************************************************************************
 ****************************************************************************/
typedef enum {
	SNAP_NONE                 = 0,
	SNAP_SCREEN               = 1,
	SNAP_BORDER               = 2
} SnapModeType;

/****************************************************************************
 * Note: extra information about the key press is stored << 8.
 * (For example, a grab of '#' will grab any number key 1-9 and pass
 * the offset (1-9) in the upper byte).
 ****************************************************************************/
typedef enum {
	KEY_NONE    = 0,
	KEY_UP      = 1,
	KEY_DOWN    = 2,
	KEY_RIGHT   = 3,
	KEY_LEFT    = 4,
	KEY_ESC     = 5,
	KEY_ENTER   = 6,
	KEY_NEXT    = 7,
	KEY_CLOSE   = 8,
	KEY_MIN     = 9,
	KEY_MAX     = 10,
	KEY_SHADE   = 11,
	KEY_MOVE    = 12,
	KEY_RESIZE  = 13,
	KEY_ROOT    = 14,
	KEY_WIN     = 15,
	KEY_DESKTOP = 16,
	KEY_EXEC    = 17,
	KEY_RESTART = 18,
	KEY_EXIT    = 19
} KeyType;

/****************************************************************************
 ****************************************************************************/
typedef enum {
	OPTION_INVALID  = 0,
	OPTION_STICKY   = 1,
	OPTION_LAYER    = 2,
	OPTION_DESKTOP  = 3,
	OPTION_ICON     = 4,
	OPTION_NOLIST   = 5,
	OPTION_BORDER   = 6,
	OPTION_NOBORDER = 7,
	OPTION_TITLE    = 8,
	OPTION_NOTITLE  = 9
} OptionType;

/****************************************************************************
 ****************************************************************************/
typedef enum {
	MOVE_OPAQUE,
	MOVE_OUTLINE
} MoveModeType;

/****************************************************************************
 ****************************************************************************/
typedef enum {
	RESIZE_OPAQUE,
	RESIZE_OUTLINE
} ResizeModeType;

/****************************************************************************
 ****************************************************************************/
typedef enum {
	SWALLOW_LEFT,
	SWALLOW_RIGHT
} SwallowLocationType;

/****************************************************************************
 ****************************************************************************/
typedef enum {
	LAYOUT_HORIZONTAL,
	LAYOUT_VERTICAL
} LayoutType;

#endif

