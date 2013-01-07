/**
 * @file event.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions to handle X11 events.
 *
 */

#include "jwm.h"
#include "event.h"

#include "client.h"
#include "clientlist.h"
#include "clock.h"
#include "confirm.h"
#include "cursor.h"
#include "desktop.h"
#include "dock.h"
#include "hint.h"
#include "icon.h"
#include "key.h"
#include "main.h"
#include "move.h"
#include "pager.h"
#include "place.h"
#include "popup.h"
#include "resize.h"
#include "root.h"
#include "swallow.h"
#include "taskbar.h"
#include "timing.h"
#include "tray.h"
#include "traybutton.h"
#include "winmenu.h"
#include "error.h"
#include "settings.h"

#define MIN_TIME_DELTA 50

Time eventTime = CurrentTime;

static void Signal();
static void DispatchBorderButtonEvent(const XButtonEvent *event,
                                      ClientNode *np);

static void HandleConfigureRequest(const XConfigureRequestEvent *event);
static char HandleConfigureNotify(const XConfigureEvent *event);
static char HandleExpose(const XExposeEvent *event);
static char HandlePropertyNotify(const XPropertyEvent *event);
static void HandleClientMessage(const XClientMessageEvent *event);
static void HandleColormapChange(const XColormapEvent *event);
static char HandleDestroyNotify(const XDestroyWindowEvent *event);
static void HandleMapRequest(const XMapEvent *event);
static void HandleUnmapNotify(const XUnmapEvent *event);
static void HandleButtonEvent(const XButtonEvent *event);
static void HandleKeyPress(const XKeyEvent *event);
static void HandleKeyRelease(const XKeyEvent *event);
static void HandleEnterNotify(const XCrossingEvent *event);
static void HandleLeaveNotify(const XCrossingEvent *event);
static void HandleMotionNotify(const XMotionEvent *event);
static char HandleSelectionClear(const XSelectionClearEvent *event);

static void HandleNetMoveResize(const XClientMessageEvent *event,
                                ClientNode *np);
static void HandleNetWMState(const XClientMessageEvent *event,
                             ClientNode *np);

#ifdef USE_SHAPE
static void HandleShapeEvent(const XShapeEvent *event);
#endif

/** Wait for an event and process it. */
void WaitForEvent(XEvent *event)
{

   struct timeval timeout;
   fd_set fds;
   int fd;
   char handled;

   fd = JXConnectionNumber(display);

   do {

      while(JXPending(display) == 0) {
         FD_ZERO(&fds);
         FD_SET(fd, &fds);
         timeout.tv_usec = 0;
         timeout.tv_sec = 1;
         if(select(fd + 1, &fds, NULL, NULL, &timeout) <= 0) {
            Signal();
         }
      }

      Signal();

      JXNextEvent(display, event);
      UpdateTime(event);

      switch(event->type) {
      case ConfigureRequest:
         HandleConfigureRequest(&event->xconfigurerequest);
         handled = 1;
         break;
      case MapRequest:
         HandleMapRequest(&event->xmap);
         handled = 1;
         break;
      case PropertyNotify:
         handled = HandlePropertyNotify(&event->xproperty);
         break;
      case ClientMessage:
         HandleClientMessage(&event->xclient);
         handled = 1;
         break;
      case UnmapNotify:
         HandleUnmapNotify(&event->xunmap);
         handled = 1;
         break;
      case Expose:
         handled = HandleExpose(&event->xexpose);
         break;
      case ColormapNotify:
         HandleColormapChange(&event->xcolormap);
         handled = 1;
         break;
      case DestroyNotify:
         handled = HandleDestroyNotify(&event->xdestroywindow);
         break;
      case SelectionClear:
         handled = HandleSelectionClear(&event->xselectionclear);
         break;
      case ResizeRequest:
         handled = HandleDockResizeRequest(&event->xresizerequest);
         break;
      case MotionNotify:
         SetMousePosition(event->xmotion.x_root, event->xmotion.y_root);
         handled = 0;
         break;
      case ReparentNotify:
         HandleDockReparentNotify(&event->xreparent);
         handled = 1;
         break;
      case ConfigureNotify:
         handled = HandleConfigureNotify(&event->xconfigure);
         break;
      case CreateNotify:
      case MapNotify:
      case GraphicsExpose:
      case NoExpose:
         handled = 1;
         break;
      default:
         if(0) {
#ifdef USE_SHAPE
         } else if(haveShape && event->type == shapeEvent) {
            HandleShapeEvent((XShapeEvent*)event);
            handled = 1;
#endif
         } else {
            handled = 0;
         }
         break;
      }

      if(!handled) {
         handled = ProcessTrayEvent(event);
      }
      if(!handled) {
         handled = ProcessDialogEvent(event);
      }
      if(!handled) {
         handled = ProcessSwallowEvent(event);
      }
      if(!handled) {
         handled = ProcessPopupEvent(event);
      }

   } while(handled && !shouldExit);

}

/** Wake up components that need to run at certain times. */
void Signal()
{

   static TimeType last = ZERO_TIME;

   TimeType now;
   int x, y;

   GetCurrentTime(&now);

   if(GetTimeDifference(&now, &last) < MIN_TIME_DELTA) {
      return;
   }
   last = now;

   GetMousePosition(&x, &y);

   SignalTaskbar(&now, x, y);
   SignalTrayButton(&now, x, y);
   SignalClock(&now, x, y);
   SignalTray(&now, x, y);
   SignalPager(&now, x, y);
   SignalPopup(&now, x, y);

}

/** Process an event. */
void ProcessEvent(XEvent *event)
{
   switch(event->type) {
   case ButtonPress:
   case ButtonRelease:
      HandleButtonEvent(&event->xbutton);
      break;
   case KeyPress:
      HandleKeyPress(&event->xkey);
      break;
   case KeyRelease:
      HandleKeyRelease(&event->xkey);
      break;
   case EnterNotify:
      HandleEnterNotify(&event->xcrossing);
      break;
   case LeaveNotify:
      HandleLeaveNotify(&event->xcrossing);
      break;
   case MotionNotify:
      while(JXCheckTypedEvent(display, MotionNotify, event));
      UpdateTime(event);
      HandleMotionNotify(&event->xmotion);
      break;
   case DestroyNotify:
   case Expose:
   case ConfigureNotify:
      break;
   default:
      Debug("Unknown event type: %d", event->type);
      break;
   }
}

/** Discard motion events for the specified window. */
void DiscardMotionEvents(XEvent *event, Window w)
{
   XEvent temp;
   while(JXCheckTypedEvent(display, MotionNotify, &temp)) {
      UpdateTime(event);
      SetMousePosition(temp.xmotion.x_root, temp.xmotion.y_root);
      if(temp.xmotion.window == w) {
         *event = temp;
      }
   }
}

/** Process a selection clear event. */
char HandleSelectionClear(const XSelectionClearEvent *event)
{
   return HandleDockSelectionClear(event);
}

/** Process a button event. */
void HandleButtonEvent(const XButtonEvent *event)
{

   ClientNode *np;
   int north, south, east, west;

   np = FindClientByParent(event->window);
   if(np) {
      if(event->type == ButtonPress) {
         RaiseClient(np);
         if(settings.focusModel == FOCUS_CLICK) {
            FocusClient(np);
         }
      }
      DispatchBorderButtonEvent(event, np);
   } else if(event->window == rootWindow && event->type == ButtonPress) {
      if(!ShowRootMenu(event->button, event->x, event->y)) {
         if(event->button == 4) {
            LeftDesktop();
         } else if(event->button == 5) {
            RightDesktop();
         }
      }
   } else {
      np = FindClientByWindow(event->window);
      if(np) {
         switch(event->button) {
         case Button1:
         case Button2:
            RaiseClient(np);
            if(settings.focusModel == FOCUS_CLICK) {
               FocusClient(np);
            }
            if(event->state & Mod1Mask) {
               GetBorderSize(np, &north, &south, &east, &west);
               MoveClient(np, event->x + west, event->y + north, 0);
            }
            break;
         case Button3:
            if(event->state & Mod1Mask) {
               GetBorderSize(np, &north, &south, &east, &west);
               ResizeClient(np, BA_RESIZE | BA_RESIZE_E | BA_RESIZE_S,
                            event->x + west, event->y + north);
            } else {
               RaiseClient(np);
               if(settings.focusModel == FOCUS_CLICK) {
                  FocusClient(np);
               }
            }
            break;
         default:
            break;
         }
         JXAllowEvents(display, ReplayPointer, eventTime);
      }

   }

}

/** Process a key press event. */
void HandleKeyPress(const XKeyEvent *event)
{
   ClientNode *np;
   KeyType key;
   key = GetKey(event);
   np = GetActiveClient();
   switch(key & 0xFF) {
   case KEY_EXEC:
      RunKeyCommand(event);
      break;
   case KEY_DESKTOP:
      ChangeDesktop((key >> 8) - 1);
      break;
   case KEY_RDESKTOP:
      RightDesktop();
      break;
   case KEY_LDESKTOP:
      LeftDesktop();
      break;
   case KEY_UDESKTOP:
      AboveDesktop();
      break;
   case KEY_DDESKTOP:
      BelowDesktop();
      break;
   case KEY_SHOWDESK:
      ShowDesktop();
      break;
   case KEY_SHOWTRAY:
      ShowAllTrays();
      break;
   case KEY_NEXT:
      FocusNext();
      break;
   case KEY_NEXTSTACK:
      StartWindowStackWalk();
      WalkWindowStack(1);
      break;
   case KEY_PREV:
      FocusPrevious();
      break;
   case KEY_PREVSTACK:
      StartWindowStackWalk();
      WalkWindowStack(0);
      break;
   case KEY_CLOSE:
      if(np) {
         DeleteClient(np);
      }
      break;
   case KEY_SHADE:
      if(np) {
         if(np->state.status & STAT_SHADED) {
            UnshadeClient(np);
         } else {
            ShadeClient(np);
         }
      }
      break;
   case KEY_STICK:
      if(np) {
         if(np->state.status & STAT_STICKY) {
            SetClientSticky(np, 0);
         } else {
            SetClientSticky(np, 1);
         }
      }
      break;
   case KEY_MOVE:
      if(np) {
         MoveClientKeyboard(np);
      }
      break;
   case KEY_RESIZE:
      if(np) {
         ResizeClientKeyboard(np);
      }
      break;
   case KEY_MIN:
      if(np) {
         MinimizeClient(np);
      }
      break;
   case KEY_MAX:
      if(np) {
         MaximizeClient(np, 1, 1);
      }
      break;
   case KEY_ROOT:
      ShowKeyMenu(event);
      break;
   case KEY_WIN:
      if(np) {
         ShowWindowMenu(np, np->x, np->y);
      }
      break;
   case KEY_RESTART:
      Restart();
      break;
   case KEY_EXIT:
      Exit();
      break;
   case KEY_FULLSCREEN:
      if(np) {
         if(np->state.status & STAT_FULLSCREEN) {
            SetClientFullScreen(np, 0);
         } else {
            SetClientFullScreen(np, 1);
         }
      }
      break;
   default:
      break;
   }
}

/** Handle a key release event. */
void HandleKeyRelease(const XKeyEvent *event)
{
   KeyType key;
   key = GetKey(event);
   if(((key & 0xFF) != KEY_NEXTSTACK) &&
      ((key & 0xFF) != KEY_PREVSTACK)) {
      StopWindowStackWalk();
   }
}

/** Process a configure request. */
void HandleConfigureRequest(const XConfigureRequestEvent *event)
{
   XWindowChanges wc;
   ClientNode *np;
   int north, south, east, west;
   int changed;
   int handled;

   handled = HandleDockConfigureRequest(event);
   if(handled) {
      return;
   }

   np = FindClientByWindow(event->window);
   if(np && np->window == event->window) {

      changed = 0;
      if((event->value_mask & CWWidth) && (event->width != np->width)) {
         np->width = event->width;
         changed = 1;
      }
      if((event->value_mask & CWHeight) && (event->height != np->height)) {
         np->height = event->height;
         changed = 1;
      }
      if((event->value_mask & CWX) && (event->x != np->x)) {
         np->x = event->x;
         changed = 1;
      }
      if((event->value_mask & CWY) && (event->y != np->y)) {
         np->y = event->y;
         changed = 1;
      }

      if(!changed) {
         return;
      }

      if(np->controller) {
         (np->controller)(0);
      }

      GetBorderSize(np, &north, &south, &east, &west);

      wc.stack_mode = Above;
      wc.sibling = np->parent;
      wc.border_width = 0;

      ConstrainSize(np);

      wc.x = np->x;
      wc.y = np->y;
      wc.width = np->width + east + west;
      wc.height = np->height + north + south;
      JXConfigureWindow(display, np->parent, event->value_mask, &wc);

      wc.x = west;
      wc.y = north;
      wc.width = np->width;
      wc.height = np->height;
      JXConfigureWindow(display, np->window, event->value_mask, &wc);

      ResetRoundedRectWindow(np);
      ShapeRoundedRectWindow(np->parent, np->width + east + west,
                             np->height + north + south);

   } else {

      /* We don't know about this window, just let the configure through. */

      wc.stack_mode = event->detail;
      wc.sibling = event->above;
      wc.border_width = event->border_width;
      wc.x = event->x;
      wc.y = event->y;
      wc.width = event->width;
      wc.height = event->height;
      JXConfigureWindow(display, event->window, event->value_mask, &wc);

   }
}

/** Process a configure notify event. */
char HandleConfigureNotify(const XConfigureEvent *event)
{
   if(event->window != rootWindow) {
      return 0;
   }
   if(rootWidth != event->width || rootHeight != event->height) {
      rootWidth = event->width;
      rootHeight = event->height;
      shouldRestart = 1;
      shouldExit = 1;
   }
   return 1;
}

/** Process an enter notify event. */
void HandleEnterNotify(const XCrossingEvent *event)
{
   ClientNode *np;
   Cursor cur;

   SetMousePosition(event->x_root, event->y_root);
   np = FindClientByWindow(event->window);
   if(np) {
      if(  !(np->state.status & STAT_ACTIVE)
         && (settings.focusModel == FOCUS_SLOPPY)) {
         FocusClient(np);
      }
      if(np->parent == event->window) {
         np->borderAction = GetBorderActionType(np, event->x, event->y);
         cur = GetFrameCursor(np->borderAction);
         JXDefineCursor(display, np->parent, cur);
      } else if(np->borderAction != BA_NONE) {
         SetDefaultCursor(np->parent);
         np->borderAction = BA_NONE;
      }
   }

}

/** Process a leave notify event. */
void HandleLeaveNotify(const XCrossingEvent *event)
{
   SetMousePosition(event->x_root, event->y_root);
}

/** Handle an expose event. */
char HandleExpose(const XExposeEvent *event)
{
   ClientNode *np;
   np = FindClientByWindow(event->window);
   if(np) {
      if(event->window == np->parent) {
         if(event->count == 0) {
            DrawBorder(np);
         }
         return 1;
      } else if(event->window == np->window
         && np->state.status & STAT_WMDIALOG) {

         /* Dialog expose events are handled elsewhere. */
         return 0;

      } else {

         /* Ignore other expose events. */
         return 1;

      }
   } else {
      return event->count ? 1 : 0;
   }
}

/** Handle a property notify event. */
char HandlePropertyNotify(const XPropertyEvent *event)
{
   ClientNode *np;
   char changed;
   np = FindClientByWindow(event->window);
   if(np) {
      changed = 0;
      switch(event->atom) {
      case XA_WM_NAME:
         ReadWMName(np);
         changed = 1;
         break;
      case XA_WM_NORMAL_HINTS:
         ReadWMNormalHints(np);
         changed = 1;
         break;
      case XA_WM_HINTS:
      case XA_WM_ICON_NAME:
      case XA_WM_CLIENT_MACHINE:
         break;
      default:
         if(event->atom == atoms[ATOM_WM_COLORMAP_WINDOWS]) {
            ReadWMColormaps(np);
            UpdateClientColormap(np);
         } else if(event->atom == atoms[ATOM_NET_WM_ICON]) {
            LoadIcon(np);
            changed = 1;
         } else if(event->atom == atoms[ATOM_NET_WM_NAME]) {
            ReadWMName(np);
            changed = 1;
         } else if(event->atom == atoms[ATOM_NET_WM_STRUT_PARTIAL]) {
            ReadClientStrut(np);
         } else if(event->atom == atoms[ATOM_NET_WM_STRUT]) {
            ReadClientStrut(np);
         }
         break;
      }

      if(changed) {
         DrawBorder(np);
         UpdateTaskBar();
         UpdatePager();
      }
      if(np->state.status & STAT_WMDIALOG) {
         return 0;
      } else {
         return 1;
      }
   }

   return 1;
}

/** Handle a client message. */
void HandleClientMessage(const XClientMessageEvent *event)
{

   ClientNode *np;
#ifdef DEBUG
   char *atomName;
#endif

   np = FindClientByWindow(event->window);
   if(np) {
      if(event->message_type == atoms[ATOM_WM_CHANGE_STATE]) {

         if(np->controller) {
            (np->controller)(0);
         }

         switch(event->data.l[0]) {
         case WithdrawnState:
            SetClientWithdrawn(np);
            break;
         case IconicState:
            MinimizeClient(np);
            break;
         case NormalState:
            RestoreClient(np, 1);
            break;
         default:
            break;
         }

      } else if(event->message_type == atoms[ATOM_NET_ACTIVE_WINDOW]) {

         RestoreClient(np, 1);
         FocusClient(np);

      } else if(event->message_type == atoms[ATOM_NET_WM_DESKTOP]) {

         if(event->data.l[0] == ~0L) {
            SetClientSticky(np, 1);
         } else {

            if(np->controller) {
               (np->controller)(0);
            }

            if(   event->data.l[0] >= 0
               && event->data.l[0] < (long)settings.desktopCount) {
               np->state.status &= ~STAT_STICKY;
               SetClientDesktop(np, event->data.l[0]);
            }
         }

      } else if(event->message_type == atoms[ATOM_NET_CLOSE_WINDOW]) {

         DeleteClient(np);

      } else if(event->message_type == atoms[ATOM_NET_MOVERESIZE_WINDOW]) {

         HandleNetMoveResize(event, np);

      } else if(event->message_type == atoms[ATOM_NET_WM_STATE]) {

         HandleNetWMState(event, np);

      } else if(event->message_type == atoms[ATOM_NET_SHOWING_DESKTOP]) {

         if(event->data.l[0] != showingDesktop) {
            ShowDesktop();
         }

      } else {

#ifdef DEBUG
         atomName = JXGetAtomName(display, event->message_type);
         Debug("Unknown ClientMessage to client: %s", atomName);
         JXFree(atomName);
#endif

      }

   } else if(event->window == rootWindow) {

      if(event->message_type == atoms[ATOM_JWM_RESTART]) {
         Restart();
      } else if(event->message_type == atoms[ATOM_JWM_EXIT]) {
         Exit();
      } else if(event->message_type == atoms[ATOM_JWM_RELOAD]) {
         ReloadMenu();
      } else if(event->message_type == atoms[ATOM_NET_CURRENT_DESKTOP]) {
         ChangeDesktop(event->data.l[0]);
      } else {
#ifdef DEBUG
         atomName = JXGetAtomName(display, event->message_type);
         Debug("Unknown ClientMessage to root: %s", atomName);
         JXFree(atomName);
#endif
      }

   } else if(event->message_type == atoms[ATOM_NET_SYSTEM_TRAY_OPCODE]) {

      HandleDockEvent(event);

   } else {
#ifdef DEBUG
         atomName = JXGetAtomName(display, event->message_type);
         Debug("ClientMessage to unknown window (0x%x): %s",
               event->window, atomName);
         JXFree(atomName);
#endif
   }

}

/** Handle a _NET_MOVERESIZE_WINDOW request. */
void HandleNetMoveResize(const XClientMessageEvent *event, ClientNode *np)
{

   long flags;
   long x, y;
   long width, height;
   int deltax, deltay;
   int north, south, east, west;

   Assert(event);
   Assert(np);

   flags = event->data.l[0] >> 8;

   x = np->x;
   y = np->y;
   width = np->width;
   height = np->height;

   if(flags & (1 << 0)) {
      x = event->data.l[1];
   }
   if(flags & (1 << 1)) {
      y = event->data.l[2];
   }
   if(flags & (1 << 2)) {
      width = event->data.l[3];
   }
   if(flags & (1 << 3)) {
      height = event->data.l[4];
   }

   GetBorderSize(np, &north, &south, &east, &west);
   GetGravityDelta(np, &deltax, &deltay);

   x -= deltax;
   y -= deltay;

   np->x = x;
   np->y = y;
   np->width = width;
   np->height = height;

   if(JUNLIKELY(np->state.status & STAT_FULLSCREEN)) {
      Warning(_("Fullscreen state will be shaped!"));
   }

   /** Reset shaped bound */
   ResetRoundedRectWindow(np);
   ShapeRoundedRectWindow(np->parent, np->width + east + west,
                          np->height + north + south);
   JXMoveResizeWindow(display, np->parent,
                      np->x - west, np->y - north,
                      np->width + east + west,
                      np->height + north + south);
   JXMoveResizeWindow(display, np->window, west, north,
                      np->width, np->height);

   WriteState(np);
   SendConfigureEvent(np);

}

/** Handle a _NET_WM_STATE request. */
void HandleNetWMState(const XClientMessageEvent *event, ClientNode *np)
{

   int actionMaxH;
   int actionMaxV;
   int actionStick;
   int actionShade;
   int actionFullScreen;
   int actionMinimize;
   int actionNolist;
   int actionBelow;
   int actionAbove;
   int x;

   /* Up to two actions to be applied together. */
   actionMaxH = 0;
   actionMaxV = 0;
   actionStick = 0;
   actionShade = 0;
   actionFullScreen = 0;
   actionMinimize = 0;
   actionNolist = 0;
   actionBelow = 0;
   actionAbove = 0;

   for(x = 1; x <= 2; x++) {
      if(event->data.l[x]
         == (long)atoms[ATOM_NET_WM_STATE_STICKY]) {
         actionStick = 1;
      } else if(event->data.l[x]
         == (long)atoms[ATOM_NET_WM_STATE_MAXIMIZED_VERT]) {
         actionMaxV = 1;
      } else if(event->data.l[x]
         == (long)atoms[ATOM_NET_WM_STATE_MAXIMIZED_HORZ]) {
         actionMaxH = 1;
      } else if(event->data.l[x]
         == (long)atoms[ATOM_NET_WM_STATE_SHADED]) {
         actionShade = 1;
      } else if(event->data.l[x]
         == (long)atoms[ATOM_NET_WM_STATE_FULLSCREEN]) {
         actionFullScreen = 1;
      } else if(event->data.l[x]
         == (long)atoms[ATOM_NET_WM_STATE_HIDDEN]) {
         actionMinimize = 1;
      } else if(event->data.l[x]
         == (long)atoms[ATOM_NET_WM_STATE_SKIP_TASKBAR]) {
         actionNolist = 1;
      } else if(event->data.l[x]
         == (long)atoms[ATOM_NET_WM_STATE_BELOW]) {
         actionBelow = 1;
      } else if(event->data.l[x]
         == (long)atoms[ATOM_NET_WM_STATE_ABOVE]) {
         actionAbove = 1;
      }
   }

   switch(event->data.l[0]) {
   case 0: /* Remove */
      if(actionStick) {
         SetClientSticky(np, 0);
      }
      if(actionMaxH || actionMaxV) {
         if(np->state.status & (STAT_HMAX | STAT_VMAX)) {
            MaximizeClient(np, 0, 0);
         }
      }
      if(actionShade) {
         UnshadeClient(np);
      }
      if(actionFullScreen) {
         SetClientFullScreen(np, 0);
      }
      if(actionMinimize) {
         RestoreClient(np, 0);
      }
      if(actionNolist) {
         np->state.status &= ~STAT_NOLIST;
         UpdateTaskBar();
      }
      if(actionBelow && np->state.layer == LAYER_BELOW) {
         SetClientLayer(np, LAYER_NORMAL);
      }
      if(actionAbove && np->state.layer == LAYER_ABOVE) {
         SetClientLayer(np, LAYER_NORMAL);
      }
      break;
   case 1: /* Add */
      if(actionStick) {
         SetClientSticky(np, 1);
      }
      if(!(np->state.status & (STAT_HMAX | STAT_VMAX))) {
         MaximizeClient(np, actionMaxH, actionMaxV);
      }
      if(actionShade) {
         ShadeClient(np);
      }
      if(actionFullScreen) {
         SetClientFullScreen(np, 1);
      }
      if(actionMinimize) {
         MinimizeClient(np);
      }
      if(actionNolist) {
         np->state.status |= STAT_NOLIST;
         UpdateTaskBar();
      }
      if(actionBelow && np->state.layer == LAYER_NORMAL) {
         SetClientLayer(np, LAYER_BELOW);
      }
      if(actionAbove && np->state.layer == LAYER_NORMAL) {
         SetClientLayer(np, LAYER_ABOVE);
      }
      break;
   case 2: /* Toggle */
      if(actionStick) {
         if(np->state.status & STAT_STICKY) {
            SetClientSticky(np, 0);
         } else {
            SetClientSticky(np, 1);
         }
      }
      if(actionMaxH || actionMaxV) {
         MaximizeClient(np, actionMaxH, actionMaxV);
      }
      if(actionShade) {
         if(np->state.status & STAT_SHADED) {
            UnshadeClient(np);
         } else {
            ShadeClient(np);
         }
      }
      if(actionFullScreen) {
         if(np->state.status & STAT_FULLSCREEN) {
            SetClientFullScreen(np, 0);
         } else {
            SetClientFullScreen(np, 1);
         }
      }
      if(actionBelow) {
         if(np->state.layer == LAYER_NORMAL) {
            SetClientLayer(np, LAYER_BELOW);
         } else if(np->state.layer == LAYER_BELOW) {
            SetClientLayer(np, LAYER_NORMAL);
         }
      }
      if(actionAbove) {
         if(np->state.layer == LAYER_NORMAL) {
            SetClientLayer(np, LAYER_ABOVE);
         } else if(np->state.layer == LAYER_ABOVE) {
            SetClientLayer(np, LAYER_NORMAL);
         }
      }
      /* Note that we don't handle toggling of hidden per EWMH
       * recommendations. */
      if(actionNolist) {
         np->state.status ^= STAT_NOLIST;
         UpdateTaskBar();
      }
      break;
   default:
      Debug("bad _NET_WM_STATE action: %ld", event->data.l[0]);
      break;
   }
}

/** Handle a motion notify event. */
void HandleMotionNotify(const XMotionEvent *event)
{

   ClientNode *np;
   Cursor cur;
   BorderActionType action;

   if(event->is_hint) {
      return;
   }

   SetMousePosition(event->x_root, event->y_root);

   np = FindClientByParent(event->window);
   if(np) {
      action = GetBorderActionType(np, event->x, event->y);
      if(np->borderAction != action) {
         np->borderAction = action;
         cur = GetFrameCursor(action);
         JXDefineCursor(display, np->parent, cur);
      }
   }

}

/** Handle a shape event. */
#ifdef USE_SHAPE
void HandleShapeEvent(const XShapeEvent *event)
{
   ClientNode *np;
   np = FindClientByWindow(event->window);
   if(np) {
      SetShape(np);
   }
}
#endif /* USE_SHAPE */

/** Handle a colormap event. */
void HandleColormapChange(const XColormapEvent *event)
{
   ClientNode *np;
   if(event->new == True) {
      np = FindClientByWindow(event->window);
      if(np) {
         np->cmap = event->colormap;
         UpdateClientColormap(np);
      }
   }
}

/** Handle a map request. */
void HandleMapRequest(const XMapEvent *event)
{
   ClientNode *np;
   Assert(event);
   if(CheckSwallowMap(event)) {
      return;
   }
   np = FindClientByWindow(event->window);
   if(!np) {
      JXGrabServer(display);
      JXSync(display, False);
      np = AddClientWindow(event->window, 0, 1);
      if(np) {
         if(     settings.focusModel == FOCUS_CLICK
            && !(np->state.status & STAT_NOFOCUS)) {
            FocusClient(np);
         }
      } else {
         JXMapWindow(display, event->window);
      }
      JXSync(display, False);
      JXUngrabServer(display);
   } else {
      if(!(np->state.status & STAT_MAPPED)) {

         /* Remove from the layer list. */
         if(np->prev != NULL) {
            np->prev->next = np->next;
         } else {
            Assert(nodes[np->state.layer] == np);
            nodes[np->state.layer] = np->next;
         }
         if(np->next != NULL) {
            np->next->prev = np->prev;
         } else {
            Assert(nodeTail[np->state.layer] == np);
            nodeTail[np->state.layer] = np->prev;
         }

         /* Read the state (and new layer). */
         np->state = ReadWindowState(np->window, 0);
         np->state.status |= STAT_MAPPED;

         /* Add to the layer list. */
         np->prev = NULL;
         np->next = nodes[np->state.layer];
         if(np->next == NULL) {
            nodeTail[np->state.layer] = np;
         } else {
            np->next->prev = np;
         }
         nodes[np->state.layer] = np;

         if(!(np->state.status & STAT_STICKY)) {
            np->state.desktop = currentDesktop;
         }
         JXMapWindow(display, np->window);
         JXMapWindow(display, np->parent);
         if(!(np->state.status & STAT_NOFOCUS)) {
            RaiseClient(np);
            FocusClient(np);
         }
         UpdateTaskBar();
         UpdatePager();
      }
   }
   RestackClients();
}

/** Handle an unmap notify event. */
void HandleUnmapNotify(const XUnmapEvent *event)
{
   ClientNode *np;
   XEvent e;

   Assert(event);

   np = FindClientByWindow(event->window);
   if(np && np->window == event->window) {

      /* Grab the server to prevent the client from destroying the
       * window after we check for a DestroyNotify. */
      JXGrabServer(display);
      JXSync(display, False);

      if(np->controller) {
         (np->controller)(1);
      }

      if(JXCheckTypedWindowEvent(display, np->window, DestroyNotify, &e)) {
         UpdateTime(&e);
         RemoveClient(np);
         JXUngrabServer(display);
         return;
      }

      if(np->state.status & STAT_MAPPED) {

         np->state.status &= ~STAT_MAPPED;

         JXUnmapWindow(display, np->parent);
         WriteState(np);

         UpdateTaskBar();
         UpdatePager();

         if(np->state.status & STAT_ACTIVE) {
            FocusNextStacked(np);
         }

      }

      JXUngrabServer(display);

   }
}

/** Handle a destroy notify event. */
char HandleDestroyNotify(const XDestroyWindowEvent *event)
{
   ClientNode *np;
   np = FindClientByWindow(event->window);
   if(np && np->window == event->window) {
      if(np->controller) {
         (np->controller)(1);
      }
      RemoveClient(np);
      return 1;
   } else if(!np) {
      return HandleDockDestroy(event->window);
   }
   return 0;
}

/** Take the appropriate action for a click on a client border. */
void DispatchBorderButtonEvent(const XButtonEvent *event,
                               ClientNode *np)
{

   static Time lastClickTime = 0;
   static int lastX = 0, lastY = 0;
   static char doubleClickActive = 0;
   BorderActionType action;
   int bsize;

   /* Middle click always starts a move. */
   if(event->button == Button2) {
      MoveClient(np, event->x, event->y, (event->state & Mod1Mask) ? 0 : 1);
      return;
   }

   /* Determine the size of the border. */
   if(np->state.border & BORDER_OUTLINE) {
      bsize = settings.borderWidth;
   } else {
      bsize = 0;
   }

   /* Other buttons are context sensitive. */
   action = GetBorderActionType(np, event->x, event->y);
   switch(action & 0x0F) {
   case BA_RESIZE:   /* Border */
      if(event->type == ButtonPress) {
         if(event->button == Button1) {
            ResizeClient(np, action, event->x, event->y);
         } else if(event->button == Button3) {
            ShowWindowMenu(np, np->x + event->x - bsize,
                           np->y + event->y - settings.titleHeight - bsize);
         }
      }
      break;
   case BA_MOVE:     /* Title bar */
      if(event->button == Button1) {
         if(event->type == ButtonPress) {
            if(doubleClickActive
               && abs(event->time - lastClickTime) > 0
               && abs(event->time - lastClickTime) <= settings.doubleClickSpeed
               && abs(event->x - lastX) <= settings.doubleClickDelta
               && abs(event->y - lastY) <= settings.doubleClickDelta) {
               MaximizeClientDefault(np);
               doubleClickActive = 0;
            } else {
               if(MoveClient(np, event->x, event->y,
                             (event->state & Mod1Mask) ? 0 : 1)) {
                  doubleClickActive = 0;
               } else {
                  doubleClickActive = 1;
                  lastClickTime = event->time;
                  lastX = event->x;
                  lastY = event->y;
               }
            }
         }
      } else if(event->button == Button3) {
         ShowWindowMenu(np, np->x + event->x - bsize,
                        np->y + event->y - settings.titleHeight - bsize);
      } else if(event->button == Button4) {
         ShadeClient(np);
      } else if(event->button == Button5) {
         UnshadeClient(np);
      }
      break;
   case BA_MENU:  /* Menu button */
      if(event->button == Button4) {
         ShadeClient(np);
      } else if(event->button == Button5) {
         UnshadeClient(np);
      } else if(event->type == ButtonPress) {
         ShowWindowMenu(np, np->x + event->x - bsize,
                        np->y + event->y - settings.titleHeight - bsize);
      }
      break;
   case BA_CLOSE: /* Close button */
      if(event->type == ButtonRelease) {
         DeleteClient(np);
      }
      break;
   case BA_MAXIMIZE: /* Maximize button */
      if(event->type == ButtonRelease) {
         MaximizeClientDefault(np);
      }
      break;
   case BA_MINIMIZE: /* Minimize button */
      if(event->type == ButtonRelease) {
         if(event->button == Button3) {
            if(np->state.status & STAT_SHADED) {
               UnshadeClient(np);
            } else {
               ShadeClient(np);
            }
         } else {
            MinimizeClient(np);
         }
      }
      break;
   default:
      break;
   }
}

/** Update the last event time. */
void UpdateTime(const XEvent *event)
{
   Time t = CurrentTime;
   Assert(event);
   switch(event->type) {
   case KeyPress:
   case KeyRelease:
      t = event->xkey.time;
      break;
   case ButtonPress:
   case ButtonRelease:
      t = event->xkey.time;
      break;
   case MotionNotify:
      t = event->xmotion.time;
      break;
   case EnterNotify:
   case LeaveNotify:
      t = event->xcrossing.time;
      break;
   case PropertyNotify:
      t = event->xproperty.time;
      break;
   case SelectionClear:
      t = event->xselectionclear.time;
      break;
   case SelectionRequest:
      t = event->xselectionrequest.time;
      break;
   case SelectionNotify:
      t = event->xselection.time;
      break;
   default:
      break;
   }
   if(t != CurrentTime) {
      if(t > eventTime || t < eventTime - 60000) {
         eventTime = t;
      }
   }
}

