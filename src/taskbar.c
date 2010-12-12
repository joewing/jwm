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
#include "font.h"
#include "winmenu.h"
#include "screen.h"

typedef enum {
   INSERT_LEFT,
   INSERT_RIGHT
} InsertModeType;

typedef struct TaskBarType {

   TrayComponentType *cp;

   int itemHeight;
   LayoutType layout;

   Pixmap buffer;

   TimeType mouseTime;
   int mousex, mousey;

   unsigned int maxItemWidth;

   struct TaskBarType *next;

} TaskBarType;

typedef struct Node {
   ClientNode *client;
   int y;
   struct Node *next;
   struct Node *prev;
} Node;

static char minimized_bitmap[] = {
   0x01, 0x03,
   0x07, 0x0F
};

static const int TASK_SPACER = 2;

static Pixmap minimizedPixmap;
static InsertModeType insertMode;

static TaskBarType *bars;
static Node *taskBarNodes;
static Node *taskBarNodesTail;

static Node *GetNode(TaskBarType *bar, int x);
static unsigned int GetItemCount();
static int ShouldShowItem(const ClientNode *np);
static unsigned int GetItemWidth(const TaskBarType *bp,
   unsigned int itemCount);
static void Render(const TaskBarType *bp);
static void ShowTaskWindowMenu(TaskBarType *bar, Node *np);

static void SetSize(TrayComponentType *cp, int width, int height);
static void Create(TrayComponentType *cp);
static void Resize(TrayComponentType *cp);
static void ProcessTaskButtonEvent(TrayComponentType *cp,
   int x, int y, int mask);
static void ProcessTaskMotionEvent(TrayComponentType *cp,
   int x, int y, int mask);

/** Initialize task bar data. */
void InitializeTaskBar() {
   bars = NULL;
   taskBarNodes = NULL;
   taskBarNodesTail = NULL;
   insertMode = INSERT_RIGHT;
}

/** Startup the task bar. */
void StartupTaskBar() {
   minimizedPixmap = JXCreateBitmapFromData(display, rootWindow,
      minimized_bitmap, 4, 4);
}

/** Shutdown the task bar. */
void ShutdownTaskBar() {

   TaskBarType *bp;

   for(bp = bars; bp; bp = bp->next) {
      JXFreePixmap(display, bp->buffer);
   }

   JXFreePixmap(display, minimizedPixmap);
}

/** Destroy task bar data. */
void DestroyTaskBar() {

   TaskBarType *bp;

   while(bars) {
      bp = bars->next;
      Release(bars);
      bars = bp;
   }

}

/** Create a new task bar tray component. */
TrayComponentType *CreateTaskBar() {

   TrayComponentType *cp;
   TaskBarType *tp;

   tp = Allocate(sizeof(TaskBarType));
   tp->next = bars;
   bars = tp;
   tp->itemHeight = 0;
   tp->layout = LAYOUT_HORIZONTAL;
   tp->mousex = -1;
   tp->mousey = -1;
   tp->mouseTime.seconds = 0;
   tp->mouseTime.ms = 0;
   tp->maxItemWidth = 0;

   cp = CreateTrayComponent();
   cp->object = tp;
   tp->cp = cp;

   cp->SetSize = SetSize;
   cp->Create = Create;
   cp->Resize = Resize;
   cp->ProcessButtonPress = ProcessTaskButtonEvent;
   cp->ProcessMotionEvent = ProcessTaskMotionEvent;

   return cp;

}

/** Set the size of a task bar tray component. */
void SetSize(TrayComponentType *cp, int width, int height) {

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
void Create(TrayComponentType *cp) {

   TaskBarType *tp;

   Assert(cp);

   tp = (TaskBarType*)cp->object;

   Assert(tp);

   if(tp->layout == LAYOUT_HORIZONTAL) {
      tp->itemHeight = cp->height - TASK_SPACER;
   } else {
      tp->itemHeight = GetStringHeight(FONT_TASK) + 12;
   }

   Assert(cp->width > 0);
   Assert(cp->height > 0);

   cp->pixmap = JXCreatePixmap(display, rootWindow, cp->width, cp->height,
      rootDepth);
   tp->buffer = cp->pixmap;

   JXSetForeground(display, rootGC, colors[COLOR_TRAY_BG]);
   JXFillRectangle(display, cp->pixmap, rootGC, 0, 0, cp->width, cp->height);

}

/** Resize a task bar tray component. */
void Resize(TrayComponentType *cp) {

   TaskBarType *tp;

   Assert(cp);

   tp = (TaskBarType*)cp->object;

   Assert(tp);

   if(tp->buffer != None) {
      JXFreePixmap(display, tp->buffer);
   }

   if(tp->layout == LAYOUT_HORIZONTAL) {
      tp->itemHeight = cp->height - TASK_SPACER;
   } else {
      tp->itemHeight = GetStringHeight(FONT_TASK) + 12;
   }

   Assert(cp->width > 0);
   Assert(cp->height > 0);

   cp->pixmap = JXCreatePixmap(display, rootWindow, cp->width, cp->height,
      rootDepth);
   tp->buffer = cp->pixmap;

   JXSetForeground(display, rootGC, colors[COLOR_TRAY_BG]);
   JXFillRectangle(display, cp->pixmap, rootGC,
      0, 0, cp->width, cp->height);
}

/** Process a task list button event. */
void ProcessTaskButtonEvent(TrayComponentType *cp, int x, int y, int mask) {

   TaskBarType *bar = (TaskBarType*)cp->object;
   Node *np;

   Assert(bar);

   if(bar->layout == LAYOUT_HORIZONTAL) {
      np = GetNode(bar, x);
   } else {
      np = GetNode(bar, y);
   }

   if(np) {
      switch(mask) {
      case Button1:
         if(np->client->state.status & STAT_ACTIVE
            && np->client == nodes[np->client->state.layer]) {
            MinimizeClient(np->client);
         } else {
            RestoreClient(np->client, 1);
            FocusClient(np->client);
         }
         break;
      case Button3:
         ShowTaskWindowMenu(bar, np);
         break;
      case Button4:
         FocusPrevious();
         break;
      case Button5:
         FocusNext();
         break;
      default:
         break;
      }
   }

}

/** Process a task list motion event. */
void ProcessTaskMotionEvent(TrayComponentType *cp, int x, int y, int mask) {

   TaskBarType *bp = (TaskBarType*)cp->object;

   bp->mousex = cp->screenx + x;
   bp->mousey = cp->screeny + y;
   GetCurrentTime(&bp->mouseTime);

}

/** Show the menu associated with a task list item. */
void ShowTaskWindowMenu(TaskBarType *bar, Node *np) {

   int x, y;
   int mwidth, mheight;
   const ScreenType *sp;

   GetWindowMenuSize(np->client, &mwidth, &mheight);

   sp = GetCurrentScreen(x, y);

   if(bar->layout == LAYOUT_HORIZONTAL) {
      GetMousePosition(&x, &y);
      if(bar->cp->screeny + bar->cp->height / 2 < sp->y + sp->height / 2) {
         y = bar->cp->screeny + bar->cp->height;
      } else {
         y = bar->cp->screeny - mheight;
      }
      x -= mwidth / 2;
   } else {
      if(bar->cp->screenx + bar->cp->width / 2 < sp->x + sp->width / 2) {
         x = bar->cp->screenx + bar->cp->width;
      } else {
         x = bar->cp->screenx - mwidth;
      }
      y = bar->cp->screeny + np->y;
   }

   ShowWindowMenu(np->client, x, y);

}

/** Add a client to the task bar. */
void AddClientToTaskBar(ClientNode *np) {

   Node *tp;

   Assert(np);

   tp = Allocate(sizeof(Node));
   tp->client = np;

   if(insertMode == INSERT_RIGHT) {
      tp->next = NULL;
      tp->prev = taskBarNodesTail;
      if(taskBarNodesTail) {
         taskBarNodesTail->next = tp;
      } else {
         taskBarNodes = tp;
      }
      taskBarNodesTail = tp;
   } else {
      tp->prev = NULL;
      tp->next = taskBarNodes;
      if(taskBarNodes) {
         taskBarNodes->prev = tp;
      }
      taskBarNodes = tp;
      if(!taskBarNodesTail) {
         taskBarNodesTail = tp;
      }
   }

   UpdateTaskBar();

   UpdateNetClientList();

}

/** Remove a client from the task bar. */
void RemoveClientFromTaskBar(ClientNode *np) {

   Node *tp;

   Assert(np);

   for(tp = taskBarNodes; tp; tp = tp->next) {
      if(tp->client == np) {
         if(tp->prev) {
            tp->prev->next = tp->next;
         } else {
            taskBarNodes = tp->next;
         }
         if(tp->next) {
            tp->next->prev = tp->prev;
         } else {
            taskBarNodesTail = tp->prev;
         }
         Release(tp);
         break;
      }
   }

   UpdateTaskBar();

   UpdateNetClientList();

}

/** Update all task bars. */
void UpdateTaskBar() {

   TaskBarType *bp;
   unsigned int count;
   int lastHeight;

   if(shouldExit) {
      return;
   }

   for(bp = bars; bp; bp = bp->next) {

      if(bp->layout == LAYOUT_VERTICAL) {
         lastHeight = bp->cp->requestedHeight;
         count = GetItemCount();
         bp->cp->requestedHeight = GetStringHeight(FONT_TASK) + 12;
         bp->cp->requestedHeight *= count;
         bp->cp->requestedHeight += 2;
         if(lastHeight != bp->cp->requestedHeight) {
            ResizeTray(bp->cp->tray);
         }
      }

      Render(bp);
   }

}

/** Signal task bar (for popups). */
void SignalTaskbar(const TimeType *now, int x, int y) {

   TaskBarType *bp;
   Node *np;

   for(bp = bars; bp; bp = bp->next) {
      if(abs(bp->mousex - x) < POPUP_DELTA
         && abs(bp->mousey - y) < POPUP_DELTA) {
         if(GetTimeDifference(now, &bp->mouseTime) >= popupDelay) {
            if(bp->layout == LAYOUT_HORIZONTAL) {
               np = GetNode(bp, x - bp->cp->screenx);
            } else {
               np = GetNode(bp, y - bp->cp->screeny);
            }
            if(np && np->client->name) {
               ShowPopup(x, y, np->client->name);
            }
         }
      }
   }

}

/** Draw a specific task bar. */
void Render(const TaskBarType *bp) {

   Node *tp;
   ButtonNode button;
   int x, y;
   int remainder;
   int itemWidth, itemCount;
   int width, height;
   int iconSize;
   Pixmap buffer;
   GC gc;
   char *minimizedName;

   if(shouldExit) {
      return;
   }

   Assert(bp);
   Assert(bp->cp);

   width = bp->cp->width;
   height = bp->cp->height;
   buffer = bp->cp->pixmap;
   gc = rootGC;

   x = TASK_SPACER;
   width -= x;
   y = 1;

   JXSetForeground(display, gc, colors[COLOR_TRAY_BG]);
   JXFillRectangle(display, buffer, gc, 0, 0, width, height);

   itemCount = GetItemCount();
   if(!itemCount) {
      UpdateSpecificTray(bp->cp->tray, bp->cp);
      return;
   }
   if(bp->layout == LAYOUT_HORIZONTAL) {
      itemWidth = GetItemWidth(bp, itemCount);
      remainder = width - itemWidth * itemCount;
   } else {
      itemWidth = width;
      remainder = 0;
   }

   iconSize = bp->itemHeight - 2 * TASK_SPACER - 4;

   ResetButton(&button, buffer, gc);
   button.font = FONT_TASK;

   for(tp = taskBarNodes; tp; tp = tp->next) {
      if(ShouldShowItem(tp->client)) {

         tp->y = y;

         if(tp->client->state.status & STAT_ACTIVE) {
            button.type = BUTTON_TASK_ACTIVE;
         } else {
            button.type = BUTTON_TASK;
         }

         if(remainder) {
            button.width = itemWidth - TASK_SPACER;
         } else {
            button.width = itemWidth - TASK_SPACER - 1;
         }
         button.height = bp->itemHeight - 1;
         button.x = x;
         button.y = y;
         button.icon = tp->client->icon;

         if(tp->client->state.status & STAT_MINIMIZED) {
            if(tp->client->name) {
               minimizedName = AllocateStack(strlen(tp->client->name) + 3);
               sprintf(minimizedName, "[%s]", tp->client->name);
               button.text = minimizedName;
               DrawButton(&button);
               ReleaseStack(minimizedName);
            } else {
               button.text = "[]";
               DrawButton(&button);
            }
         } else {
            button.text = tp->client->name;
            DrawButton(&button);
         }

         if(tp->client->state.status & STAT_MINIMIZED) {
            JXSetForeground(display, gc, colors[COLOR_TASK_FG]);
            JXSetClipMask(display, gc, minimizedPixmap);
            JXSetClipOrigin(display, gc, x + 3, y + bp->itemHeight - 7);
            JXFillRectangle(display, buffer, gc,
               x + 3, y + bp->itemHeight - 7, 4, 4);
            JXSetClipMask(display, gc, None);
         }

         if(bp->layout == LAYOUT_HORIZONTAL) {
            x += itemWidth;
            if(remainder) {
               ++x;
               --remainder;
            }
         } else {
            y += bp->itemHeight;
            if(remainder) {
               ++y;
               --remainder;
            }
         }

      }
   }

   UpdateSpecificTray(bp->cp->tray, bp->cp);

}

/** Focus the next client in the task bar. */
void FocusNext() {

   Node *tp;

   for(tp = taskBarNodes; tp; tp = tp->next) {
      if(ShouldFocus(tp->client)) {
         if(tp->client->state.status & STAT_ACTIVE) {
            tp = tp->next;
            break;
         }
      }
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

}

/** Focus the previous client in the task bar. */
void FocusPrevious() {

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

}

/** Get the item associated with an x-coordinate on the task bar. */
Node *GetNode(TaskBarType *bar, int x) {

   Node *tp;
   int remainder;
   int itemCount;
   int itemWidth;
   int index, stop;
   int width;

   index = TASK_SPACER;

   itemCount = GetItemCount();

   if(bar->layout == LAYOUT_HORIZONTAL) {

      width = bar->cp->width - index; 
      itemWidth = GetItemWidth(bar, itemCount);
      remainder = width - itemWidth * itemCount;

      for(tp = taskBarNodes; tp; tp = tp->next) {
         if(ShouldShowItem(tp->client)) {
            if(remainder) {
               stop = index + itemWidth + 1;
               --remainder;
            } else {
               stop = index + itemWidth;
            }
            if(x >= index && x < stop) {
               return tp;
            }
            index = stop;
         }
      }

   } else {

      for(tp = taskBarNodes; tp; tp = tp->next) {
         if(ShouldShowItem(tp->client)) {
            stop = index + bar->itemHeight;
            if(x >= index && x < stop) {
               return tp;
            }
            index = stop;
         }
      }

   }

   return NULL;

}

/** Get the number of items on the task bar. */
unsigned int GetItemCount() {

   Node *tp;
   unsigned int count;

   count = 0;
   for(tp = taskBarNodes; tp; tp = tp->next) {
      if(ShouldShowItem(tp->client)) {
         ++count;
      }
   }

   return count;

}

/** Determine if a client should be shown on the task bar. */
int ShouldShowItem(const ClientNode *np) {

   /* Only display clients on the current desktop or clients that are sticky. */
   if(np->state.desktop != currentDesktop
      && !(np->state.status & STAT_STICKY)) {
      return 0;
   }

   /* Don't display a client if it doesn't want to be displayed. */
   if(np->state.status & STAT_NOLIST) {
      return 0;
   }

   /* Don't display a client on the tray if it has an owner. */
   if(np->owner != None) {
      return 0;
   }

   if(   !(np->state.status & STAT_MAPPED)
      && !(np->state.status & (STAT_MINIMIZED | STAT_SHADED))) {
      return 0;
   }

   return 1;

}

/** Get the width of an item in the task bar. */
unsigned int GetItemWidth(const TaskBarType *bp, unsigned int itemCount) {

   unsigned int itemWidth;

   itemWidth = bp->cp->width - TASK_SPACER;

   if(!itemCount) {
      return itemWidth;
   }

   itemWidth /= itemCount;
   if(!itemWidth) {
      itemWidth = 1;
   }

   if(bp->maxItemWidth > 0 && itemWidth > bp->maxItemWidth) {
      itemWidth = bp->maxItemWidth;
   }

   return itemWidth;

}

/** Set the maximum width of an item in the task bar. */
void SetMaxTaskBarItemWidth(TrayComponentType *cp, const char *value) {

   int temp;
   TaskBarType *bp;

   Assert(cp);

   if(value) {
      temp = atoi(value);
      if(temp < 0) {
         Warning("invalid maxwidth for TaskList: %s", value);
         return;
      }
      bp = (TaskBarType*)cp->object;
      bp->maxItemWidth = temp;
   }

}

/** Set the way items are inserted into the task bar. */
void SetTaskBarInsertMode(const char *mode) {

   if(!mode) {
      insertMode = INSERT_RIGHT;
      return;
   }

   if(!strcmp(mode, "right")) {
      insertMode = INSERT_RIGHT;
   } else if(!strcmp(mode, "left")) {
      insertMode = INSERT_LEFT;
   } else {
      Warning("invalid insert mode: \"%s\"", mode);
      insertMode = INSERT_RIGHT;
   }

}

/** Maintain the _NET_CLIENT_LIST[_STACKING] properties on the root. */
void UpdateNetClientList() {

   Node *np;
   ClientNode *client;
   Window *windows;
   int count, temp;
   int layer;

   /* Determine how much we need to allocate. */
   count = 0;
   for(np = taskBarNodes; np; np = np->next) {
      ++count;
   }
   temp = 0;
   for(layer = LAYER_BOTTOM; layer <= LAYER_TOP; layer++) {
      for(client = nodes[layer]; client; client = client->next) {
         ++temp;
      }
   }
   if(temp > count) {
      count = temp;
   }

   if(count == 0) {
      windows = NULL;
   } else {
      windows = AllocateStack(count * sizeof(Window));
   }

   /* Set _NET_CLIENT_LIST */
   count = 0;
   for(np = taskBarNodes; np; np = np->next) {
      windows[count++] = np->client->window;
   }
   JXChangeProperty(display, rootWindow, atoms[ATOM_NET_CLIENT_LIST],
      XA_WINDOW, 32, PropModeReplace, (unsigned char*)windows, count);

   /* Set _NET_CLIENT_LIST_STACKING */
   count = 0;
   for(layer = LAYER_BOTTOM; layer <= LAYER_TOP; layer++) {
      for(client = nodes[layer]; client; client = client->next) {
         windows[count++] = client->window;
      }
   }
   JXChangeProperty(display, rootWindow, atoms[ATOM_NET_CLIENT_LIST_STACKING],
      XA_WINDOW, 32, PropModeReplace, (unsigned char*)windows, count);

   if(windows != NULL) {
      ReleaseStack(windows);
   }
   
}

