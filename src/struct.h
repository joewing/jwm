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
typedef struct IconNode {
	char *name;
	int scaled;
	int width;
	int height;
	Pixmap image;
	Pixmap mask;
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

	/* These fields are handled by menu.c */
	Window window;
	GC gc;
	int x, y;
	int width, height;
	int currentIndex, lastIndex;
	int itemHeight;
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

