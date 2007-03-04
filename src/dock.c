/**
 * @file dock.c
 * @author Joe Wingbermuehle
 * @date 2006
 *
 * @brief Dock tray component.
 *
 */

#include "jwm.h"
#include "dock.h"
#include "tray.h"
#include "main.h"
#include "error.h"
#include "color.h"

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

#define SYSTEM_TRAY_ORIENTATION_HORZ 0
#define SYSTEM_TRAY_ORIENTATION_VERT 1

/** Structure to represent a docked window. */
typedef struct DockNode {

   Window window;
   int needs_reparent;

   struct DockNode *next;

} DockNode;

/** Structure to represent a dock tray component. */
typedef struct DockType {

   TrayComponentType *cp;

   Window window;

   DockNode *nodes;

} DockType;

static const char *BASE_SELECTION_NAME = "_NET_SYSTEM_TRAY_S%d";
static const char *ORIENTATION_ATOM = "_NET_SYSTEM_TRAY_ORIENTATION";

static DockType *dock = NULL;
static int owner = 0;
static Atom dockAtom;
static unsigned long orientation;

static void SetSize(TrayComponentType *cp, int width, int height);
static void Create(TrayComponentType *cp);
static void Resize(TrayComponentType *cp);

static void DockWindow(Window win);
static int UndockWindow(Window win);

static void UpdateDock();

/** Initialize dock data. */
void InitializeDock() {
}

/** Startup the dock. */
void StartupDock() {

   char *selectionName;

   if(!dock) {
      /* No dock has been requested. */
      return;
   }

   if(!dock->cp) {
      /* The Dock item has been removed from the configuration. */
      JXDestroyWindow(display, dock->window);
      Release(dock);
      dock = NULL;
      return;
   }

   if(dock->window == None) {

      /* No dock yet. */

      /* Get the selection atom. */
      selectionName = AllocateStack(strlen(BASE_SELECTION_NAME) + 1);
      sprintf(selectionName, BASE_SELECTION_NAME, rootScreen);
      dockAtom = JXInternAtom(display, selectionName, False);
      ReleaseStack(selectionName);

      /* The location and size of the window doesn't matter here. */
      dock->window = JXCreateSimpleWindow(display, rootWindow,
         /* x, y, width, height */ 0, 0, 1, 1,
         /* border_size, border_color */ 0, 0,
         /* background */ colors[COLOR_TRAY_BG]);
      JXSelectInput(display, dock->window,
           SubstructureNotifyMask
         | SubstructureRedirectMask
         | PointerMotionMask | PointerMotionHintMask);

   }
   dock->cp->window = dock->window;

}

/** Shutdown the dock. */
void ShutdownDock() {

   DockNode *np;

   if(dock) {

      if(shouldRestart) {

         /* If restarting we just reparent the dock window to the root
          * window. We need to keep the dock around and visible so that
          * we don't cause problems with the docked windows.
          * It seems every application handles docking differently...
          */
         JXReparentWindow(display, dock->window, rootWindow, 0, 0);

      } else {

         /* JWM is exiting. */

         /* Release memory used by the dock list. */
         while(dock->nodes) {
            np = dock->nodes->next;
            JXReparentWindow(display, dock->nodes->window, rootWindow, 0, 0);
            Release(dock->nodes);
            dock->nodes = np;
         }

         /* Release the selection. */
         if(owner) {
            JXSetSelectionOwner(display, dockAtom, None, CurrentTime);
         }

         /* Destroy the dock window. */
         JXDestroyWindow(display, dock->window);

      }

   }

}

/** Destroy dock data. */
void DestroyDock() {

   if(dock) {
      if(shouldRestart) {
         dock->cp = NULL;
      } else {
         Release(dock);
         dock = NULL;
      }
   }

}

/** Create a dock component. */
TrayComponentType *CreateDock() {

   TrayComponentType *cp;

   if(dock != NULL && dock->cp != NULL) {
      Warning("only one Dock allowed");
      return NULL;
   } else if(dock == NULL) {
      dock = Allocate(sizeof(DockType));
      dock->nodes = NULL;
      dock->window = None;
   }

   cp = CreateTrayComponent();
   cp->object = dock;
   dock->cp = cp;
   cp->requestedWidth = 1;
   cp->requestedHeight = 1;

   cp->SetSize = SetSize;
   cp->Create = Create;
   cp->Resize = Resize;

   return cp;

}

/** Set the size of a dock component. */
void SetSize(TrayComponentType *cp, int width, int height) {

   int count;
   DockNode *np;

   Assert(cp);
   Assert(dock);

   count = 0;
   for(np = dock->nodes; np; np = np->next) {
      ++count;
   }

   if(width == 0) {
      if(count > 0) {
         cp->width = count * height;
         cp->requestedWidth = cp->width;
      } else {
         cp->width = 1;
         cp->requestedWidth = 1;
      }
   } else if(height == 0) {
      if(count > 0) {
         cp->height = count * width;
         cp->requestedHeight = cp->height;
      } else {
         cp->height = 1;
         cp->requestedHeight = 1;
      }
   }

}

/** Initialize a dock component. */
void Create(TrayComponentType *cp) {

   XEvent event;
   Atom orientationAtom;

   Assert(cp);

   /* Map the dock window. */
   if(cp->window != None) {
      JXResizeWindow(display, cp->window, cp->width, cp->height);
      JXMapRaised(display, cp->window);
   }

   /* Set the orientation. */
   orientationAtom = JXInternAtom(display, ORIENTATION_ATOM, False);
   if(cp->height == 1) {
      orientation = SYSTEM_TRAY_ORIENTATION_VERT;
   } else {
      orientation = SYSTEM_TRAY_ORIENTATION_HORZ;
   }
   JXChangeProperty(display, dock->cp->window, orientationAtom,
      XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&orientation, 1);

   /* Get the selection if we don't already own it.
    * If we did already own it, getting it again would cause problems
    * with some clients due to the way restarts are handled.
    */
   if(!owner) {

      owner = 1;
      JXSetSelectionOwner(display, dockAtom, dock->cp->window, CurrentTime);
      if(JXGetSelectionOwner(display, dockAtom) != dock->cp->window) {

         owner = 0;
         Warning("could not acquire system tray selection");

      } else {

         memset(&event, 0, sizeof(event));
         event.xclient.type = ClientMessage;
         event.xclient.window = rootWindow;
         event.xclient.message_type = JXInternAtom(display, "MANAGER", False);
         event.xclient.format = 32;
         event.xclient.data.l[0] = CurrentTime;
         event.xclient.data.l[1] = dockAtom;
         event.xclient.data.l[2] = dock->cp->window;
         event.xclient.data.l[3] = 0;
         event.xclient.data.l[4] = 0;

         JXSendEvent(display, rootWindow, False, StructureNotifyMask, &event);

      }

   }

}

/** Resize a dock component. */
void Resize(TrayComponentType *cp) {

   Assert(cp);

   JXResizeWindow(display, cp->window, cp->width, cp->height);
   UpdateDock();

}

/** Handle a dock event. */
void HandleDockEvent(const XClientMessageEvent *event) {

   Assert(event);

   switch(event->data.l[1]) {
   case SYSTEM_TRAY_REQUEST_DOCK:
      DockWindow(event->data.l[2]);
      break;
   case SYSTEM_TRAY_BEGIN_MESSAGE:
      break;
   case SYSTEM_TRAY_CANCEL_MESSAGE:
      break;
   default:
      Debug("invalid opcode in dock event");
      break;
   }

}

/** Handle a resize request event. */
int HandleDockResizeRequest(const XResizeRequestEvent *event) {

   DockNode *np;

   Assert(event);

   if(!dock) {
      return 0;
   }

   for(np = dock->nodes; np; np = np->next) {
      if(np->window == event->window) {

         JXResizeWindow(display, np->window, event->width, event->height);
         UpdateDock();

         return 1;
      }
   }

   return 0;
}

/** Handle a configure request event. */
int HandleDockConfigureRequest(const XConfigureRequestEvent *event) {

   XWindowChanges wc;
   DockNode *np;

   Assert(event);

   if(!dock) {
      return 0;
   }

   for(np = dock->nodes; np; np = np->next) {
      if(np->window == event->window) {
         wc.stack_mode = event->detail;
         wc.sibling = event->above;
         wc.border_width = event->border_width;
         wc.x = event->x;
         wc.y = event->y;
         wc.width = event->width;
         wc.height = event->height;
         JXConfigureWindow(display, np->window, event->value_mask, &wc);
         UpdateDock();
         return 1;
      }
   }

   return 0;

}

/** Handle a reparent notify event. */
int HandleDockReparentNotify(const XReparentEvent *event) {

   DockNode *np;
   int handled;

   Assert(event);

   /* Just return if there is no dock. */
   if(!dock) {
      return 0;
   }

   /* Check each docked window. */
   handled = 0;
   for(np = dock->nodes; np; np = np->next) {
      if(np->window == event->window) {
         if(event->parent != dock->cp->window) {
            /* For some reason the application reparented the window.
             * We make note of this condition and reparent every time
             * the dock is updated. Unfortunately we can't do this for
             * all applications because some won't deal with it.
             */
            np->needs_reparent = 1;
            handled = 1;
         }
      }
   }

   /* Layout the stuff on the dock again if something happened. */
   if(handled) {
      UpdateDock();
   }

   return handled;

}

/** Handle a destroy event. */
int HandleDockDestroy(Window win) {

   if(dock) {
      return UndockWindow(win);
   } else {
      return 0;
   }

}

/** Handle a selection clear event. */
int HandleDockSelectionClear(const XSelectionClearEvent *event) {

   if(event->selection == dockAtom) {
      Debug("lost _NET_SYSTEM_TRAY selection");
      owner = 0;
   }

   return 0;

}

/** Add a window to the dock. */
void DockWindow(Window win) {

   DockNode *np;

	/* If no dock is running, just return. */
	if(!dock) {
		return;
	}

   /* Make sure we have a valid window to add. */
   if(win == None) {
      return;
   }

   /* If this window is already docked ignore it. */
   for(np = dock->nodes; np; np = np->next) {
      if(np->window == win) {
         return;
      }
   }

   /* Add the window to our list. */
   np = Allocate(sizeof(DockNode));
   np->window = win;
   np->needs_reparent = 0;
   np->next = dock->nodes;
   dock->nodes = np;

   /* Update the requested size. */
   if(orientation == SYSTEM_TRAY_ORIENTATION_HORZ) {
      if(dock->cp->requestedWidth > 1) {
         dock->cp->requestedWidth += dock->cp->height;
      } else {
         dock->cp->requestedWidth = dock->cp->height;
      }
   } else {
      if(dock->cp->requestedHeight > 1) {
         dock->cp->requestedHeight += dock->cp->width;
      } else {
         dock->cp->requestedHeight = dock->cp->width;
      }
   }

   /* It's safe to reparent at (0, 0) since we call
    * ResizeTray which will invoke the Resize callback.
    */
   JXAddToSaveSet(display, win);
   JXSelectInput(display, win,
        StructureNotifyMask
      | ResizeRedirectMask
      | PointerMotionMask | PointerMotionHintMask);
   JXReparentWindow(display, win, dock->cp->window, 0, 0);
   JXMapRaised(display, win);

   /* Resize the tray containing the dock. */
   ResizeTray(dock->cp->tray);

}

/** Remove a window from the dock. */
int UndockWindow(Window win) {

   DockNode *np;
   DockNode *last;

	/* If no dock is running, just return. */
	if(!dock) {
		return 0;
	}

   last = NULL;
   for(np = dock->nodes; np; np = np->next) {
      if(np->window == win) {

         /* Remove the window from our list. */
         if(last) {
            last->next = np->next;
         } else {
            dock->nodes = np->next;
         }
         Release(np);

         /* Update the requested size. */
         if(orientation == SYSTEM_TRAY_ORIENTATION_HORZ) {
            dock->cp->requestedWidth -= dock->cp->height;
            if(dock->cp->requestedWidth <= 0) {
               dock->cp->requestedWidth = 1;
            }
         } else {
            dock->cp->requestedHeight -= dock->cp->width;
            if(dock->cp->requestedHeight <= 0) {
               dock->cp->requestedHeight = 1;
            }
         }

         /* Resize the tray. */
         ResizeTray(dock->cp->tray);

         return 1;

      }
      last = np;
   }

   return 0;
}

/** Layout items on the dock. */
void UpdateDock() {

   XWindowAttributes attr;
   DockNode *np;
   int x, y;
   int width, height;
   int xoffset, yoffset;
   int itemWidth, itemHeight;
   double ratio;

   Assert(dock);

   /* Determine the size of items in the dock. */
   if(orientation == SYSTEM_TRAY_ORIENTATION_HORZ) {
      itemWidth = dock->cp->height;
      itemHeight = dock->cp->height;
   } else {
      itemHeight = dock->cp->width;
      itemWidth = dock->cp->width;
   }

   x = 0;
   y = 0;
   for(np = dock->nodes; np; np = np->next) {

      xoffset = 0;
      yoffset = 0;
      width = itemWidth;
      height = itemHeight;

      if(JXGetWindowAttributes(display, np->window, &attr)) {

         ratio = (double)attr.width / attr.height;

         if(ratio > 1.0) {
            if(width > attr.width) {
               width = attr.width;
            }
            height = width / ratio;
         } else {
            if(height > attr.height) {
               height = attr.height;
            }
            width = height * ratio;
         }

         xoffset = (itemWidth - width) / 2;
         yoffset = (itemHeight - height) / 2;

      }

      JXMoveResizeWindow(display, np->window, x + xoffset, y + yoffset,
         width, height);

      /* Reparent if this window likes to go other places. */
      if(np->needs_reparent) {
         JXReparentWindow(display, np->window, dock->cp->window,
         x + xoffset, y + yoffset);
      }

      if(orientation == SYSTEM_TRAY_ORIENTATION_HORZ) {
         x += itemWidth;
      } else {
         y += itemHeight;
      }
   }

}

