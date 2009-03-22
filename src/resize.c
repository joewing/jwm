/**
 * @file resize.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions to handle resizing client windows.
 *
 */

#include "jwm.h"
#include "resize.h"
#include "client.h"
#include "outline.h"
#include "main.h"
#include "cursor.h"
#include "misc.h"
#include "pager.h"
#include "status.h"
#include "key.h"
#include "event.h"
#include "border.h"

static ResizeModeType resizeMode = RESIZE_OPAQUE;

static int shouldStopResize;

static void StopResize(ClientNode *np);
static void ResizeController(int wasDestroyed);
static void FixWidth(ClientNode *np);
static void FixHeight(ClientNode *np);

/** Set the resize mode to use. */
void SetResizeMode(ResizeModeType mode) {
   resizeMode = mode;
}

/** Callback to stop a resize. */
void ResizeController(int wasDestroyed) {
   if(resizeMode == RESIZE_OUTLINE) {
      ClearOutline();
   }
   JXUngrabPointer(display, CurrentTime);
   JXUngrabKeyboard(display, CurrentTime);
   DestroyResizeWindow();
   shouldStopResize = 1;
}

/** Resize a client window (mouse initiated). */
void ResizeClient(ClientNode *np, BorderActionType action,
   int startx, int starty) {

   XEvent event;
   int oldx, oldy;
   int oldw, oldh;
   int gwidth, gheight;
   int lastgwidth, lastgheight;
   int delta;
   int north, south, east, west;
   float ratio, minr, maxr;

   Assert(np);

   if(!(np->state.border & BORDER_RESIZE)) {
      return;
   }

   if(!GrabMouseForResize(action)) {
      Debug("ResizeClient: could not grab mouse");
      return;
   }

   if(np->state.status & STAT_SHADED) {
      action &= ~(BA_RESIZE_N | BA_RESIZE_S);
   }

   np->controller = ResizeController;
   shouldStopResize = 0;

   oldx = np->x;
   oldy = np->y;
   oldw = np->width;
   oldh = np->height;

   gwidth = (np->width - np->baseWidth) / np->xinc;
   gheight = (np->height - np->baseHeight) / np->yinc;

   GetBorderSize(np, &north, &south, &east, &west);

   startx += np->x - west;
   starty += np->y - north;

   CreateResizeWindow(np);
   UpdateResizeWindow(np, gwidth, gheight);

   if(!(GetMouseMask() & (Button1Mask | Button3Mask))) {
      StopResize(np);
      return;
   }

   for(;;) {

      WaitForEvent(&event);

      if(shouldStopResize) {
         np->controller = NULL;
         return;
      }

      switch(event.type) {
      case ButtonRelease:
         if(   event.xbutton.button == Button1
            || event.xbutton.button == Button3) {
            StopResize(np);
            return;
         }
         break;
      case MotionNotify:

         SetMousePosition(event.xmotion.x_root, event.xmotion.y_root);
         DiscardMotionEvents(&event, np->window);

         if(action & BA_RESIZE_N) {
            delta = (event.xmotion.y - starty) / np->yinc;
            delta *= np->yinc;
            if(oldh - delta >= np->minHeight
               && (oldh - delta <= np->maxHeight || delta > 0)) {
               np->height = oldh - delta;
               np->y = oldy + delta;
            }
            if(!(action & (BA_RESIZE_E | BA_RESIZE_W))) {
               FixWidth(np);
            }
         }
         if(action & BA_RESIZE_S) {
            delta = (event.xmotion.y - starty) / np->yinc;
            delta *= np->yinc;
            np->height = oldh + delta;
            np->height = Max(np->height, np->minHeight);
            np->height = Min(np->height, np->maxHeight);
            if(!(action & (BA_RESIZE_E | BA_RESIZE_W))) {
               FixWidth(np);
            }
         }
         if(action & BA_RESIZE_E) {
            delta = (event.xmotion.x - startx) / np->xinc;
            delta *= np->xinc;
            np->width = oldw + delta;
            np->width = Max(np->width, np->minWidth);
            np->width = Min(np->width, np->maxWidth);
            if(!(action & (BA_RESIZE_N | BA_RESIZE_S))) {
               FixHeight(np);
            }
         }
         if(action & BA_RESIZE_W) {
            delta = (event.xmotion.x - startx) / np->xinc;
            delta *= np->xinc;
            if(oldw - delta >= np->minWidth
               && (oldw - delta <= np->maxWidth || delta > 0)) {
               np->width = oldw - delta;
               np->x = oldx + delta;
            }
            if(!(action & (BA_RESIZE_N | BA_RESIZE_S))) {
               FixHeight(np);
            }
         }

         if(np->sizeFlags & PAspect) {
            if((action & (BA_RESIZE_N | BA_RESIZE_S)) &&
               (action & (BA_RESIZE_E | BA_RESIZE_W))) {

               ratio = (float)np->width / np->height;

               minr = (float)np->aspect.minx / np->aspect.miny;
               if(ratio < minr) {
                  delta = np->width;
                  np->width = (int)((float)np->height * minr);
                  if(action & BA_RESIZE_W) {
                     np->x -= np->width - delta;
                  }
               }

               maxr = (float)np->aspect.maxx / np->aspect.maxy;
               if(ratio > maxr) {
                  delta = np->height;
                  np->height = (int)((float)np->width / maxr);
                  if(action & BA_RESIZE_N) {
                     np->y -= np->height - delta;
                  }
               }

            }
         }

         lastgwidth = gwidth;
         lastgheight = gheight;

         gwidth = (np->width - np->baseWidth) / np->xinc;
         gheight = (np->height - np->baseHeight) / np->yinc;

         if(lastgheight != gheight || lastgwidth != gwidth) {

            if(np->state.status & (STAT_HMAX | STAT_VMAX)) {
               np->state.status &= ~(STAT_HMAX | STAT_VMAX);
               WriteState(np);
               SendConfigureEvent(np);
            }

            UpdateResizeWindow(np, gwidth, gheight);

            if(resizeMode == RESIZE_OUTLINE) {
               ClearOutline();
               if(np->state.status & STAT_SHADED) {
                  DrawOutline(np->x - west, np->y - north,
                     np->width + west + east, north + south);
               } else {
                  DrawOutline(np->x - west, np->y - north,
                     np->width + west + east,
                     np->height + north + south);
               }
            } else {
               ResetRoundedRectWindow(np->parent);
               if(np->state.status & STAT_SHADED) {
                  ShapeRoundedRectWindow(np->parent, 
                     np->width + east + west,
                     north + south);
                  JXMoveResizeWindow(display, np->parent,
                     np->x - west, np->y - north,
                     np->width + west + east, north + south);
               } else {
                  ShapeRoundedRectWindow(np->parent, 
                     np->width + east + west,
                     np->height + north + south);
                  JXMoveResizeWindow(display, np->parent,
                     np->x - west, np->y - north,
                     np->width + west + east,
                     np->height + north + south);
               }
               JXMoveResizeWindow(display, np->window, west,
                  north, np->width, np->height);
               SendConfigureEvent(np);
            }

            UpdatePager();

         }

         break;
      default:
         break;
      }
   }

}

/** Resize a client window (keyboard or menu initiated). */
void ResizeClientKeyboard(ClientNode *np) {

   XEvent event;
   int gwidth, gheight;
   int lastgwidth, lastgheight;
   int north, south, east, west;
   int deltax, deltay;
   float ratio, minr, maxr;

   Assert(np);

   if(!(np->state.border & BORDER_RESIZE)) {
      return;
   }

   if(JXGrabKeyboard(display, np->window, True, GrabModeAsync,
      GrabModeAsync, CurrentTime) != GrabSuccess) {
      Debug("ResizeClientKeyboard: could not grab keyboard");
      return;
   }
   GrabMouseForResize(BA_RESIZE_S | BA_RESIZE_E | BA_RESIZE);

   np->controller = ResizeController;
   shouldStopResize = 0;

   gwidth = (np->width - np->baseWidth) / np->xinc;
   gheight = (np->height - np->baseHeight) / np->yinc;

   GetBorderSize(np, &north, &south, &east, &west);

   CreateResizeWindow(np);
   UpdateResizeWindow(np, gwidth, gheight);

   MoveMouse(rootWindow, np->x + np->width, np->y + np->height);
   DiscardMotionEvents(&event, np->window);

   for(;;) {

      WaitForEvent(&event);

      if(shouldStopResize) {
         np->controller = NULL;
         return;
      }

      deltax = 0;
      deltay = 0;

      if(event.type == KeyPress) {

         while(JXCheckTypedWindowEvent(display, np->window, KeyPress, &event));

         switch(GetKey(&event.xkey) & 0xFF) {
         case KEY_UP:
            deltay = Min(-np->yinc, -10);
            break;
         case KEY_DOWN:
            deltay = Max(np->yinc, 10);
            break;
         case KEY_RIGHT:
            deltax = Max(np->xinc, 10);
            break;
         case KEY_LEFT:
            deltax = Min(-np->xinc, -10);
            break;
         default:
            StopResize(np);
            return;
         }

      } else if(event.type == MotionNotify) {

         SetMousePosition(event.xmotion.x_root, event.xmotion.y_root);
         DiscardMotionEvents(&event, np->window);

         deltax = event.xmotion.x - (np->x + np->width);
         deltay = event.xmotion.y - (np->y + np->height);

      } else if(event.type == ButtonRelease) {

         StopResize(np);
         return;

      }

      if(abs(deltax) < np->xinc && abs(deltay) < np->yinc) {
         continue;
      }

      deltay -= deltay % np->yinc;
      np->height += deltay;
      np->height = Max(np->height, np->minHeight);
      np->height = Min(np->height, np->maxHeight);
      deltax -= deltax % np->xinc;
      np->width += deltax;
      np->width = Max(np->width, np->minWidth);
      np->width = Min(np->width, np->maxWidth);

      if(np->sizeFlags & PAspect) {

         ratio = (float)np->width / np->height;

         minr = (float)np->aspect.minx / np->aspect.miny;
         if(ratio < minr) {
            np->width = (int)((float)np->height * minr);
         }

         maxr = (float)np->aspect.maxx / np->aspect.maxy;
         if(ratio > maxr) {
            np->height = (int)((float)np->width / maxr);
         }

      }

      lastgwidth = gwidth;
      lastgheight = gheight;
      gwidth = (np->width - np->baseWidth) / np->xinc;
      gheight = (np->height - np->baseHeight) / np->yinc;

      if(lastgwidth != gwidth || lastgheight != gheight) {

         if(np->state.status & (STAT_HMAX | STAT_VMAX)) {
            np->state.status &= ~(STAT_HMAX | STAT_VMAX);
            WriteState(np);
            SendConfigureEvent(np);
         }

         UpdateResizeWindow(np, gwidth, gheight);

         if(resizeMode == RESIZE_OUTLINE) {
            ClearOutline();
            if(np->state.status & STAT_SHADED) {
               DrawOutline(np->x - west, np->y - north,
                  np->width + west + east,
                  north + south);
            } else {
               DrawOutline(np->x - west, np->y - north,
                  np->width + west + east,
                  np->height + north + south);
            }
         } else {
            ResetRoundedRectWindow(np->parent);
            if(np->state.status & STAT_SHADED) {
               ShapeRoundedRectWindow(np->parent, 
                  np->width + east + west,
                  north + south);
              JXResizeWindow(display, np->parent,
                  np->width + west + east, north + south);
            } else {
               ShapeRoundedRectWindow(np->parent, 
                  np->width + east + west,
                  np->height + north + south);
               JXResizeWindow(display, np->parent,
                  np->width + west + east, np->height + north + south);
            }
            JXResizeWindow(display, np->window, np->width, np->height);
            SendConfigureEvent(np);
         }

         UpdatePager();

      }

   }

}

/** Stop a resize action. */
void StopResize(ClientNode *np) {

   int north, south, east, west;

   np->controller = NULL;

   if(resizeMode == RESIZE_OUTLINE) {
      ClearOutline();
   }

   JXUngrabPointer(display, CurrentTime);
   JXUngrabKeyboard(display, CurrentTime);

   DestroyResizeWindow();

   GetBorderSize(np, &north, &south, &east, &west);

   /* Reset shaped bound */
   ResetRoundedRectWindow(np->parent);
	  
   if(np->state.status & STAT_SHADED) {
      ShapeRoundedRectWindow(np->parent, 
         np->width + east + west, north + south);
      JXMoveResizeWindow(display, np->parent,
         np->x - west, np->y - north,
         np->width + east + west, north + south);
   } else {
      ShapeRoundedRectWindow(np->parent, 
         np->width + east + west, 
         np->height + north + south);
      JXMoveResizeWindow(display, np->parent,
         np->x - west, np->y - north,
         np->width + east + west,
         np->height + north + south);
   }
   JXMoveResizeWindow(display, np->window, west,
      north, np->width, np->height);
   SendConfigureEvent(np);

}

/** Fix the width to match the aspect ratio. */
void FixWidth(ClientNode *np) {

   float ratio, minr, maxr;

   Assert(np);

   if((np->sizeFlags & PAspect) && np->height > 0) {

      ratio = (float)np->width / np->height;

      minr = (float)np->aspect.minx / np->aspect.miny;
      if(ratio < minr) {
         np->width = (int)((float)np->height * minr);
      }

      maxr = (float)np->aspect.maxx / np->aspect.maxy;
      if(ratio > maxr) {
         np->width = (int)((float)np->height * maxr);
      }

   }

}

/** Fix the height to match the aspect ratio. */
void FixHeight(ClientNode *np) {

   float ratio, minr, maxr;

   Assert(np);

   if((np->sizeFlags & PAspect) && np->height > 0) {

      ratio = (float)np->width / np->height;

      minr = (float)np->aspect.minx / np->aspect.miny;
      if(ratio < minr) {
         np->height = (int)((float)np->width / minr);
      }

      maxr = (float)np->aspect.maxx / np->aspect.maxy;
      if(ratio > maxr) {
         np->height = (int)((float)np->width / maxr);
      }

   }

}

