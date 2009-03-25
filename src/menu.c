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

#define BASE_ICON_OFFSET 3

typedef enum {
   MENU_NOSELECTION = 0,
   MENU_LEAVE       = 1,
   MENU_SUBSELECT   = 2
} MenuSelectionType;

/** Submenu arrow, 4 x 7 pixels */
static char menu_bitmap[] = {
   0x01, 0x03, 0x07, 0x0F, 0x07, 0x03, 0x01
};

static int ShowSubmenu(Menu *menu, Menu *parent, int x, int y);

static void CreateMenu(Menu *menu, int x, int y);
static void HideMenu(Menu *menu);
static void DrawMenu(Menu *menu);
static void RedrawMenuTree(Menu *menu);

static int MenuLoop(Menu *menu);
static MenuSelectionType UpdateMotion(Menu *menu, XEvent *event);

static void UpdateMenu(Menu *menu);
static void DrawMenuItem(Menu *menu, MenuItem *item, int index);
static MenuItem *GetMenuItem(Menu *menu, int index);
static int GetNextMenuIndex(Menu *menu);
static int GetPreviousMenuIndex(Menu *menu);
static int GetMenuIndex(Menu *menu, int index);
static void SetPosition(Menu *tp, int index);
static int IsMenuValid(const Menu *menu);

static MenuAction *menuAction = NULL;
static unsigned int menuOpacity = UINT_MAX;

int menuShown = 0;

/** Initialize a menu. */
void InitializeMenu(Menu *menu) {

   MenuItem *np;
   int index, temp;
   int hasSubmenu;
   int userHeight;

   menu->textOffset = 0;
   menu->itemCount = 0;

   /* Compute the max size needed */
   userHeight = menu->itemHeight;
   if(userHeight < 0) {
      userHeight = 0;
   }
   menu->itemHeight = GetStringHeight(FONT_MENU);
   for(np = menu->items; np; np = np->next) {
      if(np->iconName) {
         np->icon = LoadNamedIcon(np->iconName);
         if(np->icon) {
            if(userHeight == 0) {
               if(menu->itemHeight < (int)np->icon->image->height) {
                  menu->itemHeight = np->icon->image->height;
               }
               if(menu->textOffset < (int)np->icon->image->width + 4) {
                  menu->textOffset = np->icon->image->width + 4;
               }
            }
         }
      } else {
         np->icon = NULL;
      }
      ++menu->itemCount;
   }
   menu->itemHeight += BASE_ICON_OFFSET * 2;

   if(userHeight) {
      menu->itemHeight = userHeight + BASE_ICON_OFFSET * 2;
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

   menu->height = 1;
   if(menu->label) {
      menu->height += menu->itemHeight;
   }

   /* Nothing else to do if there is nothing in the menu. */
   if(menu->itemCount == 0) {
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
         temp = GetStringWidth(FONT_MENU, np->name) + 2;
         if(temp > menu->width) {
            menu->width = temp;
         }
      }
      if(np->submenu) {
         hasSubmenu = 7;
         InitializeMenu(np->submenu);
      }
   }
   menu->height += 2;
   menu->width += 15 + hasSubmenu + menu->textOffset;

}

/** Show a menu. */
void ShowMenu(Menu *menu, RunMenuCommandType runner, int x, int y) {

   int mouseStatus, keyboardStatus;

   /* Don't show the menu if there isn't anything to show. */
   if(!IsMenuValid(menu)) {
      return;
   }

   mouseStatus = GrabMouse(rootWindow);
   keyboardStatus = JXGrabKeyboard(display, rootWindow, False,
      GrabModeAsync, GrabModeAsync, CurrentTime);
   if(!mouseStatus || keyboardStatus != GrabSuccess) {
      return;
   }

   ShowSubmenu(menu, NULL, x, y);

   JXUngrabKeyboard(display, CurrentTime);
   JXUngrabPointer(display, CurrentTime);
   RefocusClient();

   if(menuAction) {
      (runner)(menuAction);
      menuAction = NULL;
   }

}

/** Destroy a menu. */
void DestroyMenu(Menu *menu) {
   MenuItem *np;

   if(menu) {
      while(menu->items) {
         np = menu->items->next;
         if(menu->items->name) {
            Release(menu->items->name);
         }
         switch(menu->items->action.type) {
         case MA_EXECUTE:
         case MA_EXIT:
            if(menu->items->action.data.str) {
               Release(menu->items->action.data.str);
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
      menu = NULL;
   }
}

/** Show a submenu. */
int ShowSubmenu(Menu *menu, Menu *parent, int x, int y) {

   int status;

   menu->parent = parent;
   CreateMenu(menu, x, y);

   ++menuShown;
   status = MenuLoop(menu);
   --menuShown;

   HideMenu(menu);

   return status;

}

/** Menu process loop.
 * Returns 0 if no selection was made or 1 if a selection was made.
 */
int MenuLoop(Menu *menu) {

   XEvent event;
   MenuItem *ip;
   int count;
   int hadMotion;
   int pressx, pressy;

   hadMotion = 0;

   GetMousePosition(&pressx, &pressy);

   for(;;) {

      WaitForEvent(&event);

      switch(event.type) {
      case Expose:
         RedrawMenuTree(menu);
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
         if(abs(event.xbutton.x_root - pressx) < doubleClickDelta) {
            if(abs(event.xbutton.y_root - pressy) < doubleClickDelta) {
               break;
            }
         }
            
         if(menu->currentIndex >= 0) {
            count = 0;
            for(ip = menu->items; ip; ip = ip->next) {
               if(count == menu->currentIndex) {
                  menuAction = &ip->action;
                  break;
               }
               ++count;
            }
         }
         return 1;
      default:
         break;
      }

   }
}

/** Create and map a menu. */
void CreateMenu(Menu *menu, int x, int y) {

   XSetWindowAttributes attr;
   unsigned long attrMask;
   int temp;

   menu->lastIndex = -1;
   menu->currentIndex = -1;

   if(x + menu->width > rootWidth) {
      if(menu->parent) {
         x = menu->parent->x - menu->width;
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
      menu->width, menu->height, 0, CopyFromParent, InputOutput,
      CopyFromParent, attrMask, &attr);

   if(menuOpacity < UINT_MAX) {
      SetCardinalAtom(menu->window, ATOM_NET_WM_WINDOW_OPACITY, menuOpacity);
      JXSync(display, False);
   }

   JXMapRaised(display, menu->window); 

}

/** Hide a menu. */
void HideMenu(Menu *menu) {

   JXDestroyWindow(display, menu->window);

}

/** Redraw a menu and its submenus. */
void RedrawMenuTree(Menu *menu) {

   if(menu->parent) {
      RedrawMenuTree(menu->parent);
   }

   DrawMenu(menu);
   UpdateMenu(menu);

}

/** Draw a menu. */
void DrawMenu(Menu *menu) {

   MenuItem *np;
   int x;

   if(menu->label) {
      DrawMenuItem(menu, NULL, -1);
   }

   x = 0;
   for(np = menu->items; np; np = np->next) {
      DrawMenuItem(menu, np, x);
      ++x;
   }

   JXSetForeground(display, rootGC, colors[COLOR_MENU_UP]);
   JXDrawLine(display, menu->window, rootGC,
      0, 0, menu->width - 1, 0);
   JXDrawLine(display, menu->window, rootGC,
      0, 1, menu->width - 2, 1);
   JXDrawLine(display, menu->window, rootGC,
      0, 2, 0, menu->height - 1);
   JXDrawLine(display, menu->window, rootGC,
      1, 2, 1, menu->height - 2);

   JXSetForeground(display, rootGC, colors[COLOR_MENU_DOWN]);
   JXDrawLine(display, menu->window, rootGC,
      1, menu->height - 1, menu->width - 1, menu->height - 1);
   JXDrawLine(display, menu->window, rootGC,
      2, menu->height - 2, menu->width - 1, menu->height - 2);
   JXDrawLine(display, menu->window, rootGC,
      menu->width - 1, 1, menu->width - 1, menu->height - 3);
   JXDrawLine(display, menu->window, rootGC,
      menu->width - 2, 2, menu->width - 2, menu->height - 3);

}

/** Determine the action to take given an event. */
MenuSelectionType UpdateMotion(Menu *menu, XEvent *event) {

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
         if(tp->currentIndex >= 0) {
            x = 0;
            for(ip = tp->items; ip; ip = ip->next) {
               if(x == tp->currentIndex) {
                  menuAction = &ip->action;
                  break;
               }
               ++x;
            }
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
      if(ShowSubmenu(ip->submenu, menu, menu->x + menu->width,
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
void UpdateMenu(Menu *menu) {

   ButtonNode button;
   Pixmap pixmap;
   MenuItem *ip;

   /* Clear the old selection. */
   ip = GetMenuItem(menu, menu->lastIndex);
   DrawMenuItem(menu, ip, menu->lastIndex);

   /* Highlight the new selection. */
   ip = GetMenuItem(menu, menu->currentIndex);
   if(ip) {

      if(ip->type == MENU_ITEM_SEPARATOR) {
         return;
      }

      ResetButton(&button, menu->window, rootGC);
      button.type = BUTTON_MENU_ACTIVE;
      button.font = FONT_MENU;
      button.width = menu->width - 5;
      button.height = menu->itemHeight - 2;
      button.icon = ip->icon;
      button.text = ip->name;
      button.x = 2;
      button.y = menu->offsets[menu->currentIndex] + 1;
      DrawButton(&button);

      if(ip->submenu) {
         pixmap = JXCreateBitmapFromData(display, menu->window,
            menu_bitmap, 4, 7);
         JXSetForeground(display, rootGC, colors[COLOR_MENU_ACTIVE_FG]);
         JXSetClipMask(display, rootGC, pixmap);
         JXSetClipOrigin(display, rootGC,
            menu->width - 9,
            menu->offsets[menu->currentIndex] + menu->itemHeight / 2 - 4);
         JXFillRectangle(display, menu->window, rootGC,
            menu->width - 9,
            menu->offsets[menu->currentIndex] + menu->itemHeight / 2 - 4,
            4, 7);
         JXSetClipMask(display, rootGC, None);
         JXFreePixmap(display, pixmap);
      }
   }

}

/** Draw a menu item. */
void DrawMenuItem(Menu *menu, MenuItem *item, int index) {

   ButtonNode button;
   Pixmap pixmap;

   Assert(menu);

   if(!item) {
      if(index == -1 && menu->label) {
         ResetButton(&button, menu->window, rootGC);
         button.x = 2;
         button.y = 2;
         button.width = menu->width - 5;
         button.height = menu->itemHeight - 2;
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
      button.x = 2;
      button.y = 1 + menu->offsets[index];
      button.font = FONT_MENU;
      button.type = BUTTON_LABEL;
      button.width = menu->width - 5;
      button.height = menu->itemHeight - 2;
      button.text = item->name;
      button.icon = item->icon;
      DrawButton(&button);

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

   if(item->submenu) {

      pixmap = JXCreatePixmapFromBitmapData(display, menu->window,
         menu_bitmap, 4, 7, colors[COLOR_MENU_FG],
         colors[COLOR_MENU_BG], rootDepth);
      JXCopyArea(display, pixmap, menu->window, rootGC, 0, 0, 4, 7,
         menu->width - 9, menu->offsets[index] + menu->itemHeight / 2 - 4);
      JXFreePixmap(display, pixmap);

   }

}

/** Get the next item in the menu. */
int GetNextMenuIndex(Menu *menu) {

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
int GetPreviousMenuIndex(Menu *menu) {

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
int GetMenuIndex(Menu *menu, int y) {

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
MenuItem *GetMenuItem(Menu *menu, int index) {

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
void SetPosition(Menu *tp, int index) {

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
int IsMenuValid(const Menu *menu) {

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

/** Set the Menu transparency level. */
void SetMenuOpacity(const char *str) {

   double temp;

   Assert(str);

   temp = atof(str);
   if(temp <= 0.0 || temp > 1.0) {
      Warning("invalid menu opacity: %s", str);
      temp = 1.0;
   }
   menuOpacity = (unsigned int)(temp * UINT_MAX);

}

