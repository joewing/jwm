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

/** Enumeration of menu action types. */
typedef unsigned char MenuActionType;
#define MA_NONE         0
#define MA_EXECUTE      1
#define MA_DESKTOP      2
#define MA_SENDTO       3
#define MA_LAYER        4
#define MA_STICK        5
#define MA_MAXIMIZE     6
#define MA_MAXIMIZE_H   7
#define MA_MAXIMIZE_V   8
#define MA_MINIMIZE     9
#define MA_RESTORE      10
#define MA_SHADE        11
#define MA_MOVE         12
#define MA_RESIZE       13
#define MA_KILL         14
#define MA_CLOSE        15
#define MA_EXIT         16
#define MA_RESTART      17

/** Structure to represent a menu action for callbacks. */
typedef struct MenuAction {

   MenuActionType type;    /**< Type of action. */

   /** Extra data for the action. */
   union {
      int i;
      char *str;
   } data;

} MenuAction;

/** Enumeration of possible menu elements. */
typedef unsigned char MenuItemType;
#define MENU_ITEM_NORMAL      0  /**< Normal menu item (button). */
#define MENU_ITEM_SUBMENU     1  /**< Submenu. */
#define MENU_ITEM_SEPARATOR   2  /**< Item separator. */

/** Structure to represent a menu item. */
typedef struct MenuItem {

   MenuItemType type;      /**< The menu item type. */
   char *name;             /**< Name to display (or NULL). */
   MenuAction action;      /**< Action to take if selected (or NULL). */
   char *iconName;         /**< Name of an icon to show (or NULL). */
   struct Menu *submenu;   /**< Submenu (or NULL). */
   struct MenuItem *next;  /**< Next item in the menu. */

   /** An icon for this menu item.
    * This field is handled by menu.c */
   struct IconNode *icon;  /**< Icon to display. */

} MenuItem;

/** Structure to represent a menu or submenu. */
typedef struct Menu {

   /* These fields must be set before calling ShowMenu */
   struct MenuItem *items; /**< Menu items. */
   char *label;            /**< Menu label (NULL for no label). */
   int itemHeight;         /**< User-specified menu item height. */

   /* These fields are handled by menu.c */
   Window window;          /**< The menu window. */
   int x;                  /**< The x-coordinate of the menu. */
   int y;                  /**< The y-coordinate of the menu. */
   int width;              /**< The width of the menu. */
   int height;             /**< The height of the menu. */
   int currentIndex;       /**< The current menu selection. */
   int lastIndex;          /**< The last menu selection. */
   unsigned int itemCount; /**< Number of menu items (excluding separators). */
   int parentOffset;       /**< y-offset of this menu wrt the parent. */
   int textOffset;         /**< x-offset of text in the menu. */
   int *offsets;           /**< y-offsets of menu items. */
   struct Menu *parent;    /**< The parent menu (or NULL). */

} Menu;

typedef void (*RunMenuCommandType)(const MenuAction *action);

/** Initialize a menu structure to be shown.
 * @param menu The menu to initialize.
 */
void InitializeMenu(Menu *menu);

/** Show a menu.
 * @param menu The menu to show.
 * @param runner Callback executed when an item is selected.
 * @param x The x-coordinate of the menu.
 * @param y The y-coordinate of the menu.
 */
void ShowMenu(Menu *menu, RunMenuCommandType runner, int x, int y);

/** Destroy a menu structure.
 * @param menu The menu to destroy.
 */
void DestroyMenu(Menu *menu);

/** The number of open menus. */
extern int menuShown;

#endif /* MENU_H */

