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
#include "color.h"
#include "icon.h"
#include "image.h"
#include "main.h"
#include "cursor.h"
#include "key.h"
#include "button.h"
#include "event.h"
#include "error.h"
#include "root.h"
#include "settings.h"

#define MENU_BORDER_SIZE   1
#define BASE_ICON_OFFSET   3

typedef unsigned char MenuSelectionType;
#define MENU_NOSELECTION   0
#define MENU_LEAVE         1
#define MENU_SUBSELECT     2

static char ShowSubmenu(Menu *menu, Menu *parent, int x, int y);

static void CreateMenu(Menu *menu, int x, int y);
static void HideMenu(Menu *menu);
static void DrawMenu(Menu *menu);

static char MenuLoop(Menu *menu);
static MenuSelectionType UpdateMotion(Menu *menu, XEvent *event);

static void UpdateMenu(Menu *menu);
static void DrawMenuItem(Menu *menu, MenuItem *item, int index);
static MenuItem *GetMenuItem(Menu *menu, int index);
static int GetNextMenuIndex(Menu *menu);
static int GetPreviousMenuIndex(Menu *menu);
static int GetMenuIndex(Menu *menu, int index);
static void SetPosition(Menu *tp, int index);
static char IsMenuValid(const Menu *menu);

static ActionNode *menuAction = NULL;

int menuShown = 0;

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
         np->icon = LoadNamedIcon(np->iconName, 1);
         if(np->icon) {
            hasIcon = 1;
         }
      } else {
         np->icon = NULL;
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
         menu->height += 5;
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
   menu->width += 2 * MENU_BORDER_SIZE;
   menu->width += 7;
   menu->height += MENU_BORDER_SIZE;

}

/** Show a menu. */
void ShowMenu(const ActionContext *context, Menu *menu)
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

   ShowSubmenu(menu, NULL,
               context->x - MENU_BORDER_SIZE,
               context->y - MENU_BORDER_SIZE);

   JXUngrabKeyboard(display, CurrentTime);
   JXUngrabPointer(display, CurrentTime);
   RefocusClient();

   if(menuAction) {
      RunAction(context, menuAction);
      menuAction = NULL;
   }

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
         if(menu->items->action.arg) {
            Release(menu->items->action.arg);
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
      menu = NULL;
   }
}

/** Show a submenu. */
char ShowSubmenu(Menu *menu, Menu *parent, int x, int y)
{

   char status;

   menu->parent = parent;
   CreateMenu(menu, x, y);

   menuShown += 1;
   status = MenuLoop(menu);
   menuShown -= 1;

   HideMenu(menu);

   return status;

}

/** Menu process loop.
 * Returns 0 if no selection was made or 1 if a selection was made.
 */
char MenuLoop(Menu *menu)
{

   XEvent event;
   MenuItem *ip;
   int pressx, pressy;
   char hadMotion;

   hadMotion = 0;

   GetMousePosition(&pressx, &pressy);

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
         switch(UpdateMotion(menu, &event)) {
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
            
         ip = GetMenuItem(menu, menu->currentIndex);
         if(ip != NULL) {
            menuAction = &ip->action;
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

   if(x + menu->width > rootWidth) {
      if(menu->parent) {
         x = menu->parent->x - menu->width + MENU_BORDER_SIZE;
      } else {
         x = rootWidth - menu->width;
      }
   }
   temp = y;
   if(y + menu->height > rootHeight) {
      y = rootHeight - menu->height;
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
}

/** Draw a menu. */
void DrawMenu(Menu *menu)
{

   MenuItem *np;
   int x;

   JXSetForeground(display, rootGC, colors[COLOR_MENU_DOWN]);
   JXDrawRectangle(display, menu->window, rootGC, 0, 0,
                   menu->width - 1, menu->height - 1);

   if(menu->label) {
      DrawMenuItem(menu, NULL, -1);
   }

   x = 0;
   for(np = menu->items; np; np = np->next) {
      DrawMenuItem(menu, np, x);
      ++x;
   }

}

/** Determine the action to take given an event. */
MenuSelectionType UpdateMotion(Menu *menu, XEvent *event)
{

   MenuItem *ip;
   Menu *tp;
   Window subwindow;
   int x, y;

   if(event->type == MotionNotify) {

      SetMousePosition(event->xmotion.x_root, event->xmotion.y_root);
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
      switch(GetKey(&event->xkey)) {
      case ACTION_UP:
         y = GetPreviousMenuIndex(tp);
         break;
      case ACTION_DOWN:
         y = GetNextMenuIndex(tp);
         break;
      case ACTION_RIGHT:
         tp = menu;
         y = 0;
         break;
      case ACTION_LEFT:
         if(tp->parent) {
            tp = tp->parent;
            if(tp->currentIndex >= 0) {
               y = tp->currentIndex;
            } else {
               y = 0;
            }
         }
         break;
      case ACTION_ESC:
         return MENU_SUBSELECT;
      case ACTION_ENTER:
         ip = GetMenuItem(menu, tp->currentIndex);
         if(ip != NULL) {
            menuAction = &ip->action;
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
   if(menu->height > rootHeight && menu->currentIndex >= 0) {

      /* If near the top, shift down. */
      if(y + menu->y <= 0) {
         if(menu->currentIndex > 0) {
            --menu->currentIndex;
            SetPosition(menu, menu->currentIndex);
         }
      }

      /* If near the bottom, shift up. */
      if(y + menu->y + menu->itemHeight / 2 >= rootHeight) {
         if(menu->currentIndex + 1 < menu->itemCount) {
            ++menu->currentIndex;
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
      if(ShowSubmenu(ip->submenu, menu,
                     menu->x + menu->width - MENU_BORDER_SIZE,
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

}

/** Draw a menu item. */
void DrawMenuItem(Menu *menu, MenuItem *item, int index)
{

   ButtonNode button;
   ColorType fg;

   Assert(menu);

   if(!item) {
      if(index == -1 && menu->label) {
         ResetButton(&button, menu->window, rootGC);
         button.x = MENU_BORDER_SIZE;
         button.y = 0;
         button.width = menu->width - 2 * MENU_BORDER_SIZE - 1;
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

      ResetButton(&button, menu->window, rootGC);
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
      button.width = menu->width - 2 * MENU_BORDER_SIZE - 1;
      button.height = menu->itemHeight - 1;
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
            JXDrawLine(display, menu->window, rootGC, x, y1, x, y2);
            x += 1;
         }
         JXDrawPoint(display, menu->window, rootGC, x, y);

      }

   } else {

      JXSetForeground(display, rootGC, colors[COLOR_MENU_DOWN]);
      JXDrawLine(display, menu->window, rootGC, 4,
                 menu->offsets[index] + 2, menu->width - 6,
                 menu->offsets[index] + 2);
      JXSetForeground(display, rootGC, colors[COLOR_MENU_UP]);
      JXDrawLine(display, menu->window, rootGC, 4,
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

   int y;
   int updated;

   y = tp->offsets[index] + tp->itemHeight / 2;

   if(tp->height > rootHeight) {

      updated = 0;
      while(y + tp->y < tp->itemHeight / 2) {
         tp->y += tp->itemHeight;
         updated = tp->itemHeight;
      }
      while(y + tp->y >= rootHeight) {
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
   MenuItem *ip;
   if(menu) {
      for(ip = menu->items; ip; ip = ip->next) {
         if(ip->type != MENU_ITEM_SEPARATOR) {
            return 1;
         }
      }
   }
   return 0;
}

