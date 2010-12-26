/**
 * @file client.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions to handle client windows.
 *
 */

#include "jwm.h"
#include "client.h"
#include "clientlist.h"
#include "main.h"
#include "icon.h"
#include "hint.h"
#include "group.h"
#include "tray.h"
#include "confirm.h"
#include "key.h"
#include "cursor.h"
#include "taskbar.h"
#include "screen.h"
#include "pager.h"
#include "color.h"
#include "error.h"
#include "place.h"

static const int STACK_BLOCK_SIZE = 8;

static ClientNode *activeClient;

static int clientCount;

static void LoadFocus();
static void ReparentClient(ClientNode *np, int notOwner);
 
static void MinimizeTransients(ClientNode *np);
static void CheckShape(ClientNode *np);

static void RestoreTransients(ClientNode *np, int raise);

static void KillClientHandler(ClientNode *np);

static unsigned int activeOpacity;
static unsigned int maxInactiveOpacity;
static unsigned int minInactiveOpacity;
static unsigned int deltaInactiveOpacity;

/** Initialize client data. */
void InitializeClients() {
   activeOpacity = (unsigned int)(1.0 * UINT_MAX);
   maxInactiveOpacity = (unsigned int)(0.9 * UINT_MAX);
   minInactiveOpacity = (unsigned int)(0.5 * UINT_MAX);
   deltaInactiveOpacity = (unsigned int)(0.1 * UINT_MAX);
}

/** Load windows that are already mapped. */
void StartupClients() {

   XWindowAttributes attr;
   Window rootReturn, parentReturn, *childrenReturn;
   unsigned int childrenCount;
   unsigned int x;

   clientCount = 0;
   activeClient = NULL;
   currentDesktop = 0;

   /* Clear out the client lists. */
   for(x = 0; x < LAYER_COUNT; x++) {
      nodes[x] = NULL;
      nodeTail[x] = NULL;
   }

   /* Query client windows. */
   JXQueryTree(display, rootWindow, &rootReturn, &parentReturn,
      &childrenReturn, &childrenCount);

   /* Add each client. */
   for(x = 0; x < childrenCount; x++) {
      if(JXGetWindowAttributes(display, childrenReturn[x], &attr)) {
         if(attr.override_redirect == False
            && attr.map_state == IsViewable) {
            AddClientWindow(childrenReturn[x], 1, 1);
         }
      }
   }

   JXFree(childrenReturn);

   LoadFocus();

   UpdateTaskBar();
   UpdatePager();

}

/** Release client windows. */
void ShutdownClients() {

   int x;

   for(x = 0; x < LAYER_COUNT; x++) {
      while(nodeTail[x]) {
         RemoveClient(nodeTail[x]);
      }
   }

}

/** Destroy client data. */
void DestroyClients() {
}

/** Set the focus to the window currently under the mouse pointer. */
void LoadFocus() {

   ClientNode *np;
   Window rootReturn, childReturn;
   int rootx, rooty;
   int winx, winy;
   unsigned int mask;

   JXQueryPointer(display, rootWindow, &rootReturn, &childReturn,
      &rootx, &rooty, &winx, &winy, &mask);

   np = FindClientByWindow(childReturn);
   if(np) {
      FocusClient(np);
   }

}

/** Add a window to management. */
ClientNode *AddClientWindow(Window w, int alreadyMapped, int notOwner) {

   XWindowAttributes attr;
   ClientNode *np;

   Assert(w != None);

   /* Get window attributes. */
   if(JXGetWindowAttributes(display, w, &attr) == 0) {
      return NULL;
   }

   /* Determine if we should care about this window. */
   if(attr.override_redirect == True) {
      return NULL;
   }
   if(attr.class == InputOnly) {
      return NULL;
   }

   /* Prepare a client node for this window. */
   np = Allocate(sizeof(ClientNode));
   memset(np, 0, sizeof(ClientNode));

   np->window = w;
   np->owner = None;
   np->state.desktop = currentDesktop;
   np->controller = NULL;
   np->name = NULL;
   np->colormaps = NULL;

   np->x = attr.x;
   np->y = attr.y;
   np->width = attr.width;
   np->height = attr.height;
   np->cmap = attr.colormap;
   np->colormaps = NULL;
   np->state.status = STAT_NONE;
   np->state.layer = LAYER_NORMAL;

   np->state.border = BORDER_DEFAULT;
   np->borderAction = BA_NONE;

   ReadClientProtocols(np);

   if(!notOwner) {
      np->state.border = BORDER_OUTLINE | BORDER_TITLE | BORDER_MOVE;
      np->state.status |= STAT_WMDIALOG | STAT_STICKY;
   }

   /* We now know the layer, so insert */
   np->prev = NULL;
   np->next = nodes[np->state.layer];
   if(np->next) {
      np->next->prev = np;
   } else {
      nodeTail[np->state.layer] = np;
   }
   nodes[np->state.layer] = np;

   LoadIcon(np);

   ApplyGroups(np);

   SetDefaultCursor(np->window);
   ReparentClient(np, notOwner);
   PlaceClient(np, alreadyMapped);

   /* If one of these fails we are SOL, so who cares. */
   XSaveContext(display, np->window, clientContext, (void*)np);
   XSaveContext(display, np->parent, frameContext, (void*)np);

   if(np->state.status & STAT_MAPPED) {
      JXMapWindow(display, np->window);
      JXMapWindow(display, np->parent);
   }

   DrawBorder(np, NULL);

   AddClientToTaskBar(np);

   if(!alreadyMapped) {
      RaiseClient(np);
   }

   ++clientCount;

   if(np->state.status & STAT_STICKY) {
      SetCardinalAtom(np->window, ATOM_NET_WM_DESKTOP, ~0UL);
   } else {
      SetCardinalAtom(np->window, ATOM_NET_WM_DESKTOP, np->state.desktop);
   }

   /* Shade the client if requested. */
   if(np->state.status & STAT_SHADED) {
      ShadeClient(np);
   }

   /* Minimize the client if requested. */
   if(np->state.status & STAT_MINIMIZED) {
      np->state.status &= ~STAT_MINIMIZED;
      MinimizeClient(np);
   }

   /* Maximize the client if requested. */
   if(np->state.status & (STAT_HMAX | STAT_VMAX)) {
      np->state.status &= ~(STAT_HMAX | STAT_VMAX);
      MaximizeClientDefault(np);
   }

   /* Make sure we're still in sync */
   WriteState(np);
   SendConfigureEvent(np);

   /* Hide the client if we're not on the right desktop. */
   if(np->state.desktop != currentDesktop
      && !(np->state.status & STAT_STICKY)) {
      HideClient(np);
   }

   ReadClientStrut(np);

   /* Focus transients if their parent has focus. */
   if(np->owner != None) {
      if(activeClient && np->owner == activeClient->window) {
         FocusClient(np);
      }
   }

   /* Make the client fullscreen if requested. */
   if(np->state.status & STAT_FULLSCREEN) {
      np->state.status &= ~STAT_FULLSCREEN;
      SetClientFullScreen(np, 1);
   }

   return np;

}

/** Minimize a client window and all of its transients. */
void MinimizeClient(ClientNode *np) {

   Assert(np);

   if(focusModel == FOCUS_CLICK && np == activeClient) {
      FocusNextStacked(np);
   }

   MinimizeTransients(np);

   UpdateTaskBar();
   UpdatePager();

}

/** Minimize all transients as well as the specified client. */
void MinimizeTransients(ClientNode *np) {

   ClientNode *tp;
   int x;

   Assert(np);

   /* A minimized client can't be active. */
   if(activeClient == np) {
      activeClient = NULL;
      np->state.status &= ~STAT_ACTIVE;
   }

   /* Unmap the window and update its state. */
   if(np->state.status & (STAT_MAPPED | STAT_SHADED)) {
      JXUnmapWindow(display, np->window);
      JXUnmapWindow(display, np->parent);
   }
   np->state.status |= STAT_MINIMIZED;
   np->state.status &= ~STAT_MAPPED;
   WriteState(np);

   /* Minimize transient windows. */
   for(x = 0; x < LAYER_COUNT; x++) {
      for(tp = nodes[x]; tp; tp = tp->next) {
         if(tp->owner == np->window
            && (tp->state.status & (STAT_MAPPED | STAT_SHADED))
            && !(tp->state.status & STAT_MINIMIZED)) {
            MinimizeTransients(tp);
         }
      }
   }

}

/** Shade a client. */
void ShadeClient(ClientNode *np) {

   int north, south, east, west;

   Assert(np);

   if(!(np->state.border & BORDER_TITLE)) {
      return;
   }

   GetBorderSize(np, &north, &south, &east, &west);

   ResetRoundedRectWindow(np->parent);
   if(np->state.status & STAT_MAPPED) {
      JXUnmapWindow(display, np->window);
   }
   np->state.status |= STAT_SHADED;
   np->state.status &= ~STAT_MINIMIZED;
   np->state.status &= ~STAT_SDESKTOP;
   np->state.status &= ~STAT_MAPPED;

   ShapeRoundedRectWindow(np->parent, np->width + west + east, north);
   JXResizeWindow(display, np->parent, np->width + east + west, north);

   WriteState(np);

#ifdef USE_SHAPE
   if(np->state.status & STAT_SHAPE) {
      SetShape(np);
   }
#endif

}

/** Unshade a client. */
void UnshadeClient(ClientNode *np) {

   int north, south, east, west;

   Assert(np);

   if(!(np->state.border & BORDER_TITLE)) {
      return;
   }

   if(np->state.status & STAT_SHADED) {
      JXMapWindow(display, np->window);
      np->state.status |= STAT_MAPPED;
      np->state.status &= ~STAT_SHADED;
   }

   GetBorderSize(np, &north, &south, &east, &west);

   ResetRoundedRectWindow(np->parent);
   ShapeRoundedRectWindow(np->parent, 
      np->width + west + east, np->height + north + south);

   JXResizeWindow(display, np->parent,
      np->width + west + east, np->height + north + south);

   WriteState(np);

#ifdef USE_SHAPE
   if(np->state.status & STAT_SHAPE) {
      SetShape(np);
   }
#endif

   RefocusClient();
   RestackClients();

}

/** Set a client's state to withdrawn. */
void SetClientWithdrawn(ClientNode *np) {

   Assert(np);

   if(activeClient == np) {
      activeClient = NULL;
      np->state.status &= ~STAT_ACTIVE;
      FocusNextStacked(np);
   }

   if(np->state.status & STAT_MAPPED) {
      JXUnmapWindow(display, np->window);
      JXUnmapWindow(display, np->parent);
   } else if(np->state.status & STAT_SHADED) {
      JXUnmapWindow(display, np->parent);
   }

   np->state.status &= ~STAT_SHADED;
   np->state.status &= ~STAT_MAPPED;
   np->state.status &= ~STAT_MINIMIZED;
   np->state.status &= ~STAT_SDESKTOP;

   WriteState(np);
   UpdateTaskBar();
   UpdatePager();

}

/** Restore a window with its transients (helper method). */
void RestoreTransients(ClientNode *np, int raise) {

   ClientNode *tp;
   int x;

   Assert(np);

   /* Restore this window. */
   if(!(np->state.status & STAT_MAPPED)) {
      if(np->state.status & STAT_SHADED) {
         JXMapWindow(display, np->parent);
      } else {
         JXMapWindow(display, np->window);
         JXMapWindow(display, np->parent);
         np->state.status |= STAT_MAPPED;
      }
   }
   np->state.status &= ~STAT_MINIMIZED;
   np->state.status &= ~STAT_SDESKTOP;

   WriteState(np);

   /* Restore transient windows. */
   for(x = 0; x < LAYER_COUNT; x++) {
      for(tp = nodes[x]; tp; tp = tp->next) {
         if(tp->owner == np->window
            && !(tp->state.status & (STAT_MAPPED | STAT_SHADED))
            && (tp->state.status & STAT_MINIMIZED)) {
            RestoreTransients(tp, raise);
         }
      }
   }

   if(raise) {
      RaiseClient(np);
   }

}

/** Restore a client window and its transients. */
void RestoreClient(ClientNode *np, int raise) {

   Assert(np);

   RestoreTransients(np, raise);

   RestackClients();
   UpdateTaskBar();
   UpdatePager();

}

/** Set the client layer. This will affect transients. */
void SetClientLayer(ClientNode *np, unsigned int layer) {

   ClientNode *tp, *next;
   int x;

   Assert(np);

   if(layer > LAYER_TOP) {
      Warning("Client %s requested an invalid layer: %d", np->name, layer);
      return;
   }

   if(np->state.layer != layer) {

      /* Loop through all clients so we get transients. */
      for(x = 0; x < LAYER_COUNT; x++) {
         tp = nodes[x];
         while(tp) {
            if(tp == np || tp->owner == np->window) {

               next = tp->next;

               /* Remove from the old node list */
               if(next) {
                  next->prev = tp->prev;
               } else {
                  nodeTail[tp->state.layer] = tp->prev;
               }
               if(tp->prev) {
                  tp->prev->next = next;
               } else {
                  nodes[tp->state.layer] = next;
               }

               /* Insert into the new node list */
               tp->prev = NULL;
               tp->next = nodes[layer];
               if(nodes[layer]) {
                  nodes[layer]->prev = tp;
               } else {
                  nodeTail[layer] = tp;
               }
               nodes[layer] = tp;

               /* Set the new layer */
               tp->state.layer = layer;
               SetCardinalAtom(tp->window, ATOM_WIN_LAYER, layer);

               /* Make sure we continue on the correct layer list. */
               tp = next;

            } else {
               tp = tp->next;
            }
         }
      }

      RestackClients();

   }

}

/** Set a client's sticky status. This will update transients. */
void SetClientSticky(ClientNode *np, int isSticky) {

   ClientNode *tp;
   int old;
   int x;

   Assert(np);

   /* Get the old sticky status. */
   if(np->state.status & STAT_STICKY) {
      old = 1;
   } else {
      old = 0;
   }

   if(isSticky && !old) {

      /* Change from non-sticky to sticky. */

      for(x = 0; x < LAYER_COUNT; x++) {
         for(tp = nodes[x]; tp; tp = tp->next) {
            if(tp == np || tp->owner == np->window) {
               tp->state.status |= STAT_STICKY;
               SetCardinalAtom(tp->window, ATOM_NET_WM_DESKTOP, ~0UL);
               WriteState(tp);
            }
         }
      }

   } else if(!isSticky && old) {

      /* Change from sticky to non-sticky. */

      for(x = 0; x < LAYER_COUNT; x++) {
         for(tp = nodes[x]; tp; tp = tp->next) {
            if(tp == np || tp->owner == np->window) {
               tp->state.status &= ~STAT_STICKY;
               WriteState(tp);
            }
         }
      }

      /* Since this client is no longer sticky, we need to assign
       * a desktop. Here we use the current desktop.
       * Note that SetClientDesktop updates transients (which is good).
       */
      SetClientDesktop(np, currentDesktop);

   }

}

/** Set a client's desktop. This will update transients. */
void SetClientDesktop(ClientNode *np, unsigned int desktop) {

   ClientNode *tp;
   int x;

   Assert(np);

   if(desktop >= desktopWidth * desktopHeight) {
      return;
   }

   if(!(np->state.status & STAT_STICKY)) {
      for(x = 0; x < LAYER_COUNT; x++) {
         for(tp = nodes[x]; tp; tp = tp->next) {
            if(tp == np || tp->owner == np->window) {

               tp->state.desktop = desktop;

               if(desktop == currentDesktop) {
                  ShowClient(tp);
               } else {
                  HideClient(tp);
               }

               SetCardinalAtom(tp->window, ATOM_NET_WM_DESKTOP,
                  tp->state.desktop);
            }
         }
      }
      UpdatePager();
      UpdateTaskBar();
   }

}

/** Hide a client without unmapping. This will not update transients. */
void HideClient(ClientNode *np) {

   Assert(np);

   if(activeClient == np) {
      activeClient = NULL;
   }
   np->state.status |= STAT_HIDDEN;
   if(np->state.status & (STAT_MAPPED | STAT_SHADED)) {
      JXUnmapWindow(display, np->parent);
   }

}

/** Show a hidden client. This will not update transients. */
void ShowClient(ClientNode *np) {

   Assert(np);

   if(np->state.status & STAT_HIDDEN) {
      np->state.status &= ~STAT_HIDDEN;
      if(np->state.status & (STAT_MAPPED | STAT_SHADED)) {
         JXMapWindow(display, np->parent);
         if(np->state.status & STAT_ACTIVE) {
            FocusClient(np);
         }
      }
   }

}

/** Maximize a client window. */
void MaximizeClient(ClientNode *np, int horiz, int vert) {

   int north, south, east, west;

   Assert(np);

   /* We don't want to mess with full screen clients. */
   if(np->state.status & STAT_FULLSCREEN) {
      return; 
   }

   if(np->state.status & STAT_SHADED) {
      UnshadeClient(np);
   }

   ResetRoundedRectWindow(np->parent);

   GetBorderSize(np, &north, &south, &east, &west);

   if(np->state.status & (STAT_HMAX | STAT_VMAX)) {
      np->x = np->oldx;
      np->y = np->oldy;
      np->width = np->oldWidth;
      np->height = np->oldHeight;
      np->state.status &= ~(STAT_HMAX | STAT_VMAX);
   } else {
      PlaceMaximizedClient(np, horiz, vert);
   }

   ShapeRoundedRectWindow(np->parent, 
      np->width + east + west,
      np->height + north + south);

   JXMoveResizeWindow(display, np->parent,
      np->x - west, np->y - north,
      np->width + east + west,
      np->height + north + south);
   JXMoveResizeWindow(display, np->window, west,
      north, np->width, np->height);

   WriteState(np);
   SendConfigureEvent(np);

}

/** Maximize a client using its default maximize settings. */
void MaximizeClientDefault(ClientNode *np) {

   int hmax, vmax;

   Assert(np);

   hmax = (np->state.border & BORDER_MAX_H) ? 1 : 0;
   vmax = (np->state.border & BORDER_MAX_V) ? 1 : 0;

   MaximizeClient(np, hmax, vmax);

}

/** Set a client's full screen state. */
void SetClientFullScreen(ClientNode *np, int fullScreen) {

   XEvent event;
   int north, south, east, west;
   const ScreenType *sp;

   Assert(np);

   /* Make sure there's something to do. */
   if(fullScreen && (np->state.status & STAT_FULLSCREEN)) {
      return;
   } else if (!fullScreen && !(np->state.status & STAT_FULLSCREEN)) {
      return;
   }

   if(np->state.status & STAT_SHADED) {
      UnshadeClient(np);
   }

   ResetRoundedRectWindow(np->parent);

   if(fullScreen) {

      np->state.status |= STAT_FULLSCREEN;

      sp = GetCurrentScreen(np->x, np->y);

      JXReparentWindow(display, np->window, rootWindow, 0, 0);
      JXMoveResizeWindow(display, np->window, 0, 0, sp->width, sp->height);
      SetClientLayer(np, LAYER_TOP);

   } else {

      np->state.status &= ~STAT_FULLSCREEN;

      /* Reparent window */
      GetBorderSize(np, &north, &south, &east, &west);
      JXReparentWindow(display, np->window, np->parent, west, north);
      JXMoveResizeWindow(display, np->window, west,
         north, np->width, np->height);

      /* Restore parent position */
      GetBorderSize(np, &north, &south, &east, &west);
      ShapeRoundedRectWindow(np->parent, np->width + east + west,
         np->height + north + south);
      JXMoveResizeWindow(display, np->parent, np->oldx - west,
         np->oldy - north, np->width + east + west, np->height + north + south);

      event.type = MapRequest;
      event.xmaprequest.send_event = True;
      event.xmaprequest.display = display;
      event.xmaprequest.parent = np->parent;
      event.xmaprequest.window = np->window;
      JXSendEvent(display, rootWindow, False,
         SubstructureRedirectMask, &event);

      SetClientLayer(np, LAYER_NORMAL);

   }

   WriteState(np);
   SendConfigureEvent(np);

}

/** Set the active client. */
void FocusClient(ClientNode *np) {

   Assert(np);

   if(np->state.status & STAT_HIDDEN) {
      return;
   }

   if(activeClient != np || !(np->state.status & STAT_ACTIVE)) {

      if(activeClient) {
         activeClient->state.status &= ~STAT_ACTIVE;
         DrawBorder(activeClient, NULL);
      }
      np->state.status |= STAT_ACTIVE;
      activeClient = np;

      if(!(np->state.status & STAT_SHADED)) {
         UpdateClientColormap(np);
         SetWindowAtom(rootWindow, ATOM_NET_ACTIVE_WINDOW, np->window);
      }

      DrawBorder(np, NULL);
      UpdatePager();
      UpdateTaskBar();

   }

   if(np->state.status & STAT_MAPPED) {
      JXSetInputFocus(display, np->window, RevertToPointerRoot, CurrentTime);
   } else {
      JXSetInputFocus(display, rootWindow, RevertToPointerRoot, CurrentTime);
   }

}


/** Refocus the active client (if there is one). */
void RefocusClient() {

   if(activeClient) {
      FocusClient(activeClient);
   }

}

/** Send a delete message to a client. */
void DeleteClient(ClientNode *np) {

   ClientProtocolType protocols;

   Assert(np);

   protocols = ReadWMProtocols(np->window);
   if(protocols & PROT_DELETE) {
      SendClientMessage(np->window, ATOM_WM_PROTOCOLS,
         ATOM_WM_DELETE_WINDOW);
   } else {
      KillClient(np);
   }

}

/** Callback to kill a client after a confirm dialog. */
void KillClientHandler(ClientNode *np) {

   Assert(np);

   if(np == activeClient) {
      FocusNextStacked(np);
   }

   JXGrabServer(display);
   JXSync(display, False);

   JXKillClient(display, np->window);

   JXSync(display, True);
   JXUngrabServer(display);

   RemoveClient(np);

}

/** Kill a client window. */
void KillClient(ClientNode *np) {

   Assert(np);

   ShowConfirmDialog(np, KillClientHandler,
      "Kill this window?",
      "This may cause data to be lost!",
      NULL);
}

/** Raise the client. This will affect transients. */
void RaiseClient(ClientNode *np) {

   ClientNode *tp, *next;
   int x;

   Assert(np);

   if(nodes[np->state.layer] != np) {

      /* Raise the window */
      Assert(np->prev);
      np->prev->next = np->next;
      if(np->next) {
         np->next->prev = np->prev;
      } else {
         nodeTail[np->state.layer] = np->prev;
      }
      np->next = nodes[np->state.layer];
      nodes[np->state.layer]->prev = np;
      np->prev = NULL;
      nodes[np->state.layer] = np;

      /* Place any transient windows on top of the owner */
      for(x = 0; x < LAYER_COUNT; x++) {
         for(tp = nodes[x]; tp; tp = tp->next) {
            if(tp->owner == np->window && tp->prev) {

               next = tp->next;

               tp->prev->next = tp->next;
               if(tp->next) {
                  tp->next->prev = tp->prev;
               } else {
                  nodeTail[tp->state.layer] = tp->prev;
               }
               tp->next = nodes[tp->state.layer];
               nodes[tp->state.layer]->prev = tp;
               tp->prev = NULL;
               nodes[tp->state.layer] = tp;

               tp = next;

            }

            /* tp will be tp->next if the above code is executed. */
            /* Thus, if it is NULL, we are done with this layer. */
            if(!tp) {
               break;
            }
         }
      }

      RestackClients();

   }

}

/** Lower the client. This will not affect transients. */
void LowerClient(ClientNode *np) {

   ClientNode *tp;

   Assert(np);

   if(nodeTail[np->state.layer] != np) {

      Assert(np->next);

      /* Take the client out of the list. */
      if(np->prev) {
         np->prev->next = np->next;
      } else {
         nodes[np->state.layer] = np->next;
      }
      np->next->prev = np->prev;

      /* Place the client at the end of the list. */
      tp = nodeTail[np->state.layer];
      nodeTail[np->state.layer] = np;
      tp->next = np;
      np->prev = tp;
      np->next = NULL;

      RestackClients();

   }

}

/** Restack the clients according the way we want them. */
void RestackClients() {

   TrayType *tp;
   ClientNode *np;
   unsigned int layer, index;
   int trayCount;
   Window *stack;
   unsigned int opacity;
   unsigned int temp;
   int isFirst;

   /** Allocate memory for restacking. */
   trayCount = GetTrayCount();
   stack = AllocateStack((clientCount + trayCount) * sizeof(Window));

   /* Prepare the stacking array. */
   index = 0;
   layer = LAYER_TOP;
   isFirst = 1;
   opacity = maxInactiveOpacity;
   for(;;) {

      for(np = nodes[layer]; np; np = np->next) {
         if((np->state.status & (STAT_MAPPED | STAT_SHADED))
            && !(np->state.status & STAT_HIDDEN)) {
            stack[index++] = np->parent;
            if(isFirst) {
               if(   !(np->state.status & STAT_OPACITY)
                  && np->state.opacity != activeOpacity) {
                  np->state.opacity = activeOpacity;
                  WriteState(np);
               }
               isFirst = 0;
            } else if(!(np->state.status & STAT_OPACITY)) {
               if(np->state.opacity != opacity) {
                  np->state.opacity = opacity;
                  WriteState(np);
               }
               temp = opacity - deltaInactiveOpacity;
               if(temp < minInactiveOpacity || temp > opacity) {
                  opacity = minInactiveOpacity;
               } else {
                  opacity = temp;
               }
            }
         }
      }

      for(tp = GetTrays(); tp; tp = tp->next) {
         if(layer == tp->layer) {
            stack[index++] = tp->window;
         }
      }

      if(layer == 0) {
         break;
      }
      --layer;

   }

   JXRestackWindows(display, stack, index);

   ReleaseStack(stack);

   UpdateNetClientList();

}

/** Send a client message to a window. */
void SendClientMessage(Window w, AtomType type, AtomType message) {

   XEvent event;
   int status;

   memset(&event, 0, sizeof(event));
   event.xclient.type = ClientMessage;
   event.xclient.window = w;
   event.xclient.message_type = atoms[type];
   event.xclient.format = 32;
   event.xclient.data.l[0] = atoms[message];
   event.xclient.data.l[1] = CurrentTime;

   status = JXSendEvent(display, w, False, 0, &event);
   if(status == False) {
      Debug("SendClientMessage failed");
   }

}

/** Set the border shape for windows using the shape extension. */
#ifdef USE_SHAPE
void SetShape(ClientNode *np) {

   XRectangle rect[4];
   int north, south, east, west;

   Assert(np);

   np->state.status |= STAT_SHAPE;

   GetBorderSize(np, &north, &south, &east, &west);

   /* Shaded windows are a special case. */
   if(np->state.status & STAT_SHADED) {

      rect[0].x = 0;
      rect[0].y = 0;
      rect[0].width = np->width + east + west;
      rect[0].height = north + south;

      JXShapeCombineRectangles(display, np->parent, ShapeBounding,
         0, 0, rect, 1, ShapeSet, Unsorted);

      return;
   }

   /* Add the shape of window. */
   JXShapeCombineShape(display, np->parent, ShapeBounding, west, north,
      np->window, ShapeBounding, ShapeSet);

   /* Add the shape of the border. */
   if(north > 0) {

      /* Top */
      rect[0].x = 0;
      rect[0].y = 0;
      rect[0].width = np->width + east + west;
      rect[0].height = north;

      /* Left */
      rect[1].x = 0;
      rect[1].y = 0;
      rect[1].width = west;
      rect[1].height = np->height + north + south;

      /* Right */
      rect[2].x = np->width + east;
      rect[2].y = 0;
      rect[2].width = west;
      rect[2].height = np->height + north + south;

      /* Bottom */
      rect[3].x = 0;
      rect[3].y = np->height + north;
      rect[3].width = np->width + east + west;
      rect[3].height = south;

      JXShapeCombineRectangles(display, np->parent, ShapeBounding,
         0, 0, rect, 4, ShapeUnion, Unsorted);

   }

}
#endif /* USE_SHAPE */

/** Remove a client window from management. */
void RemoveClient(ClientNode *np) {

   ColormapNode *cp;

   Assert(np);
   Assert(np->window != None);
   Assert(np->parent != None);

   JXGrabServer(display);

   /* Remove this client from the client list */
   if(np->next) {
      np->next->prev = np->prev;
   } else {
      nodeTail[np->state.layer] = np->prev;
   }
   if(np->prev) {
      np->prev->next = np->next;
   } else {
      nodes[np->state.layer] = np->next;
   }
   --clientCount;
   XDeleteContext(display, np->window, clientContext);
   XDeleteContext(display, np->parent, frameContext);

   /* Make sure this client isn't active */
   if(activeClient == np && !shouldExit) {
      FocusNextStacked(np);
   }
   if(activeClient == np) {
      SetWindowAtom(rootWindow, ATOM_NET_ACTIVE_WINDOW, None);
      activeClient = NULL;
   }

   /* If the window manager is exiting (ie, not the client), then
    * reparent etc. */
   if(shouldExit && !(np->state.status & STAT_WMDIALOG)) {
      if(np->state.status & (STAT_VMAX | STAT_HMAX)) {
         np->x = np->oldx;
         np->y = np->oldy;
         np->width = np->oldWidth;
         np->height = np->oldHeight;
         JXMoveResizeWindow(display, np->window,
            np->x, np->y, np->width, np->height);
      }
      GravitateClient(np, 1);
      if(!(np->state.status & STAT_MAPPED)
         && (np->state.status & (STAT_MINIMIZED | STAT_SHADED))) {
         JXMapWindow(display, np->window);
      }
      JXUngrabButton(display, AnyButton, AnyModifier, np->window);
      JXReparentWindow(display, np->window, rootWindow, np->x, np->y);
      JXRemoveFromSaveSet(display, np->window);
   }

   /* Destroy the parent */
   if(np->parent) {
      JXDestroyWindow(display, np->parent);
   }

   if(np->name) {
      JXFree(np->name);
   }
   if(np->instanceName) {
      JXFree(np->instanceName);
   }
   if(np->className) {
      JXFree(np->className);
   }

   RemoveClientFromTaskBar(np);
   RemoveClientStrut(np);
   UpdatePager();

   while(np->colormaps) {
      cp = np->colormaps->next;
      Release(np->colormaps);
      np->colormaps = cp;
   }

   DestroyIcon(np->icon);

   Release(np);

   JXUngrabServer(display);

   RestackClients();

}

/** Get the active client (possibly NULL). */
ClientNode *GetActiveClient() {

   return activeClient;

}

/** Find a client given a window (searches frame windows too). */
ClientNode *FindClientByWindow(Window w) {

   ClientNode *np;

   if(!XFindContext(display, w, clientContext, (void*)&np)) {
      return np;
   } else {
      return FindClientByParent(w);
   }

}

/** Find a client by its frame window. */
ClientNode *FindClientByParent(Window p) {

   ClientNode *np;

   if(!XFindContext(display, p, frameContext, (void*)&np)) {
      return np;
   } else {
      return NULL;
   }

}

/** Reparent a client window. */
void ReparentClient(ClientNode *np, int notOwner) {

   XSetWindowAttributes attr;
   int attrMask;
   int x, y, width, height;
   int north, south, east, west;

   Assert(np);

   if(notOwner) {
      JXAddToSaveSet(display, np->window);

      attr.event_mask
         = EnterWindowMask
         | ColormapChangeMask
         | PropertyChangeMask
         | KeyReleaseMask
         | StructureNotifyMask;
      attr.do_not_propagate_mask = NoEventMask;
      XChangeWindowAttributes(display, np->window,
         CWEventMask | CWDontPropagate, &attr);

   }
  JXGrabButton(display, AnyButton, AnyModifier, np->window,
     True, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
   GrabKeys(np);

   attrMask = 0;

   attrMask |= CWOverrideRedirect;
   attr.override_redirect = True;

   attrMask |= CWBackPixmap;
   attr.background_pixmap = ParentRelative;

   /* We can't use PointerMotionHint mask here since the exact location
    * of the mouse on the frame is important. */
   attrMask |= CWEventMask;
   attr.event_mask
      = ButtonPressMask
      | ButtonReleaseMask
      | ExposureMask
      | PointerMotionMask
      | SubstructureRedirectMask
      | SubstructureNotifyMask
      | EnterWindowMask
      | LeaveWindowMask
      | KeyPressMask
      | KeyReleaseMask;

   attrMask |= CWDontPropagate;
   attr.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask;

   attrMask |= CWBackPixel;
   attr.background_pixel = colors[COLOR_TITLE_BG2];

   x = np->x;
   y = np->y;
   width = np->width;
   height = np->height;
   GetBorderSize(np, &north, &south, &east, &west);
   x -= west;
   y -= north;
   width += east + west;
   height += north + south;

   /* Create the frame window. */
   np->parent = JXCreateWindow(display, rootWindow,
      x, y, width, height, 0, rootDepth, InputOutput,
      rootVisual, attrMask, &attr);
 
   /* Update the window to get only the events we want. */
   attrMask = CWDontPropagate;
   attr.do_not_propagate_mask
      = ButtonPressMask
      | ButtonReleaseMask
      | PointerMotionMask
      | ButtonMotionMask
      | KeyPressMask
      | KeyReleaseMask;
   JXChangeWindowAttributes(display, np->window, attrMask, &attr);
   JXSetWindowBorderWidth(display, np->window, 0);

   /* Reparent the client window. */
   JXReparentWindow(display, np->window, np->parent, west, north);

#ifdef USE_SHAPE
   if(haveShape) {
      JXShapeSelectInput(display, np->window, ShapeNotifyMask);
      CheckShape(np);
   }
#endif

}

/** Determine if a window uses the shape extension. */
#ifdef USE_SHAPE
void CheckShape(ClientNode *np) {

   int xb, yb;
   int xc, yc;
   unsigned int wb, hb;
   unsigned int wc, hc;
   Bool boundingShaped, clipShaped;

   JXShapeQueryExtents(display, np->window, &boundingShaped,
      &xb, &yb, &wb, &hb, &clipShaped, &xc, &yc, &wc, &hc);

   if(boundingShaped == True) {
      SetShape(np);
   }

}
#endif

/** Send a configure event to a client window. */
void SendConfigureEvent(ClientNode *np) {

   XConfigureEvent event;
   const ScreenType *sp;

   Assert(np);

   event.type = ConfigureNotify;
   event.event = np->window;
   event.window = np->window;

   if(np->state.status & STAT_FULLSCREEN) {
      sp = GetCurrentScreen(np->x, np->y);
      event.x = sp->x;
      event.y = sp->y;
      event.width = sp->width;
      event.height = sp->height;
   } else {
      event.x = np->x;
      event.y = np->y;
      event.width = np->width;
      event.height = np->height;
   }

   event.border_width = 0;
   event.above = None;
   event.override_redirect = False;

   JXSendEvent(display, np->window, False, StructureNotifyMask,
      (XEvent*)&event);

}

/** Update a window's colormap.
 * A call to this function indicates that the colormap(s) for the given
 * client changed. This will change the active colormap(s) if the given
 * client is active.
 */
void UpdateClientColormap(ClientNode *np) {

   XWindowAttributes attr;
   ColormapNode *cp;
   int wasInstalled;

   Assert(np);

   cp = np->colormaps;

   if(np == activeClient) {

      wasInstalled = 0;
      cp = np->colormaps;
      while(cp) {
         if(JXGetWindowAttributes(display, cp->window, &attr)) {
            if(attr.colormap != None) {
               if(attr.colormap == np->cmap) {
                  wasInstalled = 1;
               }
               JXInstallColormap(display, attr.colormap);
            }
         }
         cp = cp->next;
      }

      if(!wasInstalled && np->cmap != None) {
         JXInstallColormap(display, np->cmap);
      }

   }

}

/** Set the opacity for active clients. */
void SetActiveClientOpacity(const char *str) {

   double temp;

   Assert(str);

   temp = atof(str);
   if(temp <= 0.0 || temp > 1.0) {
      Warning("invalid active client opacity: %s", str);
      activeOpacity = UINT_MAX;
   } else {
      activeOpacity = (unsigned int)(1.0 * UINT_MAX);
   }

}

/** Set the opacity range for inactive clients. */
void SetInactiveClientOpacity(const char *str) {

   double temp;
   const char *str_u;
   const char *str_d;
   unsigned int first;
   unsigned int second;

   Assert(str);

   /* Reset in case there's a problem. */
   maxInactiveOpacity = (unsigned int)(0.9 * UINT_MAX);
   minInactiveOpacity = (unsigned int)(0.5 * UINT_MAX);
   deltaInactiveOpacity = (unsigned int)(0.1 * UINT_MAX);

   /* Read the first (or only) bound of the range. */
   temp = atof(str);
   if(temp < 0.0 || temp > 1.0) {
      Warning("invalid inactive client opacity: %s", str);
      return;
   }
   first = (unsigned int)(temp * UINT_MAX);
   second = first;

   /* Check for a range. */
   str_u = strchr(str, ':');
   if(str_u) {

      /* A range was specified. */
      temp = atof(str_u + 1);
      if(temp < 0.0 || temp > 1.0) {
         Warning("invalid inactive client opacity: %s", str);
         return;
      }
      second = (unsigned int)(temp * UINT_MAX);

      /* Check for a delta. */
      str_d = strchr(str_u + 1, ':');
      if(str_d) {
         temp = atof(str_d + 1);
         if(temp <= 0.0 || temp > 1.0) {
            Warning("invalid inactive client opacity delta: %s", str);
            return;
         }
         deltaInactiveOpacity = (unsigned int)(temp * UINT_MAX);
      }

   }

   /* Set the min/max opacities. */
   if(first > second) {
      minInactiveOpacity = second;
      maxInactiveOpacity = first;
   } else {
      minInactiveOpacity = first;
      maxInactiveOpacity = second;
   }

}

