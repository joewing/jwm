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
#include "settings.h"
#include "event.h"
#include "mouse.h"
#include "move.h"
#include "resize.h"

typedef struct TaskBarType {

   TrayComponentType *cp;

   int itemHeight;
   LayoutType layout;
   char border;

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

static const int TASK_SPACER = 2;

static TaskBarType *bars;
static Node *taskBarNodes;
static Node *taskBarNodesTail;

static Node *GetNode(TaskBarType *bar, int x);
static unsigned int GetItemCount();
static unsigned int GetItemWidth(const TaskBarType *bp,
                                 unsigned int itemCount);
static void Render(const TaskBarType *bp);

static void SetSize(TrayComponentType *cp, int width, int height);
static void Create(TrayComponentType *cp);
static void Resize(TrayComponentType *cp);
static void ProcessTaskButtonEvent(TrayComponentType *cp,
                                   const XButtonEvent *event,
                                   int x, int y);
static void ProcessTaskMotionEvent(TrayComponentType *cp,
                                   int x, int y, int mask);
static void SignalTaskbar(const TimeType *now, int x, int y, void *data);

/** Initialize task bar data. */
void InitializeTaskBar()
{
   bars = NULL;
   taskBarNodes = NULL;
   taskBarNodesTail = NULL;
}

/** Shutdown the task bar. */
void ShutdownTaskBar()
{
   TaskBarType *bp;
   for(bp = bars; bp; bp = bp->next) {
      UnregisterCallback(SignalTaskbar, bp);
      JXFreePixmap(display, bp->buffer);
   }
}

/** Destroy task bar data. */
void DestroyTaskBar()
{
   TaskBarType *bp;
   while(bars) {
      bp = bars->next;
      Release(bars);
      bars = bp;
   }
}

/** Create a new task bar tray component. */
TrayComponentType *CreateTaskBar(char border)
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
   tp->maxItemWidth = 0;
   tp->border = border;

   cp = CreateTrayComponent();
   cp->object = tp;
   tp->cp = cp;

   cp->SetSize = SetSize;
   cp->Create = Create;
   cp->Resize = Resize;
   cp->ProcessButtonEvent = ProcessTaskButtonEvent;
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

   ClearTrayDrawable(cp);

}

/** Resize a task bar tray component. */
void Resize(TrayComponentType *cp)
{

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

   ClearTrayDrawable(cp);
}

/** Process a task list button event. */
void ProcessTaskButtonEvent(TrayComponentType *cp,
                            const XButtonEvent *event,
                            int x, int y)
{

   TaskBarType *bar = (TaskBarType*)cp->object;
   Node *np;
   ActionContext context;

   Assert(bar);

   if(bar->layout == LAYOUT_HORIZONTAL) {
      np = GetNode(bar, x);
   } else {
      np = GetNode(bar, y);
   }
   InitActionContext(&context);
   context.client = np ? np->client : NULL;
   context.x = event->x_root;
   context.y = event->y_root;
   context.ResizeFunc = ResizeClientKeyboard;
   context.MoveFunc = MoveClientKeyboard;

   RunMouseCommand(event, CONTEXT_TASK, &context);

}

/** Process a task list motion event. */
void ProcessTaskMotionEvent(TrayComponentType *cp, int x, int y, int mask)
{
   TaskBarType *bp = (TaskBarType*)cp->object;
   bp->mousex = cp->screenx + x;
   bp->mousey = cp->screeny + y;
   GetCurrentTime(&bp->mouseTime);
}

/** Add a client to the task bar. */
void AddClientToTaskBar(ClientNode *np)
{

   Node *tp;

   Assert(np);

   tp = Allocate(sizeof(Node));
   tp->client = np;

   if(settings.taskInsertMode == INSERT_RIGHT) {
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
void RemoveClientFromTaskBar(ClientNode *np)
{

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
void UpdateTaskBar()
{

   TaskBarType *bp;
   int lastHeight;

   if(JUNLIKELY(shouldExit)) {
      return;
   }

   for(bp = bars; bp; bp = bp->next) {
      if(bp->layout == LAYOUT_VERTICAL) {
         lastHeight = bp->cp->requestedHeight;
         bp->cp->requestedHeight = GetStringHeight(FONT_TASK) + 12;
         bp->cp->requestedHeight *= GetItemCount();
         bp->cp->requestedHeight += 2;
         if(lastHeight != bp->cp->requestedHeight) {
            ResizeTray(bp->cp->tray);
         }
      }
      Render(bp);
   }

}

/** Signal task bar (for popups). */
void SignalTaskbar(const TimeType *now, int x, int y, void *data)
{

   TaskBarType *bp = (TaskBarType*)data;
   Node *np;

   if(abs(bp->mousex - x) < settings.doubleClickDelta
      && abs(bp->mousey - y) < settings.doubleClickDelta) {
      if(GetTimeDifference(now, &bp->mouseTime) >= settings.popupDelay) {
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

/** Draw a specific task bar. */
void Render(const TaskBarType *bp)
{

   Node *tp;
   ButtonNode button;
   int x, y;
   int remainder;
   int itemWidth, itemCount;
   int width, height;
   Pixmap buffer;
   GC gc;
   char *minimizedName;

   if(JUNLIKELY(shouldExit)) {
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

   ClearTrayDrawable(bp->cp);

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

   ResetButton(&button, buffer, gc);
   button.font = FONT_TASK;

   for(tp = taskBarNodes; tp; tp = tp->next) {
      if(ShouldFocus(tp->client)) {

         tp->y = y;

         if(tp->client->state.status & (STAT_ACTIVE | STAT_FLASH)) {
            button.type = BUTTON_TASK_ACTIVE;
            button.border = 1;
         } else {
            button.border = bp->border;
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
            const int isize = (bp->itemHeight + 7) / 8;
            int i;
            JXSetForeground(display, gc, colors[COLOR_TASK_FG]);
            for(i = 0; i <= isize; i++) {
               const int xc = x + i + 3;
               const int y1 = bp->itemHeight - 3 - isize + i;
               const int y2 = bp->itemHeight - 3;
               JXDrawLine(display, buffer, gc, xc, y1, xc, y2);
            }
         }

         if(bp->layout == LAYOUT_HORIZONTAL) {
            x += itemWidth;
            if(remainder) {
               x += 1;
               remainder -= 1;
            }
         } else {
            y += bp->itemHeight;
            if(remainder) {
               y += 1;
               remainder -= 1;
            }
         }

      }
   }

   UpdateSpecificTray(bp->cp->tray, bp->cp);

}

/** Focus the next client in the task bar. */
void FocusNext()
{

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
void FocusPrevious()
{

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
Node *GetNode(TaskBarType *bar, int x)
{

   Node *tp;
   int remainder;
   unsigned int itemCount;
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
         if(ShouldFocus(tp->client)) {
            if(remainder) {
               stop = index + itemWidth + 1;
               remainder -= 1;
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
         if(ShouldFocus(tp->client)) {
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
unsigned int GetItemCount()
{

   Node *tp;
   unsigned int count;

   count = 0;
   for(tp = taskBarNodes; tp; tp = tp->next) {
      if(ShouldFocus(tp->client)) {
         count += 1;
      }
   }

   return count;

}

/** Get the width of an item in the task bar. */
unsigned int GetItemWidth(const TaskBarType *bp, unsigned int itemCount)
{

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
void SetMaxTaskBarItemWidth(TrayComponentType *cp, const char *value)
{

   TaskBarType *bp;
   int temp;

   Assert(cp);
   Assert(value);

   temp = atoi(value);
   if(JUNLIKELY(temp < 0)) {
      Warning(_("invalid maxwidth for TaskList: %s"), value);
      return;
   }
   bp = (TaskBarType*)cp->object;
   bp->maxItemWidth = temp;

}

/** Maintain the _NET_CLIENT_LIST[_STACKING] properties on the root. */
void UpdateNetClientList()
{

   Node *np;
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
   for(np = taskBarNodes; np; np = np->next) {
      windows[count] = np->client->window;
      count += 1;
   }
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

