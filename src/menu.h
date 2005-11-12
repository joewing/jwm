/***************************************************************************
 * Header for the menu functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ***************************************************************************/

#ifndef MENU_H
#define MENU_H

typedef enum {
	MENU_ITEM_NORMAL   = 0,
	MENU_ITEM_DESKTOPS = 1
} MenuItemFlags;

typedef struct MenuItemType {

	char *name;
	char *command;
	char *iconName;
	struct IconNode *icon;  /* This field is handled by menu.c */
	struct MenuItemType *next;
	struct MenuType *submenu;
	MenuItemFlags flags;

} MenuItemType;

typedef struct MenuType {

	/* These fields must be set before calling ShowMenu */
	struct MenuItemType *items;
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

typedef void (*RunMenuCommandType)(const char *command);

void InitializeMenu(MenuType *menu);
void ShowMenu(MenuType *menu, RunMenuCommandType runner, int x, int y);
void DestroyMenu(MenuType *menu);

extern int menuShown;

#endif

