/**
 * @file client.c
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
#include "event.h"
#include "settings.h"

static ClientNode *activeClient;

static int clientCount;

static void LoadFocus();
static void ReparentClient(ClientNode *np, char notOwner);
static void MinimizeTransients(ClientNode *np);
static void RestoreTransients(ClientNode *np, char raise);
static void KillClientHandler(ClientNode *np);

/** Initialize client data. */
void InitializeClients()
{
}

/** Load windows that are already mapped. */
void StartupClients()
{

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
void ShutdownClients()
{

   int x;

   for(x = 0; x < LAYER_COUNT; x++) {
      while(nodeTail[x]) {
         RemoveClient(nodeTail[x]);
      }
   }

}

/** Destroy client data. */
void DestroyClients()
{
}

/** Set the focus to the window currently under the mouse pointer. */
void LoadFocus()
{

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
ClientNode *AddClientWindow(Window w, char alreadyMapped, char notOwner)
{

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

   ReadClientInfo(np, alreadyMapped);

   if(!notOwner) {
      np->state.border = BORDER_OUTLINE | BORDER_TITLE | BORDER_MOVE;
      np->state.status |= STAT_WMDIALOG | STAT_STICKY;
      np->state.layer = LAYER_ABOVE;
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
      np->state.status &= ~STAT_SHADED;
      ShadeClient(np);
   }

   /* Minimize the client if requested. */
   if(np->state.status & STAT_MINIMIZED) {
      np->state.status &= ~STAT_MINIMIZED;
      MinimizeClient(np);
   }

   /* Maximize the client if requested. */
   if(np->state.status & (STAT_HMAX | STAT_VMAX)) {
      const char hmax = (np->state.status & STAT_HMAX) ? 1 : 0;
      const char vmax = (np->state.status & STAT_VMAX) ? 1 : 0;
      np->state.status &= ~(STAT_HMAX | STAT_VMAX);
      MaximizeClient(np, hmax, vmax);
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
void MinimizeClient(ClientNode *np)
{

   Assert(np);

   if(settings.focusModel == FOCUS_CLICK && np == activeClient) {
      FocusNextStacked(np);
   }

   MinimizeTransients(np);

   UpdateTaskBar();
   UpdatePager();

}

/** Minimize all transients as well as the specified client. */
void MinimizeTransients(ClientNode *np)
{

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
      if(np->state.status & STAT_MAPPED) {
         JXUnmapWindow(display, np->window);
      }
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
void ShadeClient(ClientNode *np)
{

   int north, south, east, west;

   Assert(np);

   if(!(np->state.border & BORDER_TITLE)) {
      return;
   }
   if(np->state.status & STAT_SHADED) {
      return;
   }
   if(!(np->state.border & BORDER_SHADE)) {
      return;
   }

   GetBorderSize(&np->state, &north, &south, &east, &west);

   if(np->state.status & STAT_MAPPED) {
      JXUnmapWindow(display, np->window);
   }
   np->state.status |= STAT_SHADED;
   np->state.status &= ~STAT_SDESKTOP;
   np->state.status &= ~STAT_MAPPED;

   JXResizeWindow(display, np->parent, np->width + east + west, north);

   /* Shape will up updated in HandleUnmapNotify. */

   WriteState(np);
   UpdatePager();

}

/** Unshade a client. */
void UnshadeClient(ClientNode *np)
{

   int north, south, east, west;

   Assert(np);

   if(!(np->state.border & BORDER_TITLE)) {
      return;
   }

   if(!(np->state.status & STAT_SHADED)) {
      return;
   }

   JXMapWindow(display, np->window);
   np->state.status |= STAT_MAPPED;
   np->state.status &= ~STAT_SHADED;

   GetBorderSize(&np->state, &north, &south, &east, &west);

   JXResizeWindow(display, np->parent,
                  np->width + west + east,
                  np->height + north + south);

   WriteState(np);

   ResetRoundedRectWindow(np);

   RefocusClient();
   UpdatePager();

}

/** Set a client's state to withdrawn. */
void SetClientWithdrawn(ClientNode *np)
{

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
      if(!(np->state.status & STAT_MINIMIZED)) {
         JXUnmapWindow(display, np->parent);
      }
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
void RestoreTransients(ClientNode *np, char raise)
{

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
void RestoreClient(ClientNode *np, char raise)
{

   Assert(np);

   RestoreTransients(np, raise);

   RestackClients();
   UpdateTaskBar();
   UpdatePager();

}

/** Set the client layer. This will affect transients. */
void SetClientLayer(ClientNode *np, unsigned int layer)
{

   ClientNode *tp, *next;
   int x;

   Assert(np);
   Assert(layer >= FIRST_LAYER);
   Assert(layer <= LAST_LAYER);

   if(np->state.layer != layer) {

      /* Loop through all clients so we get transients. */
      for(x = FIRST_LAYER; x <= LAST_LAYER; x++) {
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
void SetClientSticky(ClientNode *np, char isSticky)
{

   ClientNode *tp;
   int x;
   char old;

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
void SetClientDesktop(ClientNode *np, unsigned int desktop)
{

   ClientNode *tp;
   int x;

   Assert(np);

   if(JUNLIKELY(desktop >= settings.desktopCount)) {
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
void HideClient(ClientNode *np)
{

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
void ShowClient(ClientNode *np)
{

   Assert(np);

   if(np->state.status & STAT_HIDDEN) {
      np->state.status &= ~STAT_HIDDEN;
      if(np->state.status & (STAT_MAPPED | STAT_SHADED)) {
         if(!(np->state.status & STAT_MINIMIZED)) {
            JXMapWindow(display, np->parent);
            if(np->state.status & STAT_ACTIVE) {
               FocusClient(np);
            }
         }
      }
   }

}

/** Maximize a client window. */
void MaximizeClient(ClientNode *np, char horiz, char vert)
{

   int north, south, east, west;

   Assert(np);

   /* We don't want to mess with full screen clients. */
   if(np->state.status & STAT_FULLSCREEN) {
      return; 
   }

   if(np->state.status & STAT_SHADED) {
      UnshadeClient(np);
   }

   GetBorderSize(&np->state, &north, &south, &east, &west);

   if(np->state.status & (STAT_HMAX | STAT_VMAX)) {
      np->x = np->oldx;
      np->y = np->oldy;
      np->width = np->oldWidth;
      np->height = np->oldHeight;
      np->state.status &= ~(STAT_HMAX | STAT_VMAX);
   } else {
      PlaceMaximizedClient(np, horiz, vert);
   }

   JXMoveResizeWindow(display, np->parent,
                      np->x - west, np->y - north,
                      np->width + east + west,
                      np->height + north + south);
   JXMoveResizeWindow(display, np->window, west,
                      north, np->width, np->height);
   ResetRoundedRectWindow(np);

   WriteState(np);
   SendConfigureEvent(np);
   UpdatePager();

}

/** Maximize a client using its default maximize settings. */
void MaximizeClientDefault(ClientNode *np)
{

   char hmax, vmax;

   Assert(np);

   hmax = (np->state.border & BORDER_MAX_H) ? 1 : 0;
   vmax = (np->state.border & BORDER_MAX_V) ? 1 : 0;

   MaximizeClient(np, hmax, vmax);

}

/** Set a client's full screen state. */
void SetClientFullScreen(ClientNode *np, char fullScreen)
{

   XEvent event;
   int north, south, east, west;
   BoundingBox box;
   const ScreenType *sp;

   Assert(np);

   /* Make sure there's something to do. */
   if(!fullScreen == !(np->state.status & STAT_FULLSCREEN)) {
      return;
   }

   if(np->state.status & STAT_SHADED) {
      UnshadeClient(np);
   }

   if(fullScreen) {

      np->state.status |= STAT_FULLSCREEN;
      np->state.border &= ~BORDER_MOVE;
      SetClientLayer(np, LAYER_ABOVE);

      np->oldx = np->x;
      np->oldy = np->y;
      np->oldWidth = np->width;
      np->oldHeight = np->height;

      sp = GetCurrentScreen(np->x, np->y);
      GetScreenBounds(sp, &box);

      GetBorderSize(&np->state, &north, &south, &east, &west);
      box.x += west;
      box.y += north;
      box.width -= east + west;
      box.height -= north + south;

      np->x = box.x;
      np->y = box.y;
      np->width = box.width;
      np->height = box.height;

      JXMoveResizeWindow(display, np->parent,
                         np->x - west, np->y - north,
                         np->width + east + west,
                         np->height + north + south);
      JXMoveResizeWindow(display, np->window, west, north,
                         np->width, np->height);
      ResetRoundedRectWindow(np);

   } else {

      np->state.status &= ~STAT_FULLSCREEN;
      np->state.border |= BORDER_MOVE;
      np->x = np->oldx;
      np->y = np->oldy;
      np->width = np->oldWidth;   
      np->height = np->oldHeight;

      GetBorderSize(&np->state, &north, &south, &east, &west);

      JXMoveResizeWindow(display, np->parent,
                         np->x - west,
                         np->y - north,
                         np->width + east + west,
                         np->height + north + south);
      JXMoveResizeWindow(display, np->window,
                         west, north, np->width, np->height);
      ResetRoundedRectWindow(np);

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
void FocusClient(ClientNode *np)
{

   Assert(np);

   if(np->state.status & STAT_HIDDEN) {
      return;
   }

   if(activeClient != np || !(np->state.status & STAT_ACTIVE)) {

      if(activeClient) {
         activeClient->state.status &= ~STAT_ACTIVE;
         DrawBorder(activeClient);
      }
      np->state.status |= STAT_ACTIVE;
      activeClient = np;

      DrawBorder(np);
      UpdatePager();
      UpdateTaskBar();

   }

   if(np->state.status & STAT_MAPPED) {
      UpdateClientColormap(np);
      SetWindowAtom(rootWindow, ATOM_NET_ACTIVE_WINDOW, np->window);
      if(np->state.status & STAT_CANFOCUS) {
         JXSetInputFocus(display, np->window, RevertToPointerRoot, eventTime);
      } else {
         JXSetInputFocus(display, rootWindow, RevertToPointerRoot, eventTime);
      }
      if(np->state.status & STAT_TAKEFOCUS) {
         SendClientMessage(np->window, ATOM_WM_PROTOCOLS, ATOM_WM_TAKE_FOCUS);
      }
   } else {
      JXSetInputFocus(display, rootWindow, RevertToPointerRoot, eventTime);
   }

}


/** Refocus the active client (if there is one). */
void RefocusClient()
{

   if(activeClient) {
      FocusClient(activeClient);
   }

}

/** Send a delete message to a client. */
void DeleteClient(ClientNode *np)
{
   Assert(np);
   ReadWMProtocols(np->window, &np->state);
   if(np->state.status & STAT_DELETE) {
      SendClientMessage(np->window, ATOM_WM_PROTOCOLS, ATOM_WM_DELETE_WINDOW);
   } else {
      KillClient(np);
   }
}

/** Callback to kill a client after a confirm dialog. */
void KillClientHandler(ClientNode *np)
{

   Assert(np);

   if(np == activeClient) {
      FocusNextStacked(np);
   }

   JXGrabServer(display);
   JXKillClient(display, np->window);
   JXSync(display, True);
   JXUngrabServer(display);

   RemoveClient(np);

}

/** Kill a client window. */
void KillClient(ClientNode *np)
{
   Assert(np);
   ShowConfirmDialog(np, KillClientHandler,
      _("Kill this window?"),
      _("This may cause data to be lost!"),
      NULL);
}

/** Raise the client. This will affect transients. */
void RaiseClient(ClientNode *np)
{

   ClientNode *tp, *next;
   unsigned int x;

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
void LowerClient(ClientNode *np)
{

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
void RestackClients()
{

   TrayType *tp;
   ClientNode *np;
   unsigned int layer, index;
   int trayCount;
   Window *stack;
   unsigned int opacity;
   unsigned int temp;
   char isFirst;

   if(JUNLIKELY(shouldExit)) {
      return;
   }

   /* Allocate memory for restacking. */
   trayCount = GetTrayCount();
   stack = AllocateStack((clientCount + trayCount) * sizeof(Window));

   /* Prepare the stacking array. */
   index = 0;
   layer = LAST_LAYER;
   isFirst = 1;
   opacity = settings.maxClientOpacity;
   for(;;) {

      for(np = nodes[layer]; np; np = np->next) {
         if((np->state.status & (STAT_MAPPED | STAT_SHADED))
            && !(np->state.status & STAT_HIDDEN)) {
            stack[index++] = np->parent;
            if(isFirst) {
               if(   !(np->state.status & STAT_OPACITY)
                  && np->state.opacity != settings.activeClientOpacity) {
                  np->state.opacity = settings.activeClientOpacity;
                  WriteState(np);
               }
               isFirst = 0;
            } else if(!(np->state.status & STAT_OPACITY)) {
               if(np->state.opacity != opacity) {
                  np->state.opacity = opacity;
                  WriteState(np);
               }
               temp = opacity - settings.deltaClientOpacity;
               if(temp < settings.minClientOpacity || temp > opacity) {
                  opacity = settings.minClientOpacity;
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

      if(layer == FIRST_LAYER) {
         break;
      }
      --layer;

   }

   JXRestackWindows(display, stack, index);

   ReleaseStack(stack);

   UpdateNetClientList();

}

/** Send a client message to a window. */
void SendClientMessage(Window w, AtomType type, AtomType message)
{

   XEvent event;
   int status;

   memset(&event, 0, sizeof(event));
   event.xclient.type = ClientMessage;
   event.xclient.window = w;
   event.xclient.message_type = atoms[type];
   event.xclient.format = 32;
   event.xclient.data.l[0] = atoms[message];
   event.xclient.data.l[1] = eventTime;

   status = JXSendEvent(display, w, False, 0, &event);
   if(JUNLIKELY(status == False)) {
      Debug("SendClientMessage failed");
   }

}

/** Remove a client window from management. */
void RemoveClient(ClientNode *np)
{

   ColormapNode *cp;

   Assert(np);
   Assert(np->window != None);
   Assert(np->parent != None);

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

      /* Must be the last client. */

      SetWindowAtom(rootWindow, ATOM_NET_ACTIVE_WINDOW, None);
      activeClient = NULL;

      JXSetInputFocus(display, rootWindow, RevertToPointerRoot, eventTime);

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

   RestackClients();

}

/** Get the active client (possibly NULL). */
ClientNode *GetActiveClient()
{
   return activeClient;
}

/** Find a client given a window (searches frame windows too). */
ClientNode *FindClientByWindow(Window w)
{

   ClientNode *np;

   if(!XFindContext(display, w, clientContext, (void*)&np)) {
      return np;
   } else {
      return FindClientByParent(w);
   }

}

/** Find a client by its frame window. */
ClientNode *FindClientByParent(Window p)
{
   ClientNode *np;
   if(!XFindContext(display, p, frameContext, (void*)&np)) {
      return np;
   } else {
      return NULL;
   }
}

/** Reparent a client window. */
void ReparentClient(ClientNode *np, char notOwner)
{

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
      JXChangeWindowAttributes(display, np->window,
                               CWEventMask | CWDontPropagate, &attr);

   }
   JXGrabButton(display, AnyButton, AnyModifier, np->window, True,
                ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
   GrabKeys(np);

   attrMask = 0;

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
   GetBorderSize(&np->state, &north, &south, &east, &west);
   x -= west;
   y -= north;
   width += east + west;
   height += north + south;

   /* Create the frame window. */
   np->parent = JXCreateWindow(display, rootWindow, x, y, width, height,
                               0, rootDepth, InputOutput,
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
      ResetRoundedRectWindow(np);
   }
#endif

}

/** Send a configure event to a client window. */
void SendConfigureEvent(ClientNode *np)
{

   XConfigureEvent event;
   const ScreenType *sp;

   Assert(np);

   memset(&event, 0, sizeof(event));
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

   JXSendEvent(display, np->window, False, StructureNotifyMask,
               (XEvent*)&event);

}

/** Update a window's colormap.
 * A call to this function indicates that the colormap(s) for the given
 * client changed. This will change the active colormap(s) if the given
 * client is active.
 */
void UpdateClientColormap(ClientNode *np)
{

   XWindowAttributes attr;
   ColormapNode *cp;
   char wasInstalled;

   Assert(np);

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

