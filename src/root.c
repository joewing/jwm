/**
 * @file root.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions to handle root menus.
 *
 */

#include "jwm.h"
#include "root.h"
#include "menu.h"
#include "client.h"
#include "main.h"
#include "error.h"
#include "confirm.h"
#include "desktop.h"
#include "misc.h"
#include "winmenu.h"
#include "command.h"
#include "parse.h"
#include "settings.h"

/** Number of root menus to support. */
#define ROOT_MENU_COUNT 10

static Menu *rootMenu[ROOT_MENU_COUNT];

static void ExitHandler(ClientNode *np);
static void PatchRootMenu(Menu *menu);
static void UnpatchRootMenu(Menu *menu);

/** Initialize root menu data. */
void InitializeRootMenu()
{
   unsigned int x;
   for(x = 0; x < ROOT_MENU_COUNT; x++) {
      rootMenu[x] = NULL;
   }
}

/** Startup root menus. */
void StartupRootMenu()
{

   unsigned int x, y;
   char found;

   for(x = 0; x < ROOT_MENU_COUNT; x++) {
      if(rootMenu[x]) {
         found = 0;
         for(y = 0; y < x; y++) {
            if(rootMenu[y] == rootMenu[x]) {
               found = 1;
               break;
            }
         }
         if(!found) {
            InitializeMenu(rootMenu[x]);
         }
      }
   }

}

/** Destroy root menu data. */
void DestroyRootMenu()
{

   unsigned int x, y;

   for(x = 0; x < ROOT_MENU_COUNT; x++) {
      if(rootMenu[x]) {
         DestroyMenu(rootMenu[x]);
         for(y = x + 1; y < ROOT_MENU_COUNT; y++) {
            if(rootMenu[x] == rootMenu[y]) {
               rootMenu[y] = NULL;
            }
         }
         rootMenu[x] = NULL;
      }
   }

}

/** Set a root menu. */
void SetRootMenu(const char *indexes, Menu *m)
{

   unsigned int x, y;
   int index;
   char found;

   /* Loop over each index to consider. */
   for(x = 0; indexes[x]; x++) {

      /* Get the index and make sure it's in range. */
      index = indexes[x] - '0';
      if(JUNLIKELY(index < 0 || index >= ROOT_MENU_COUNT)) {
         Warning(_("invalid root menu specified: \"%c\""), indexes[x]);
         continue;
      }

      if(rootMenu[index] && rootMenu[index] != m) {

         /* See if replacing this value will cause an orphan. */
         found = 0;
         for(y = 0; y < ROOT_MENU_COUNT; y++) {
            if(x != y && rootMenu[y] == rootMenu[x]) {
               found = 1;
               break;
            }
         }

         /* If we have an orphan, destroy it. */
         if(!found) {
            DestroyMenu(rootMenu[index]);
         }

      }

      rootMenu[index] = m;

   }

}

/** Determine if the specified root menu is defined. */
char IsRootMenuDefined(int index)
{
   if(index >= 0 && index < ROOT_MENU_COUNT && rootMenu[index]) {
      return 1;
   } else {
      return 0;
   }
}

/** Determine the size of a root menu. */
void GetRootMenuSize(int index, int *width, int *height)
{

   if(!rootMenu[index]) {
      *width = 0;
      *height = 0;
      return;
   }

   PatchRootMenu(rootMenu[index]);
   *width = rootMenu[index]->width;
   *height = rootMenu[index]->height;
   UnpatchRootMenu(rootMenu[index]);

}

/** Show a root menu. */
char ShowRootMenu(int index, int x, int y)
{

   ActionContext ac;

   if(!rootMenu[index]) {
      return 0;
   }

   InitActionContext(&ac);
   ac.x = x;
   ac.y = y;

   PatchRootMenu(rootMenu[index]);
   ShowMenu(&ac, rootMenu[index]);
   UnpatchRootMenu(rootMenu[index]);

   return 1;

}

/** Prepare a root menu to be shown. */
void PatchRootMenu(Menu *menu)
{

   MenuItem *item;

   for(item = menu->items; item; item = item->next) {
      if(item->submenu) {
         PatchRootMenu(item->submenu);
      }
      if(item->action.type == ACTION_DESKTOP && item->action.value < 0) {
         item->submenu = CreateDesktopMenu(1 << currentDesktop);
         InitializeMenu(item->submenu);
      }
   }

}

/** Remove temporary items from a root menu. */
void UnpatchRootMenu(Menu *menu) {

   MenuItem *item;

   for(item = menu->items; item; item = item->next) {
      if(item->action.type == ACTION_DESKTOP && item->action.value < 0) {
         DestroyMenu(item->submenu);
         item->submenu = NULL;
      } else if(item->submenu) {
         UnpatchRootMenu(item->submenu);
      }
   }

}

/** Exit callback for the exit menu item. */
void ExitHandler(ClientNode *np)
{
   shouldExit = 1;
}

/** Restart callback for the restart menu item. */
void Restart()
{
   shouldRestart = 1;
   shouldExit = 1;
}

/** Exit with optional confirmation. */
void Exit()
{
   if(settings.exitConfirmation) {
      ShowConfirmDialog(NULL, ExitHandler,
                        _("Exit JWM"),
                        _("Are you sure?"),
                        NULL);
   } else {
      ExitHandler(NULL);
   }
}

/** Reload the menu. */
void ReloadMenu()
{
   shouldReload = 1;
   if(!menuShown) {
      ShutdownRootMenu();
      DestroyRootMenu();
      InitializeRootMenu();
      ParseConfig(configPath);
      StartupRootMenu();
      shouldReload = 0;
   }
}

