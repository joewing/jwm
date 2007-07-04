/**
 * @file swallow.c
 * @author Joe Wingbermuehle
 * @date 2005-2006
 *
 * @brief Swallow tray component.
 *
 */

#include "jwm.h"
#include "swallow.h"
#include "main.h"
#include "tray.h"
#include "error.h"
#include "command.h"
#include "color.h"
#include "client.h"
#include "event.h"
#include "misc.h"

typedef struct SwallowNode {

   TrayComponentType *cp;

   char *name;
   char *command;
   int border;
   int userWidth;
   int userHeight;

   struct SwallowNode *next;

} SwallowNode;

static SwallowNode *swallowNodes;

static void Destroy(TrayComponentType *cp);
static void Resize(TrayComponentType *cp);

/** Initialize swallow data. */
void InitializeSwallow() {
   swallowNodes = NULL;
}

/** Start swallow processing. */
void StartupSwallow() {

   SwallowNode *np;

   for(np = swallowNodes; np; np = np->next) {
      if(np->command) {
         RunCommand(np->command);
      }
   }

}

/** Stop swallow processing. */
void ShutdownSwallow() {
}

/** Destroy swallow data. */
void DestroySwallow() {

   SwallowNode *np;

   while(swallowNodes) {

      np = swallowNodes->next;

      Assert(swallowNodes->name);
      Release(swallowNodes->name);

      if(swallowNodes->command) {
         Release(swallowNodes->command);
      }

      Release(swallowNodes);
      swallowNodes = np;

   }

}

/** Create a swallowed application tray component. */
TrayComponentType *CreateSwallow(const char *name, const char *command,
   int width, int height) {

   TrayComponentType *cp;
   SwallowNode *np;

   if(!name) {
      Warning("cannot swallow a client with no name");
      return NULL;
   }

   /* Make sure this name isn't already used. */
   for(np = swallowNodes; np; np = np->next) {
      if(!strcmp(np->name, name)) {
         Warning("cannot swallow the same client multiple times");
         return NULL;
      }
   }

   np = Allocate(sizeof(SwallowNode));
   np->name = CopyString(name);
   np->command = CopyString(command);

   np->next = swallowNodes;
   swallowNodes = np;

   cp = CreateTrayComponent();
   np->cp = cp;
   cp->object = np;
   cp->Destroy = Destroy;
   cp->Resize = Resize;

   if(width) {
      cp->requestedWidth = width;
      np->userWidth = 1;
   } else {
      cp->requestedWidth = 1;
      np->userWidth = 0;
   }
   if(height) {
      cp->requestedHeight = height;
      np->userHeight = 1;
   } else {
      cp->requestedHeight = 1;
      np->userHeight = 0;
   }

   return cp;

}

/** Process an event on a swallowed window. */
int ProcessSwallowEvent(const XEvent *event) {

   SwallowNode *np;
   int width, height;

   for(np = swallowNodes; np; np = np->next) {
      if(event->xany.window == np->cp->window) {
         switch(event->type) {
         case DestroyNotify:
            np->cp->window = None;
            np->cp->requestedWidth = 1;
            np->cp->requestedHeight = 1;
            ResizeTray(np->cp->tray);
            break;
         case ResizeRequest:
            np->cp->requestedWidth
               = event->xresizerequest.width + np->border * 2;
            np->cp->requestedHeight
               = event->xresizerequest.height + np->border * 2;
            ResizeTray(np->cp->tray);
            break;
         case ConfigureNotify:
            /* I don't think this should be necessary, but somehow
             * resize requests slip by sometimes... */
            width = event->xconfigure.width + np->border * 2;
            height = event->xconfigure.height + np->border * 2;
            if(   width != np->cp->requestedWidth
               && height != np->cp->requestedHeight) {
               np->cp->requestedWidth = width;
               np->cp->requestedHeight = height;
               ResizeTray(np->cp->tray);
            }
            break;
         default:
            break;
         }
         return 1;
      }
   }

   return 0;

}

/** Handle a tray resize. */
void Resize(TrayComponentType *cp) {

   int width, height;

   SwallowNode *np = (SwallowNode*)cp->object;

   if(cp->window != None) {

      width = cp->width - np->border * 2;
      height = cp->height - np->border * 2;

      JXResizeWindow(display, cp->window, width, height);

   }

}

/** Destroy a swallow tray component. */
void Destroy(TrayComponentType *cp) {

   ClientProtocolType protocols;

   /* Destroy the window if there is one. */
   if(cp->window) {

      JXReparentWindow(display, cp->window, rootWindow, 0, 0);
      JXRemoveFromSaveSet(display, cp->window);

      protocols = ReadWMProtocols(cp->window);
      if(protocols & PROT_DELETE) {
         SendClientMessage(cp->window, ATOM_WM_PROTOCOLS,
            ATOM_WM_DELETE_WINDOW);
      } else {
         JXKillClient(display, cp->window);
      }

   }

}

/** Determine if this is a window to be swallowed, if it is, swallow it. */
int CheckSwallowMap(const XMapEvent *event) {

   SwallowNode *np;
   XClassHint hint;
   XWindowAttributes attr;

   for(np = swallowNodes; np; np = np->next) {

      if(np->cp->window != None) {
         continue;
      }

      Assert(np->cp->tray->window != None);

      if(JXGetClassHint(display, event->window, &hint)) {
         if(!strcmp(hint.res_name, np->name)) {

            /* Swallow the window. */
            JXSelectInput(display, event->window,
                 StructureNotifyMask | ResizeRedirectMask);
            JXAddToSaveSet(display, event->window);
            JXSetWindowBorder(display, event->window, colors[COLOR_TRAY_BG]);
            JXReparentWindow(display, event->window,
               np->cp->tray->window, 0, 0);
            JXMapRaised(display, event->window);
            JXFree(hint.res_name);
            JXFree(hint.res_class);
            np->cp->window = event->window;

            /* Update the size. */
            JXGetWindowAttributes(display, event->window, &attr);
            np->border = attr.border_width;
            if(!np->userWidth) {
               np->cp->requestedWidth = attr.width + 2 * np->border;
            }
            if(!np->userHeight) {
               np->cp->requestedHeight = attr.height + 2 * np->border;
            }

            ResizeTray(np->cp->tray);

            return 1;

         } else {

            JXFree(hint.res_name);
            JXFree(hint.res_class);

         }
      }

   }

   return 0;

}

