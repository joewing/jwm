/**
 * @file menu.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Menu display and handling functions.
 *
 */

#include "jwm.h"
#include "menu.h"
#include "font.h"
#include "client.h"
#include "icon.h"
#include "cursor.h"
#include "key.h"
#include "button.h"
#include "event.h"
#include "root.h"
#include "settings.h"
#include "desktop.h"
#include "parse.h"
#include "winmenu.h"
#include "screen.h"

#define BASE_ICON_OFFSET   3
#define MENU_BORDER_SIZE   1

typedef unsigned char MenuSelectionType;
#define MENU_NOSELECTION   0
#define MENU_LEAVE         1
#define MENU_SUBSELECT     2

static char ShowSubmenu(Menu *menu, Menu *parent,
                        RunMenuCommandType runner,
                        int x, int y);

static void PatchMenu(Menu *menu);
static void UnpatchMenu(Menu *menu);
static void CreateMenu(Menu *menu, int x, int y);
static void HideMenu(Menu *menu);
static void DrawMenu(Menu *menu);

static char MenuLoop(Menu *menu, RunMenuCommandType runner);
static MenuSelectionType UpdateMotion(Menu *menu,
                                      RunMenuCommandType runner,
                                      XEvent *event);

static void UpdateMenu(Menu *menu);
static void DrawMenuItem(Menu *menu, MenuItem *item, int index);
static MenuItem *GetMenuItem(Menu *menu, int index);
static int GetNextMenuIndex(Menu *menu);
static int GetPreviousMenuIndex(Menu *menu);
static int GetMenuIndex(Menu *menu, int index);
static void SetPosition(Menu *tp, int index);
static char IsMenuValid(const Menu *menu);

int menuShown = 0;

/** Create an empty menu item. */
MenuItem *CreateMenuItem(MenuItemType type)
{
   MenuItem *item = Allocate(sizeof(MenuItem));
   memset(item, 0, sizeof(MenuItem));
   item->type = type;
   return item;
}

/** Initialize a menu. */
void InitializeMenu(Menu *menu)
{

   MenuItem *np;
   int index, temp;
   int userHeight;
   int hasSubmenu;
   char hasIcon;

   menu->textOffset = 0;
   menu->itemCount = 0;

   /* Compute the max size needed */
   hasIcon = 0;
   userHeight = menu->itemHeight;
   if(userHeight < 0) {
      userHeight = 0;
   }
   menu->itemHeight = GetStringHeight(FONT_MENU);
   for(np = menu->items; np; np = np->next) {
      if(np->iconName) {
         np->icon = LoadNamedIcon(np->iconName, 1, 1);
         if(np->icon) {
            hasIcon = 1;
         }
      } else if(np->icon) {
         hasIcon = 1;
      }
      menu->itemCount += 1;
   }
   menu->itemHeight += BASE_ICON_OFFSET * 2;

   if(userHeight) {
      menu->itemHeight = userHeight + BASE_ICON_OFFSET * 2;
   }
   if(hasIcon) {
      menu->textOffset = menu->itemHeight + BASE_ICON_OFFSET * 2;
   }

   menu->width = 5;
   menu->parent = NULL;
   menu->parentOffset = 0;

   /* Make sure the menu is wide enough for a label if it is labeled. */
   if(menu->label) {
      temp = GetStringWidth(FONT_MENU, menu->label);
      if(temp > menu->width) {
         menu->width = temp;
      }
   }

   menu->height = MENU_BORDER_SIZE;
   if(menu->label) {
      menu->height += menu->itemHeight;
   }

   /* Nothing else to do if there is nothing in the menu. */
   if(JUNLIKELY(menu->itemCount == 0)) {
      return;
   }

   menu->offsets = Allocate(sizeof(int) * menu->itemCount);

   hasSubmenu = 0;
   index = 0;
   for(np = menu->items; np; np = np->next) {
      menu->offsets[index++] = menu->height;
      if(np->type == MENU_ITEM_SEPARATOR) {
         menu->height += 6;
      } else {
         menu->height += menu->itemHeight;
      }
      if(np->name) {
         temp = GetStringWidth(FONT_MENU, np->name);
         if(temp > menu->width) {
            menu->width = temp;
         }
      }
      if(hasIcon && !np->icon) {
         np->icon = &emptyIcon;
      }
      if(np->submenu) {
         hasSubmenu = (menu->itemHeight + 3) / 4;
         InitializeMenu(np->submenu);
      }
   }
   menu->width += hasSubmenu + menu->textOffset;
   menu->width += 7 + 2 * MENU_BORDER_SIZE;
   menu->height += MENU_BORDER_SIZE;

}

/** Show a menu. */
void ShowMenu(Menu *menu, RunMenuCommandType runner, int x, int y)
{
   int mouseStatus, keyboardStatus;

   /* Don't show the menu if there isn't anything to show. */
   if(JUNLIKELY(!IsMenuValid(menu))) {
      return;
   }
   if(JUNLIKELY(shouldExit)) {
      return;
   }

   mouseStatus = GrabMouse(rootWindow);
   keyboardStatus = JXGrabKeyboard(display, rootWindow, False,
                                   GrabModeAsync, GrabModeAsync, CurrentTime);
   if(JUNLIKELY(!mouseStatus || keyboardStatus != GrabSuccess)) {
      return;
   }

   ShowSubmenu(menu, NULL, runner, x, y);
   UnpatchMenu(menu);

   JXUngrabKeyboard(display, CurrentTime);
   JXUngrabPointer(display, CurrentTime);
   RefocusClient();

   if(shouldReload) {
      ReloadMenu();
   }

}

/** Destroy a menu. */
void DestroyMenu(Menu *menu)
{
   MenuItem *np;
   if(menu) {
      while(menu->items) {
         np = menu->items->next;
         if(menu->items->name) {
            Release(menu->items->name);
         }
         switch(menu->items->action.type & MA_ACTION_MASK) {
         case MA_EXECUTE:
         case MA_EXIT:
         case MA_DYNAMIC:
            if(menu->items->action.str) {
               Release(menu->items->action.str);
            }
            break;
         default:
            break;
         }
         if(menu->items->iconName) {
            Release(menu->items->iconName);
         }
         if(menu->items->submenu) {
            DestroyMenu(menu->items->submenu);
         }
         Release(menu->items);
         menu->items = np;
      }
      if(menu->label) {
         Release(menu->label);
      }
      if(menu->offsets) {
         Release(menu->offsets);
      }
      Release(menu);
   }
}

/** Show a submenu. */
char ShowSubmenu(Menu *menu, Menu *parent,
                 RunMenuCommandType runner,
                 int x, int y)
{

   char status;

   PatchMenu(menu);
   menu->parent = parent;
   CreateMenu(menu, x, y);

   menuShown += 1;
   status = MenuLoop(menu, runner);
   menuShown -= 1;

   HideMenu(menu);

   return status;

}

/** Prepare a menu to be shown. */
void PatchMenu(Menu *menu)
{
   MenuItem *item;
   for(item = menu->items; item; item = item->next) {
      Menu *submenu = NULL;
      switch(item->action.type & MA_ACTION_MASK) {
      case MA_DESKTOP_MENU:
         submenu = CreateDesktopMenu(1 << currentDesktop,
                                     item->action.context);
         break;
      case MA_SENDTO_MENU:
         submenu = CreateSendtoMenu(
            item->action.type & ~MA_ACTION_MASK,
            item->action.context);
         break;
      case MA_WINDOW_MENU:
         submenu = CreateWindowMenu(item->action.context);
         break;
      case MA_DYNAMIC:
         if(!item->submenu) {
            submenu = ParseDynamicMenu(item->action.str);
            submenu->itemHeight = item->action.value;
         }
         break;
      default:
         break;
      }
      if(submenu) {
         InitializeMenu(submenu);
         item->submenu = submenu;
      }
   }
}

/** Remove temporary items from a menu. */
void UnpatchMenu(Menu *menu)
{
   MenuItem *item;
   for(item = menu->items; item; item = item->next) {
      if(item->submenu) {
         UnpatchMenu(item->submenu);
         switch(item->action.type & MA_ACTION_MASK) {
         case MA_DESKTOP_MENU:
         case MA_SENDTO_MENU:
         case MA_WINDOW_MENU:
         case MA_DYNAMIC:
            DestroyMenu(item->submenu);
            item->submenu = NULL;
            break;
         default:
            break;
         }
      }
   }
}

/** Menu process loop.
 * Returns 0 if no selection was made or 1 if a selection was made.
 */
char MenuLoop(Menu *menu, RunMenuCommandType runner)
{

   XEvent event;
   MenuItem *ip;
   Window pressw;
   int pressx, pressy;
   char hadMotion;

   hadMotion = 0;

   GetMousePosition(&pressx, &pressy, &pressw);

   for(;;) {

      WaitForEvent(&event);

      switch(event.type) {
      case Expose:
         if(event.xexpose.count == 0) {
            Menu *mp = menu;
            while(mp) {
               if(mp->window == event.xexpose.window) {
                  DrawMenu(mp);
                  break;
               }
               mp = mp->parent;
            }
         }
         break;

      case ButtonPress:

         pressx = -100;
         pressy = -100;

      case KeyPress:
      case MotionNotify:
         hadMotion = 1;
         switch(UpdateMotion(menu, runner, &event)) {
         case MENU_NOSELECTION: /* no selection */
            break;
         case MENU_LEAVE: /* mouse left the menu */
            JXPutBackEvent(display, &event);
            return 0;
         case MENU_SUBSELECT: /* selection made */
            return 1;
         }
         break;

      case ButtonRelease:

         if(event.xbutton.button == Button4) {
            break;
         }
         if(event.xbutton.button == Button5) {
            break;
         }
         if(!hadMotion) {
            break;
         }
         if(abs(event.xbutton.x_root - pressx) < settings.doubleClickDelta) {
            if(abs(event.xbutton.y_root - pressy) < settings.doubleClickDelta) {
               break;
            }
         }

         if(menu->parent && menu->currentIndex < 0) {
            ip = GetMenuItem(menu->parent, menu->parent->currentIndex);
         } else {
            ip = GetMenuItem(menu, menu->currentIndex);
         }
         if(ip != NULL) {
            (runner)(&ip->action, event.xbutton.button);
         }
         return 1;
      default:
         break;
      }

   }
}

/** Create and map a menu. */
void CreateMenu(Menu *menu, int x, int y)
{

   XSetWindowAttributes attr;
   unsigned long attrMask;
   int temp;

   menu->lastIndex = -1;
   menu->currentIndex = -1;

   if(menu->parent) {
      menu->screen = menu->parent->screen;
   } else {
      menu->screen = GetCurrentScreen(x + menu->width / 2,
                                      y + menu->height / 2);
   }
   if(x + menu->width > menu->screen->x + menu->screen->width) {
      if(menu->parent) {
         x = menu->parent->x - menu->width;
      } else {
         x = menu->screen->x + menu->screen->width - menu->width;
      }
   }
   temp = y;
   if(y + menu->height > menu->screen->y + menu->screen->height) {
      y = menu->screen->y + menu->screen->height - menu->height;
   }
   if(y < 0) {
      y = 0;
   }

   menu->x = x;
   menu->y = y;
   menu->parentOffset = temp - y;

   attrMask = 0;

   attrMask |= CWEventMask;
   attr.event_mask = ExposureMask;

   attrMask |= CWBackPixel;
   attr.background_pixel = colors[COLOR_MENU_BG];

   attrMask |= CWSaveUnder;
   attr.save_under = True;

   menu->window = JXCreateWindow(display, rootWindow, x, y,
                                 menu->width, menu->height, 0,
                                 CopyFromParent, InputOutput,
                                 CopyFromParent, attrMask, &attr);
   menu->pixmap = JXCreatePixmap(display, menu->window,
                                 menu->width, menu->height, rootVisual.depth);

   if(settings.menuOpacity < UINT_MAX) {
      SetCardinalAtom(menu->window, ATOM_NET_WM_WINDOW_OPACITY,
                      settings.menuOpacity);
   }

   JXMapRaised(display, menu->window); 

}

/** Hide a menu. */
void HideMenu(Menu *menu)
{
   JXDestroyWindow(display, menu->window);
   JXFreePixmap(display, menu->pixmap);
}

/** Draw a menu. */
void DrawMenu(Menu *menu)
{

   MenuItem *np;
   int x;

   JXSetForeground(display, rootGC, colors[COLOR_MENU_BG]);
   JXFillRectangle(display, menu->pixmap, rootGC, 0, 0,
                   menu->width, menu->height);

   JXSetForeground(display, rootGC, colors[COLOR_MENU_UP]);
   JXDrawLine(display, menu->pixmap, rootGC,
              0, 0, menu->width, 0);
   JXDrawLine(display, menu->pixmap, rootGC,
              0, 0, 0, menu->height);

   JXSetForeground(display, rootGC, colors[COLOR_MENU_DOWN]);
   JXDrawLine(display, menu->pixmap, rootGC,
              0, menu->height - 1, menu->width, menu->height - 1);
   JXDrawLine(display, menu->pixmap, rootGC,
              menu->width - 1, 0, menu->width - 1, menu->height);

   if(menu->label) {
      DrawMenuItem(menu, NULL, -1);
   }

   x = 0;
   for(np = menu->items; np; np = np->next) {
      DrawMenuItem(menu, np, x);
      ++x;
   }
   JXCopyArea(display, menu->pixmap, menu->window, rootGC,
              0, 0, menu->width, menu->height, 0, 0);

}

/** Determine the action to take given an event. */
MenuSelectionType UpdateMotion(Menu *menu,
                               RunMenuCommandType runner,
                               XEvent *event)
{
   MenuItem *ip;
   Menu *tp;
   Window subwindow;
   int x, y;

   if(event->type == MotionNotify) {

      SetMousePosition(event->xmotion.x_root, event->xmotion.y_root,
                       event->xmotion.window);
      DiscardMotionEvents(event, menu->window);

      x = event->xmotion.x_root - menu->x;
      y = event->xmotion.y_root - menu->y;
      subwindow = event->xmotion.subwindow;

   } else if(event->type == ButtonPress) {

      if(menu->currentIndex >= 0 || !menu->parent) {
         tp = menu;
      } else {
         tp = menu->parent;
      }

      y = -1;
      if(event->xbutton.button == Button4) {
         y = GetPreviousMenuIndex(tp);
      } else if(event->xbutton.button == Button5) {
         y = GetNextMenuIndex(tp);
      }

      if(y >= 0) {
         SetPosition(tp, y);
      }

      return MENU_NOSELECTION;

   } else if(event->type == KeyPress) {

      if(menu->currentIndex >= 0 || !menu->parent) {
         tp = menu;
      } else {
         tp = menu->parent;
      }

      y = -1;
      switch(GetKey(&event->xkey) & 0xFF) {
      case KEY_UP:
         y = GetPreviousMenuIndex(tp);
         break;
      case KEY_DOWN:
         y = GetNextMenuIndex(tp);
         break;
      case KEY_RIGHT:
         tp = menu;
         y = 0;
         break;
      case KEY_LEFT:
         if(tp->parent) {
            tp = tp->parent;
            if(tp->currentIndex >= 0) {
               y = tp->currentIndex;
            } else {
               y = 0;
            }
         }
         break;
      case KEY_ESC:
         return MENU_SUBSELECT;
      case KEY_ENTER:
         ip = GetMenuItem(tp, tp->currentIndex);
         if(ip != NULL) {
            (runner)(&ip->action, 0);
         }
         return MENU_SUBSELECT;
      default:
         break;
      }

      if(y >= 0) {
         SetPosition(tp, y);
      }

      return MENU_NOSELECTION;

   } else {
      Debug("invalid event type in menu.c:UpdateMotion");
      return MENU_SUBSELECT;
   }

   /* Update the selection on the current menu */
   if(x > 0 && y > 0 && x < menu->width && y < menu->height) {
      menu->currentIndex = GetMenuIndex(menu, y);
   } else if(menu->parent && subwindow != menu->parent->window) {

      /* Leave if over a menu window. */
      for(tp = menu->parent->parent; tp; tp = tp->parent) {
         if(tp->window == subwindow) {
            return MENU_LEAVE;
         }
      }
      menu->currentIndex = -1;

   } else {

      /* Leave if over the parent, but not on this selection. */
      tp = menu->parent;
      if(tp && subwindow == tp->window) {
         if(y < menu->parentOffset
            || y > tp->itemHeight + menu->parentOffset) {
            return MENU_LEAVE;
         }
      }

      menu->currentIndex = -1;

   }

   /* Move the menu if needed. */
   if(menu->height > menu->screen->height && menu->currentIndex >= 0) {

      /* If near the top, shift down. */
      if(y + menu->y <= 0) {
         if(menu->currentIndex > 0) {
            menu->currentIndex -= 1;
            SetPosition(menu, menu->currentIndex);
         }
      }

      /* If near the bottom, shift up. */
      if(y + menu->y + menu->itemHeight / 2
            >= menu->screen->y + menu->screen->height) {
         if(menu->currentIndex + 1 < menu->itemCount) {
            menu->currentIndex += 1;
            SetPosition(menu, menu->currentIndex);
         }
      }

   }

   if(menu->lastIndex != menu->currentIndex) {
      UpdateMenu(menu);
      menu->lastIndex = menu->currentIndex;
   }

   /* If the selected item is a submenu, show it. */
   ip = GetMenuItem(menu, menu->currentIndex);
   if(ip && IsMenuValid(ip->submenu)) {
      if(ShowSubmenu(ip->submenu, menu, runner,
                     menu->x + menu->width,
                     menu->y + menu->offsets[menu->currentIndex])) {

         /* Item selected; destroy the menu tree. */
         return MENU_SUBSELECT;

      } else {

         /* No selection made. */
         UpdateMenu(menu);

      }
   }

   return MENU_NOSELECTION;

}

/** Update the menu selection. */
void UpdateMenu(Menu *menu)
{

   MenuItem *ip;

   /* Clear the old selection. */
   ip = GetMenuItem(menu, menu->lastIndex);
   DrawMenuItem(menu, ip, menu->lastIndex);

   /* Highlight the new selection. */
   ip = GetMenuItem(menu, menu->currentIndex);
   if(ip != NULL) {
      DrawMenuItem(menu, ip, menu->currentIndex);
   }

   JXCopyArea(display, menu->pixmap, menu->window, rootGC,
              0, 0, menu->width, menu->height, 0, 0);

}

/** Draw a menu item. */
void DrawMenuItem(Menu *menu, MenuItem *item, int index)
{

   ButtonNode button;

   Assert(menu);

   if(!item) {
      if(index == -1 && menu->label) {
         ResetButton(&button, menu->pixmap, &rootVisual);
         button.x = MENU_BORDER_SIZE;
         button.y = MENU_BORDER_SIZE;
         button.width = menu->width - MENU_BORDER_SIZE * 2;
         button.height = menu->itemHeight - 1;
         button.font = FONT_MENU;
         button.type = BUTTON_LABEL;
         button.text = menu->label;
         button.alignment = ALIGN_CENTER;
         DrawButton(&button);
      }
      return;
   }

   if(item->type != MENU_ITEM_SEPARATOR) {
      ColorType fg;

      ResetButton(&button, menu->pixmap, &rootVisual);
      if(menu->currentIndex == index) {
         button.type = BUTTON_MENU_ACTIVE;
         fg = COLOR_MENU_ACTIVE_FG;
      } else {
         button.type = BUTTON_LABEL;
         fg = COLOR_MENU_FG;
      }

      button.x = MENU_BORDER_SIZE;
      button.y = menu->offsets[index];
      button.font = FONT_MENU;
      button.width = menu->width - MENU_BORDER_SIZE * 2;
      button.height = menu->itemHeight;
      button.text = item->name;
      button.icon = item->icon;
      DrawButton(&button);

      if(item->submenu) {

         const int asize = (menu->itemHeight + 7) / 8;
         const int y = menu->offsets[index] + (menu->itemHeight + 1) / 2;
         int x = menu->width - 2 * asize - 1;
         int i;

         JXSetForeground(display, rootGC, colors[fg]);
         for(i = 0; i < asize; i++) {
            const int y1 = y - asize + i;
            const int y2 = y + asize - i;
            JXDrawLine(display, menu->pixmap, rootGC, x, y1, x, y2);
            x += 1;
         }
         JXDrawPoint(display, menu->pixmap, rootGC, x, y);

      }

   } else {
      JXSetForeground(display, rootGC, colors[COLOR_MENU_DOWN]);
      JXDrawLine(display, menu->pixmap, rootGC, 4,
                 menu->offsets[index] + 2, menu->width - 6,
                 menu->offsets[index] + 2);
      JXSetForeground(display, rootGC, colors[COLOR_MENU_UP]);
      JXDrawLine(display, menu->pixmap, rootGC, 4,
                 menu->offsets[index] + 3, menu->width - 6,
                 menu->offsets[index] + 3);
   }

}

/** Get the next item in the menu. */
int GetNextMenuIndex(Menu *menu)
{

   MenuItem *item;
   int x;

   for(x = menu->currentIndex + 1; x < menu->itemCount; x++) {
      item = GetMenuItem(menu, x);
      if(item->type != MENU_ITEM_SEPARATOR) {
         return x;
      }
   }

   return 0;

}

/** Get the previous item in the menu. */
int GetPreviousMenuIndex(Menu *menu)
{

   MenuItem *item;
   int x;

   for(x = menu->currentIndex - 1; x >= 0; x--) {
      item = GetMenuItem(menu, x);
      if(item->type != MENU_ITEM_SEPARATOR) {
         return x;
      }
   }

   return menu->itemCount - 1;

}

/** Get the item in the menu given a y-coordinate. */
int GetMenuIndex(Menu *menu, int y)
{

   int x;

   if(y < menu->offsets[0]) {
      return -1;
   }
   for(x = 0; x < menu->itemCount - 1; x++) {
      if(y >= menu->offsets[x] && y < menu->offsets[x + 1]) {
         return x;
      }
   }
   return x;

}

/** Get the menu item associated with an index. */
MenuItem *GetMenuItem(Menu *menu, int index)
{

   MenuItem *ip;

   if(index >= 0) {
      for(ip = menu->items; ip; ip = ip->next) {
         if(!index) {
            return ip;
         }
         --index;
      }
   } else {
      ip = NULL;
   }

   return ip;

}

/** Set the active menu item. */
void SetPosition(Menu *tp, int index)
{
   int y = tp->offsets[index] + tp->itemHeight / 2;
   if(tp->height > tp->screen->height) {

      int updated = 0;
      while(y + tp->y < tp->itemHeight / 2) {
         tp->y += tp->itemHeight;
         updated = tp->itemHeight;
      }
      while(y + tp->y >= tp->screen->y + tp->screen->height) {
         tp->y -= tp->itemHeight;
         updated = -tp->itemHeight;
      }
      if(updated) {
         JXMoveWindow(display, tp->window, tp->x, tp->y);
         y += updated;
      }

   }

   /* We need to do this twice so the event gets registered
    * on the submenu if one exists. */
   MoveMouse(tp->window, 6, y);
   MoveMouse(tp->window, 6, y);

}

/** Determine if a menu is valid (and can be shown). */
char IsMenuValid(const Menu *menu)
{
   if(menu) {
      MenuItem *ip;
      for(ip = menu->items; ip; ip = ip->next) {
         if(ip->type != MENU_ITEM_SEPARATOR) {
            return 1;
         }
      }
   }
   return 0;
}

