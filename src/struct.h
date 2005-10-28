/****************************************************************************
 * Structures used in JWM.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef STRUCT_H
#define STRUCT_H

/****************************************************************************
 ****************************************************************************/
typedef struct {
	int minx, maxx;
	int miny, maxy;
} AspectRatio;

/****************************************************************************
 ****************************************************************************/
typedef struct ColormapNode {
	Window window;
	struct ColormapNode *next;
} ColormapNode;

/****************************************************************************
 ****************************************************************************/
typedef struct ImageNode {
#ifdef USE_PNG
	png_uint_32 width;
	png_uint_32 height;
#else
	int width;
	int height;
#endif
	unsigned long *data;
} ImageNode;

/****************************************************************************
 ****************************************************************************/
typedef struct ScaledIconNode {

	int width;
	int height;

	Pixmap image;
	Pixmap mask;
#ifdef USE_XRENDER
	Picture imagePicture;
	Picture maskPicture;
#endif

	struct ScaledIconNode *next;

} ScaledIconNode;

/****************************************************************************
 ****************************************************************************/
typedef struct IconNode {

	char *name;

	ImageNode *image;

	ScaledIconNode *nodes;
	
	struct IconNode *next;
	struct IconNode *prev;

} IconNode;

/****************************************************************************
 ****************************************************************************/
typedef struct ClientNode {
	Window window;
	Window parent;
	GC parentGC;
	int desktop;

	Window owner;

	int x, y, width, height;
	int oldx, oldy, oldWidth, oldHeight;

	long sizeFlags;
	int baseWidth, baseHeight;
	int minWidth, minHeight;
	int maxWidth, maxHeight;
	int xinc, yinc;
	AspectRatio aspect;
	int gravity;

	Colormap cmap;
	ColormapNode *colormaps;

	char *name;
	char *instanceName;
	char *className;

	BorderFlags borderFlags;
	BorderActionType borderAction;

	ClientProtocolType protocols;
	StatusFlags statusFlags;
	int layer;

	IconNode *icon;

	void (*controller)(int wasDestroyed);

	struct ClientNode *prev;
	struct ClientNode *next;
} ClientNode;

/****************************************************************************
 ****************************************************************************/
typedef struct TrayComponentType {

	/* The tray containing the component.
	 * UpdateSpecificTray(TrayType*, TrayComponentType*) should be called
	 * when content changes.
	 */
	struct TrayType *tray;

	/* Additional information needed for the component. */
	void *object;

	/* Coordinates on the tray (valid only after Create). */
	int x, y;

	/* Coordinates on the screen (valid only after Create). */
	int screenx, screeny;

	/* Component size.
	 * Sizing is handled as follows:
	 *  - The component is created via a factory method. It sets its
	 *    preferred size (0 for no preference).
	 *  - The SetSize callback is issued with size constraints
	 *    (0 for no constraint). The component should update its preference
	 *    during SetSize.
	 *  - The Create callback is issued with finalized size information.
	 */
	int width, height;

	/* Content. */
	Window window;
	Pixmap pixmap;

	/* Create the component. */
	void (*Create)(struct TrayComponentType *cp);

	/* Destroy the component. */
	void (*Destroy)(struct TrayComponentType *cp);

	/* Set the size known so far for items that need width/height ratios.
	 * Either width or height may be zero.
	 * This is called before Create.
	 */
	void (*SetSize)(struct TrayComponentType *cp, int width, int height);

	/* Callback for mouse clicks. */
	void (*ProcessButtonEvent)(struct TrayComponentType *cp,
		int x, int y, int mask);

	/* Callback for mouse motion. */
	void (*ProcessMotionEvent)(struct TrayComponentType *cp,
		int x, int y, int mask);

	/* The next component in the tray. */
	struct TrayComponentType *next;

} TrayComponentType;

/****************************************************************************
 ****************************************************************************/
typedef struct TrayType {

	int x, y;
	int width, height;
	int border;
	WinLayerType layer;
	LayoutType layout;

	int autoHide;
	int hidden;

	Window window;
	GC gc;

	TrayComponentType *components;
	TrayComponentType *componentsTail;

	struct TrayType *next;

} TrayType;

/****************************************************************************
 ****************************************************************************/
typedef struct MenuItemType {
	char *name;
	char *command;
	char *iconName;
	IconNode *icon;  /* This field is handled by menu.c */
	struct MenuItemType *next;
	void *submenu;  /* struct MenuType* */
} MenuItemType;

/****************************************************************************
 ****************************************************************************/
typedef struct MenuType {
	/* These fields must be set before calling ShowMenu */
	MenuItemType *items;
	char *label;
	int itemHeight;

	/* These fields are handled by menu.c */
	Window window;
	GC gc;
	int x, y;
	int width, height;
	int currentIndex, lastIndex;
	int itemCount;
	int parentOffset;
	int wasCovered;
	int textOffset;
	int *offsets;
	struct MenuType *parent;
} MenuType;

/****************************************************************************
 ****************************************************************************/
typedef struct AttributeNode {
	char *name;
	char *value;
	struct AttributeNode *next;
} AttributeNode;

/****************************************************************************
 ****************************************************************************/
typedef struct TokenNode {
	TokenType type;
	char *value;
	AttributeNode *attributes;
	struct TokenNode *parent;
	struct TokenNode *subnodeHead, *subnodeTail;
	struct TokenNode *next;
} TokenNode;

/****************************************************************************
 ****************************************************************************/
typedef struct {
	unsigned long seconds;
	int ms;
} TimeType;

/****************************************************************************
 ****************************************************************************/
typedef struct {
	unsigned long flags;
	unsigned long functions;
	unsigned long decorations;
	long          inputMode;
	unsigned long status;
} PropMwmHints;

#endif

