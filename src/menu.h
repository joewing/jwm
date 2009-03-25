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
typedef enum {
   MA_NONE,
   MA_EXECUTE,
   MA_DESKTOP,
   MA_SENDTO,
   MA_LAYER,
   MA_STICK,
   MA_MAXIMIZE,
   MA_MAXIMIZE_H,
   MA_MAXIMIZE_V,
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
typedef enum {
   MENU_ITEM_NORMAL,       /**< Normal menu item (button). */
   MENU_ITEM_SUBMENU,      /**< Submenu. */
   MENU_ITEM_SEPARATOR     /**< Item separator. */
} MenuItemType;

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

/** Set the Menu opacity level.
 * @param str The value (ASCII).
 */
void SetMenuOpacity(const char *str);

#endif /* MENU_H */

