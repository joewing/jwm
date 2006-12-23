/**
 * @file menu.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the menu functions.
 *
 */

#ifndef MENU_H
#define MENU_H

typedef enum {
	MA_NONE,
	MA_EXECUTE,
	MA_DESKTOP,
	MA_SENDTO,
	MA_LAYER,
	MA_STICK,
	MA_MAXIMIZE,
	MA_MINIMIZE,
	MA_RESTORE,
	MA_SHADE,
	MA_MOVE,
	MA_RESIZE,
	MA_KILL,
	MA_CLOSE,
	MA_EXIT,
	MA_RESTART
} MenuActionType;

typedef struct MenuAction {
	MenuActionType type;
	union {
		int i;
		char *str;
	} data;
} MenuAction;

typedef enum {
	MENU_ITEM_NORMAL,
	MENU_ITEM_SUBMENU,
	MENU_ITEM_SEPARATOR
} MenuItemType;

typedef struct MenuItem {

	MenuItemType type;
	char *name;
	MenuAction action;
	char *iconName;
	struct Menu *submenu;
	struct MenuItem *next;

	/* This field is handled by menu.c */
	struct IconNode *icon;

} MenuItem;

typedef struct Menu {

	/* These fields must be set before calling ShowMenu */
	struct MenuItem *items;
	char *label;
	int itemHeight;

	/* These fields are handled by menu.c */
	Window window;
	int x, y;
	int width, height;
	int currentIndex, lastIndex;
	unsigned int itemCount;
	int parentOffset;
	int textOffset;
	int *offsets;
	struct Menu *parent;

} Menu;

typedef void (*RunMenuCommandType)(const MenuAction *action);

void InitializeMenu(Menu *menu);
void ShowMenu(Menu *menu, RunMenuCommandType runner, int x, int y);
void DestroyMenu(Menu *menu);

extern int menuShown;

#endif

