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

#include "binding.h"

/** Enumeration of possible menu elements. */
typedef unsigned char MenuItemType;
#define MENU_ITEM_NORMAL      0  /**< Normal menu item (button). */
#define MENU_ITEM_SUBMENU     1  /**< Submenu. */
#define MENU_ITEM_SEPARATOR   2  /**< Item separator. */

/** Structure to represent a menu item. */
typedef struct MenuItem {

   MenuItemType type;         /**< The menu item type. */
   char *name;                /**< Name to display (or NULL). */
   ActionNode action;         /**< Action if selected. */
   char *iconName;            /**< Name of an icon to show (or NULL). */
   struct Menu *submenu;      /**< Submenu (or NULL). */
   struct MenuItem *next;     /**< Next item in the menu. */

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

/** Initialize a menu structure to be shown.
 * @param menu The menu to initialize.
 */
void InitializeMenu(Menu *menu);

/** Show a menu.
 * @param context The action context (including the menu position).
 * @param menu The menu to show.
 */
void ShowMenu(const struct ActionContext *context, Menu *menu);

/** Destroy a menu structure.
 * @param menu The menu to destroy.
 */
void DestroyMenu(Menu *menu);

/** The number of open menus. */
extern int menuShown;

#endif /* MENU_H */

