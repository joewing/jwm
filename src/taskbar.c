/**
 * @file taskbar.c
 * @author Joe Wingbermuehle
 * @date 2005-2006
 *
 * @brief Task list tray component.
 *
 */

#include "jwm.h"
#include "taskbar.h"
#include "tray.h"
#include "timing.h"
#include "main.h"
#include "client.h"
#include "clientlist.h"
#include "color.h"
#include "popup.h"
#include "button.h"
#include "cursor.h"
#include "icon.h"
#include "error.h"
#include "winmenu.h"
#include "screen.h"
#include "settings.h"
#include "event.h"
#include "misc.h"

typedef struct TaskBarType {

   TrayComponentType *cp;

   int itemHeight;
   int itemWidth;
   LayoutType layout;

   Pixmap buffer;

   TimeType mouseTime;
   int mousex, mousey;

   struct TaskBarType *next;

} TaskBarType;

typedef struct ClientEntry {
   ClientNode *client;
   struct ClientEntry *next;
   struct ClientEntry *prev;
} ClientEntry;

typedef struct TaskEntry {
   ClientEntry *clients;
   struct TaskEntry *next;
   struct TaskEntry *prev;
} TaskEntry;

static TaskEntry *selectedEntry;
static TaskBarType *bars;
static TaskEntry *taskEntries;
static TaskEntry *taskEntriesTail;

static char ShouldShowEntry(const TaskEntry *tp);
static TaskEntry *GetEntry(TaskBarType *bar, int x, int y);
static void Render(const TaskBarType *bp);
static void ShowClientList(TaskBarType *bar, TaskEntry *tp);
static void RunTaskBarCommand(const MenuAction *action);

static void SetSize(TrayComponentType *cp, int width, int height);
static void Create(TrayComponentType *cp);
static void Resize(TrayComponentType *cp);
static void ProcessTaskButtonEvent(TrayComponentType *cp,
                                   int x, int y, int mask);
static void MinimizeGroup(const TaskEntry *tp);
static void FocusGroup(const TaskEntry *tp);
static void ProcessTaskMotionEvent(TrayComponentType *cp,
                                   int x, int y, int mask);
static void SignalTaskbar(const TimeType *now, int x, int y, Window w,
                          void *data);

/** Initialize task bar data. */
void InitializeTaskBar(void)
{
   bars = NULL;
   taskEntries = NULL;
   taskEntriesTail = NULL;
}

/** Shutdown the task bar. */
void ShutdownTaskBar(void)
{
   TaskBarType *bp;
   for(bp = bars; bp; bp = bp->next) {
      JXFreePixmap(display, bp->buffer);
   }
}

/** Destroy task bar data. */
void DestroyTaskBar(void)
{
   TaskBarType *bp;
   while(bars) {
      bp = bars->next;
      UnregisterCallback(SignalTaskbar, bars);
      Release(bars);
      bars = bp;
   }
}

/** Create a new task bar tray component. */
TrayComponentType *CreateTaskBar()
{

   TrayComponentType *cp;
   TaskBarType *tp;

   tp = Allocate(sizeof(TaskBarType));
   tp->next = bars;
   bars = tp;
   tp->itemHeight = 0;
   tp->layout = LAYOUT_HORIZONTAL;
   tp->mousex = -settings.doubleClickDelta;
   tp->mousey = -settings.doubleClickDelta;
   tp->mouseTime.seconds = 0;
   tp->mouseTime.ms = 0;

   cp = CreateTrayComponent();
   cp->object = tp;
   tp->cp = cp;

   cp->SetSize = SetSize;
   cp->Create = Create;
   cp->Resize = Resize;
   cp->ProcessButtonPress = ProcessTaskButtonEvent;
   cp->ProcessMotionEvent = ProcessTaskMotionEvent;

   RegisterCallback(settings.popupDelay / 2, SignalTaskbar, tp);

   return cp;

}

/** Set the size of a task bar tray component. */
void SetSize(TrayComponentType *cp, int width, int height)
{

   TaskBarType *tp;

   Assert(cp);

   tp = (TaskBarType*)cp->object;

   Assert(tp);

   if(width == 0) {
      tp->layout = LAYOUT_HORIZONTAL;
   } else if(height == 0) {
      tp->layout = LAYOUT_VERTICAL;
   } else if(width > height) {
      tp->layout = LAYOUT_HORIZONTAL;
   } else {
      tp->layout = LAYOUT_VERTICAL;
   }

}

/** Initialize a task bar tray component. */
void Create(TrayComponentType *cp)
{

   TaskBarType *tp = (TaskBarType*)cp->object;

   if(tp->layout == LAYOUT_HORIZONTAL) {
      tp->itemHeight = cp->height;
      tp->itemWidth = cp->height;
   } else {
      tp->itemHeight = cp->width;
      tp->itemWidth = cp->width;
   }

   cp->pixmap = JXCreatePixmap(display, rootWindow, cp->width, cp->height,
                               rootVisual.depth);
   tp->buffer = cp->pixmap;

   ClearTrayDrawable(cp);

}

/** Resize a task bar tray component. */
void Resize(TrayComponentType *cp)
{
   TaskBarType *tp = (TaskBarType*)cp->object;

   if(tp->buffer != None) {
      JXFreePixmap(display, tp->buffer);
   }

   if(tp->layout == LAYOUT_HORIZONTAL) {
      tp->itemHeight = cp->height;
      tp->itemWidth = cp->height;
   } else {
      tp->itemHeight = cp->width;
      tp->itemWidth = cp->width;
   }

   cp->pixmap = JXCreatePixmap(display, rootWindow, cp->width, cp->height,
                               rootVisual.depth);
   tp->buffer = cp->pixmap;

   ClearTrayDrawable(cp);
}

/** Process a task list button event. */
void ProcessTaskButtonEvent(TrayComponentType *cp, int x, int y, int mask)
{

   TaskBarType *bar = (TaskBarType*)cp->object;
   TaskEntry *entry = GetEntry(bar, x, y);

   if(entry) {
      ClientEntry *cp;
      char hasActive = 0;

      switch(mask) {
      case Button1:  /* Raise or minimize items in this group. */
         for(cp = entry->clients; cp; cp = cp->next)  {
            if(!ShouldFocus(cp->client)) {
               continue;
            }
            if(cp->client->state.status & STAT_ACTIVE &&
               !(cp->client->state.status & STAT_MINIMIZED)) {
               hasActive = 1;
               break;
            }
         }
         if(hasActive) {
            MinimizeGroup(entry);
         } else {
            FocusGroup(entry);
         }
         break;
      case Button3:
         ShowClientList(bar, entry);
         break;
      default:
         break;
      }
   }

}

/** Minimize all clients in a group. */
void MinimizeGroup(const TaskEntry *tp)
{
   ClientEntry *cp;
   for(cp = tp->clients; cp; cp = cp->next) {
      if(ShouldFocus(cp->client)) {
         MinimizeClient(cp->client, 0);
      }
   }
}

/** Raise all clients in a group and focus the top-most. */
void FocusGroup(const TaskEntry *tp)
{
   const char *className = tp->clients->client->className;
   ClientNode **toRestore;
   unsigned restoreCount;
   int i;

   /* If there is no class name, then there will only be one client. */
   if(!className) {
      RestoreClient(tp->clients->client, 1);
      FocusClient(tp->clients->client);
      return;
   }

   /* Build up the list of clients to restore in correct order. */
   toRestore = AllocateStack(sizeof(ClientNode*) * clientCount);
   restoreCount = 0;
   for(i = 0; i < LAYER_COUNT; i++) {
      ClientNode *np;
      for(np = nodes[i]; np; np = np->next) {
         if(!ShouldFocus(np)) {
            continue;
         }
         if(np->className && !strcmp(np->className, className)) {
            toRestore[restoreCount] = np;
            restoreCount += 1;
         }
      }
   }
   Assert(restoreCount <= clientCount);
   for(i = restoreCount - 1; i >= 0; i--) {
      RestoreClient(toRestore[i], 1);
   }
   FocusClient(toRestore[0]);
   ReleaseStack(toRestore);
}

/** Process a task list motion event. */
void ProcessTaskMotionEvent(TrayComponentType *cp, int x, int y, int mask)
{
   TaskBarType *bp = (TaskBarType*)cp->object;
   bp->mousex = cp->screenx + x;
   bp->mousey = cp->screeny + y;
   GetCurrentTime(&bp->mouseTime);
}

/** Show the menu associated with a task list item. */
void ShowClientList(TaskBarType *bar, TaskEntry *tp)
{
   Menu *menu;
   MenuItem *item;
   ClientEntry *cp;

   const ScreenType *sp;
   int x, y;
   Window w;

   selectedEntry = tp;

   menu = Allocate(sizeof(Menu));
   menu->itemHeight = 0;
   menu->items = NULL;
   menu->label = NULL;

   item = CreateMenuItem(MENU_ITEM_SUBMENU);
   item->name = CopyString(_("Close"));
   item->action.type = MA_CLOSE;
   item->next = menu->items;
   menu->items = item;

   item = CreateMenuItem(MENU_ITEM_SUBMENU);
   item->name = CopyString(_("Minimize"));
   item->action.type = MA_MINIMIZE;
   item->next = menu->items;
   menu->items = item;

   item = CreateMenuItem(MENU_ITEM_SUBMENU);
   item->name = CopyString(_("Restore"));
   item->action.type = MA_RESTORE;
   item->next = menu->items;
   menu->items = item;

   item = CreateMenuItem(MENU_ITEM_SUBMENU);
   item->name = CopyString(_("Send To"));
   item->action.type = MA_SENDTO_MENU;
   item->next = menu->items;
   menu->items = item;

   /* Load the separator and group actions. */
   item = CreateMenuItem(MENU_ITEM_SEPARATOR);
   item->next = menu->items;
   menu->items = item;

   /* Load the clients into the menu. */
   for(cp = tp->clients; cp; cp = cp->next) {
      if(!ShouldFocus(cp->client)) {
         continue;
      }
      item = CreateMenuItem(MENU_ITEM_NORMAL);
      if(cp->client->state.status & STAT_MINIMIZED) {
         size_t len = 0;
         if(cp->client->name) {
            len = strlen(cp->client->name);
         }
         item->name = Allocate(len + 3);
         item->name[0] = '[';
         memcpy(&item->name[1], cp->client->name, len);
         item->name[len + 1] = ']';
         item->name[len + 2] = 0;
      } else {
         item->name = CopyString(cp->client->name);
      }
      item->icon = cp->client->icon;
      item->action.data.client = cp->client;
      item->next = menu->items;
      menu->items = item;
   }

   /* Initialize and position the menu. */
   InitializeMenu(menu);
   sp = GetCurrentScreen(bar->cp->screenx, bar->cp->screeny);
   GetMousePosition(&x, &y, &w);
   if(bar->layout == LAYOUT_HORIZONTAL) {
      if(bar->cp->screeny + bar->cp->height / 2 < sp->y + sp->height / 2) {
         y = bar->cp->screeny + bar->cp->height;
      } else {
         y = bar->cp->screeny - menu->height;
      }
      x -= menu->width / 2;
      x = Max(x, 0);
   } else {
      if(bar->cp->screenx + bar->cp->width / 2 < sp->x + sp->width / 2) {
         x = bar->cp->screenx + bar->cp->width;
      } else {
         x = bar->cp->screenx - menu->width;
      }
      y -= menu->height / 2;
      y = Max(y, 0);
   }

   ShowMenu(menu, RunTaskBarCommand, x, y);

   DestroyMenu(menu);

}

/** Run a menu action. */
void RunTaskBarCommand(const MenuAction *action)
{
   ClientEntry *cp;

   /* Check if the program entry was clicked. */
   if(action->type == MA_NONE) {
      RestoreClient(action->data.client, 1);
      FocusClient(action->data.client);
      return;
   }

   /* Apply the specified action to all clients in the group. */
   for(cp = selectedEntry->clients; cp; cp = cp->next) {
      if(!ShouldFocus(cp->client)) {
         continue;
      }
      switch(action->type) {
      case MA_SENDTO:
         SetClientDesktop(cp->client, action->data.i);
         break;
      case MA_CLOSE:
         DeleteClient(cp->client);
         break;
      case MA_RESTORE:
         RestoreClient(cp->client, 0);
         break;
      case MA_MINIMIZE:
         MinimizeClient(cp->client, 0);
         break;
      default:
         break;
      }
   }
}

/** Add a client to the task bar. */
void AddClientToTaskBar(ClientNode *np)
{
   TaskEntry *tp = NULL;
   ClientEntry *cp = Allocate(sizeof(ClientEntry));
   cp->client = np;

   if(np->className) {
      for(tp = taskEntries; tp; tp = tp->next) {
         const char *className = tp->clients->client->className;
         if(className && !strcmp(np->className, className)) {
            break;
         }
      }
   }
   if(tp == NULL) {
      tp = Allocate(sizeof(TaskEntry));
      tp->clients = NULL;
      tp->next = NULL;
      tp->prev = taskEntriesTail;
      if(taskEntriesTail) {
         taskEntriesTail->next = tp;
      } else {
         taskEntries = tp;
      }
      taskEntriesTail = tp;
   }

   cp->next = tp->clients;
   if(tp->clients) {
      tp->clients->prev = cp;
   }
   cp->prev = NULL;
   tp->clients = cp;

   UpdateTaskBar();
   UpdateNetClientList();

}

/** Remove a client from the task bar. */
void RemoveClientFromTaskBar(ClientNode *np)
{
   TaskEntry *tp;
   for(tp = taskEntries; tp; tp = tp->next) {
      ClientEntry *cp;
      for(cp = tp->clients; cp; cp = cp->next) {
         if(cp->client == np) {
            if(cp->prev) {
               cp->prev->next = cp->next;
            } else {
               tp->clients = cp->next;
            }
            if(cp->next) {
               cp->next->prev = cp->prev;
            }
            Release(cp);
            if(!tp->clients) {
               if(tp->prev) {
                  tp->prev->next = tp->next;
               } else {
                  taskEntries = tp->next;
               }
               if(tp->next) {
                  tp->next->prev = tp->prev;
               } else {
                  taskEntriesTail = tp->prev;
               }
               Release(tp);
            }
            UpdateTaskBar();
            UpdateNetClientList();
            return;
         }
      }
   }
}

/** Update all task bars. */
void UpdateTaskBar(void)
{
   TaskBarType *bp;
   int lastHeight = -1;

   if(JUNLIKELY(shouldExit)) {
      return;
   }

   for(bp = bars; bp; bp = bp->next) {
      if(bp->layout == LAYOUT_VERTICAL) {
         TaskEntry *tp;
         lastHeight = bp->cp->requestedHeight;
         bp->cp->requestedHeight = 2;
         for(; tp; tp = tp->next) {
            bp->cp->requestedHeight += bp->cp->width;
         }
         if(lastHeight != bp->cp->requestedHeight) {
            ResizeTray(bp->cp->tray);
         }
      }
      Render(bp);
   }
}

/** Signal task bar (for popups). */
void SignalTaskbar(const TimeType *now, int x, int y, Window w, void *data)
{

   TaskBarType *bp = (TaskBarType*)data;
   TaskEntry *ep;

   if(w == bp->cp->tray->window &&
      abs(bp->mousex - x) < settings.doubleClickDelta &&
      abs(bp->mousey - y) < settings.doubleClickDelta) {
      if(GetTimeDifference(now, &bp->mouseTime) >= settings.popupDelay) {
         ep = GetEntry(bp, x - bp->cp->screenx, y - bp->cp->screeny);
         if(ep && ep->clients->client->className) {
            ShowPopup(x, y, ep->clients->client->className);
         }
      }
   }

}

/** Draw a specific task bar. */
void Render(const TaskBarType *bp)
{
   TaskEntry *tp;
   ButtonNode button;
   int x, y;

   if(JUNLIKELY(shouldExit)) {
      return;
   }

   ClearTrayDrawable(bp->cp);
   if(!taskEntries) {
      UpdateSpecificTray(bp->cp->tray, bp->cp);
      return;
   }

   ResetButton(&button, bp->cp->pixmap, &rootVisual);
   button.font = FONT_TASK;
   button.height = bp->itemHeight;
   button.width = bp->itemWidth;
   button.text = NULL;

   x = 0;
   y = 0;
   for(tp = taskEntries; tp; tp = tp->next) {

      if(!ShouldShowEntry(tp)) {
         continue;
      }

      /* Check for an active or urgent window. */
      ClientEntry *cp;
      button.type = BUTTON_TASK;
      for(cp = tp->clients; cp; cp = cp->next) {
         if(cp->client->state.status & (STAT_ACTIVE | STAT_FLASH)) {
            button.type = BUTTON_TASK_ACTIVE;
            break;
         }
      }
      button.x = x;
      button.y = y;
      button.icon = tp->clients->client->icon;
      DrawButton(&button);

      if(bp->layout == LAYOUT_HORIZONTAL) {
         x += bp->itemWidth;
      } else {
         y += bp->itemHeight;
      }
   }

   UpdateSpecificTray(bp->cp->tray, bp->cp);

}

/** Focus the next client in the task bar. */
void FocusNext(void)
{
#if 0
   TaskEntry *tp;
   ClientEntry *cp;

   /* Find the current entry. */
   for(tp = taskEntries; tp; tp = tp->next) {
      for(cp = tp->clients; cp; cp = cp->next) {
         if(ShouldFocus(cp->client)) {
            if(cp->client->state.status & STAT_ACTIVE) {
               cp = cp->next;
               goto ClientFound;
            }
         }
      }
   }
ClientFound:
   if(!cp && tp) {
      tp = tp->next;
      if(tp) {
         cp = tp->clients;
      }
   }
   if(!tp) {
      tp = taskEntries;
      
   } else if(!cp) {
      
   }

   if(!tp) {
      tp = taskBarNodes;
   }

   while(tp && !ShouldFocus(tp->client)) {
      tp = tp->next;
   }

   if(!tp) {
      tp = taskBarNodes;
      while(tp && !ShouldFocus(tp->client)) {
         tp = tp->next;
      }
   }

   if(tp) {
      RestoreClient(tp->client, 1);
      FocusClient(tp->client);
   }
#endif

}

/** Focus the previous client in the task bar. */
void FocusPrevious(void)
{
#if 0

   Node *tp;

   for(tp = taskBarNodesTail; tp; tp = tp->prev) {
      if(ShouldFocus(tp->client)) {
         if(tp->client->state.status & STAT_ACTIVE) {
            tp = tp->prev;
            break;
         }
      }
   }

   if(!tp) {
      tp = taskBarNodesTail;
   }

   while(tp && !ShouldFocus(tp->client)) {
      tp = tp->prev;
   }

   if(!tp) {
      tp = taskBarNodesTail;
      while(tp && !ShouldFocus(tp->client)) {
         tp = tp->prev;
      }
   }

   if(tp) {
      RestoreClient(tp->client, 1);
      FocusClient(tp->client);
   }
#endif
}

/** Determine if there is anything to show for the specified entry. */
char ShouldShowEntry(const TaskEntry *tp)
{
   const ClientEntry *cp;
   for(cp = tp->clients; cp; cp = cp->next) {
      if(ShouldFocus(cp->client)) {
         return 1;
      }
   }
   return 0;
}

/** Get the item associated with a coordinate on the task bar. */
TaskEntry *GetEntry(TaskBarType *bar, int x, int y)
{
   TaskEntry *tp;
   int offset;

   offset = 0;
   for(tp = taskEntries; tp; tp = tp->next) {
      if(!ShouldShowEntry(tp)) {
         continue;
      }
      offset += bar->itemWidth;
      if(bar->layout == LAYOUT_HORIZONTAL) {
         if(x < offset) {
            return tp;
         }
      } else {
         offset += bar->itemHeight;
         if(y < offset) {
            return tp;
         }
      }
   }

   return NULL;
}

/** Set the maximum width of an item in the task bar. */
void SetMaxTaskBarItemWidth(TrayComponentType *cp, const char *value)
{
#if 0
   TaskBarType *bp = (TaskBarType*)cp->object;
   int temp;

   Assert(cp);
   Assert(value);

   temp = atoi(value);
   if(JUNLIKELY(temp < 0)) {
      Warning(_("invalid maxwidth for TaskList: %s"), value);
      return;
   }
   bp->maxItemWidth = temp;
#endif

}

/** Maintain the _NET_CLIENT_LIST[_STACKING] properties on the root. */
void UpdateNetClientList(void)
{
   TaskEntry *tp;
   ClientNode *client;
   Window *windows;
   unsigned int count;
   int layer;

   /* Determine how much we need to allocate. */
   if(clientCount == 0) {
      windows = NULL;
   } else {
      windows = AllocateStack(clientCount * sizeof(Window));
   }

   /* Set _NET_CLIENT_LIST */
   count = 0;
   for(tp = taskEntries; tp; tp = tp->next) {
      ClientEntry *cp;
      for(cp = tp->clients; cp; cp = cp->next) {
         windows[count] = cp->client->window;
         count += 1;
      }
   }
   Assert(count <= clientCount);
   JXChangeProperty(display, rootWindow, atoms[ATOM_NET_CLIENT_LIST],
                    XA_WINDOW, 32, PropModeReplace,
                    (unsigned char*)windows, count);

   /* Set _NET_CLIENT_LIST_STACKING */
   count = 0;
   for(layer = FIRST_LAYER; layer <= LAST_LAYER; layer++) {
      for(client = nodes[layer]; client; client = client->next) {
         windows[count] = client->window;
         count += 1;
      }
   }
   JXChangeProperty(display, rootWindow, atoms[ATOM_NET_CLIENT_LIST_STACKING],
                    XA_WINDOW, 32, PropModeReplace,
                    (unsigned char*)windows, count);

   if(windows != NULL) {
      ReleaseStack(windows);
   }
   
}

