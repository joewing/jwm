/**
 * Functions for dealing with window borders.
 * Copyright (C) 2004 Joe Wingbermuehle
 * 
 */

#include "jwm.h"
#include "border.h"
#include "client.h"
#include "clientlist.h"
#include "color.h"
#include "main.h"
#include "icon.h"
#include "font.h"
#include "error.h"
#include "misc.h"
#include "settings.h"
#include "grab.h"

static GC borderGC;

static void DrawBorderHelper(const ClientNode *np);
static void DrawBorderButtons(const ClientNode *np, Pixmap canvas);
static void DrawCloseButton(unsigned int offset, Pixmap canvas);
static void DrawMaxIButton(unsigned int offset, Pixmap canvas);
static void DrawMaxAButton(unsigned int offset, Pixmap canvas);
static void DrawMinButton(unsigned int offset, Pixmap canvas);
static unsigned int GetButtonCount(const ClientNode *np);

#ifdef USE_SHAPE
static void FillRoundedRectangle(Drawable d, GC gc, int x, int y,
                                 int width, int height, int radius);
#endif

/** Initialize server resources. */
void StartupBorders()
{

   XGCValues gcValues;
   unsigned long gcMask;

   gcMask = GCGraphicsExposures;
   gcValues.graphics_exposures = False;
   borderGC = JXCreateGC(display, rootWindow, gcMask, &gcValues);

}

/** Release server resources. */
void ShutdownBorders()
{
   JXFreeGC(display, borderGC);
}

/** Get the size of the icon to display on a window. */
int GetBorderIconSize()
{
   return settings.titleHeight - 6;
}

/** Determine the border action to take given coordinates. */
BorderActionType GetBorderActionType(const ClientNode *np, int x, int y)
{

   int north, south, east, west;
   int offset;
   unsigned int resizeMask;
   const unsigned int titleHeight = settings.titleHeight;

   Assert(np);

   GetBorderSize(&np->state, &north, &south, &east, &west);

   /* Check title bar actions. */
   if((np->state.border & BORDER_TITLE) &&
      settings.titleHeight > settings.borderWidth) {

      /* Check buttons on the title bar. */
      offset = np->width + west;
      if(y >= settings.borderWidth && y <= titleHeight) {

         /* Menu button. */
         if(np->width >= titleHeight) {
            if(x > settings.borderWidth && x <= titleHeight) {
               return BA_MENU;
            }
         }

         /* Close button. */
         if((np->state.border & BORDER_CLOSE) && offset > 2 * titleHeight) {
            if(x > offset - titleHeight && x < offset) {
               return BA_CLOSE;
            }
            offset -= titleHeight;
         }

         /* Maximize button. */
         if((np->state.border & BORDER_MAX) && offset > 2 * titleHeight) {
            if(x > offset - titleHeight && x < offset) {
               return BA_MAXIMIZE;
            }
            offset -= titleHeight;
         }

         /* Minimize button. */
         if((np->state.border & BORDER_MIN) && offset > 2 * titleHeight) {
            if(x > offset - titleHeight && x < offset) {
               return BA_MINIMIZE;
            }
         }

      }

      /* Check for move. */
      if(y >= settings.borderWidth && y <= titleHeight) {
         if(x > settings.borderWidth && x < offset) {
            if(np->state.border & BORDER_MOVE) {
               return BA_MOVE;
            } else {
               return BA_NONE;
            }
         }
      }

   }

   /* Now we check resize actions.
    * There is no need to go further if resizing isn't allowed. */
   if(!(np->state.border & BORDER_RESIZE)) {
      return BA_NONE;
   }

   /* We don't allow resizing maximized windows. */
   resizeMask = BA_RESIZE_S | BA_RESIZE_N
              | BA_RESIZE_E | BA_RESIZE_W
              | BA_RESIZE;
   if(np->state.status & STAT_HMAX) {
      resizeMask &= ~(BA_RESIZE_E | BA_RESIZE_W);
   }
   if(np->state.status & STAT_VMAX) {
      resizeMask &= ~(BA_RESIZE_N | BA_RESIZE_S);
   }
   if(np->state.status & STAT_SHADED) {
      resizeMask &= ~(BA_RESIZE_N | BA_RESIZE_S);
   }

   /* Check south east/west and north east/west resizing. */
   if(   np->width >= settings.titleHeight * 2
      && np->height >= settings.titleHeight * 2) {
      if(y > np->height + north - settings.titleHeight) {
         if(x < settings.titleHeight) {
            return (BA_RESIZE_S | BA_RESIZE_W | BA_RESIZE) & resizeMask;
         } else if(x > np->width + west - settings.titleHeight) {
            return (BA_RESIZE_S | BA_RESIZE_E | BA_RESIZE) & resizeMask;
         }
      } else if(y < settings.titleHeight) {
         if(x < settings.titleHeight) {
            return (BA_RESIZE_N | BA_RESIZE_W | BA_RESIZE) & resizeMask;
         } else if(x > np->width + west - settings.titleHeight) {
            return (BA_RESIZE_N | BA_RESIZE_E | BA_RESIZE) & resizeMask;
         }
      }
   }

   /* Check east, west, north, and south resizing. */
   if(x <= west) {
      return (BA_RESIZE_W | BA_RESIZE) & resizeMask;
   } else if(x >= np->width + west) {
      return (BA_RESIZE_E | BA_RESIZE) & resizeMask;
   } else if(y >= np->height + north) {
      return (BA_RESIZE_S | BA_RESIZE) & resizeMask;
   } else if(y <= south) {
      return (BA_RESIZE_N | BA_RESIZE) & resizeMask;
   } else {
      return BA_NONE;
   }

}

/** Reset the shape of a window border. */
void ResetBorder(const ClientNode *np)
{
#ifdef USE_SHAPE
   Pixmap shapePixmap;
   GC shapeGC;
#endif

   int north, south, east, west;
   int width, height;

   GrabServer();

   /* Determine the size of the window. */
   GetBorderSize(&np->state, &north, &south, &east, &west);
   width = np->width + east + west;
   if(np->state.status & STAT_SHADED) {
      height = north;
   } else {
      height = np->height + north + south;
   }

   /** Set the window size. */
   if(!(np->state.status & STAT_SHADED)) {
      JXMoveResizeWindow(display, np->window, west, north,
                         np->width, np->height);
   }
   JXMoveResizeWindow(display, np->parent, np->x - west, np->y - north,
                      width, height);

#ifdef USE_SHAPE

   /* First set the shape to the window border. */
   shapePixmap = JXCreatePixmap(display, np->parent, width, height, 1);
   shapeGC = JXCreateGC(display, shapePixmap, 0, NULL);

   /* Make the whole area transparent. */
   JXSetForeground(display, shapeGC, 0);
   JXFillRectangle(display, shapePixmap, shapeGC, 0, 0, width, height);

   /* Draw the window area without the corners. */
   /* Corner bound radius -1 to allow slightly better outline drawing */
   JXSetForeground(display, shapeGC, 1);
   FillRoundedRectangle(shapePixmap, shapeGC, 0, 0, width, height,
                        CORNER_RADIUS - 1);

   /* Apply the client window. */
   if(!(np->state.status & STAT_SHADED) && (np->state.status & STAT_SHAPED)) {

      XRectangle *rects;
      int count;
      int ordering;
      int i;

      /* Cut out an area for the client window. */
      JXSetForeground(display, shapeGC, 0);
      JXFillRectangle(display, shapePixmap, shapeGC, west, north,
                      np->width, np->height);

      /* Fill in the visible area. */
      rects = JXShapeGetRectangles(display, np->window, ShapeBounding,
                                   &count, &ordering);
      if(JLIKELY(rects)) {
         for(i = 0; i < count; i++) {
            rects[i].x += east;
            rects[i].y += north;
         }
         JXSetForeground(display, shapeGC, 1);
         JXFillRectangles(display, shapePixmap, shapeGC, rects, count);
         JXFree(rects);
      }

   }

   /* Set the shape. */
   JXShapeCombineMask(display, np->parent, ShapeBounding, 0, 0,
                      shapePixmap, ShapeSet);

   JXFreeGC(display, shapeGC);
   JXFreePixmap(display, shapePixmap);

#endif

   UngrabServer();

}

/** Draw a client border. */
void DrawBorder(const ClientNode *np)
{

   Assert(np);

   /* Don't draw any more if we are shutting down. */
   if(JUNLIKELY(shouldExit)) {
      return;
   }

   /* Must be either mapped or shaded to have a border. */
   if(!(np->state.status & (STAT_MAPPED | STAT_SHADED))) {
      return;
   }

   /* Hidden and fullscreen windows don't get borders. */
   if(np->state.status & (STAT_HIDDEN | STAT_FULLSCREEN)) {
      return;
   }

   /* Return if there is no border. */
   if(!(np->state.border & (BORDER_TITLE | BORDER_OUTLINE))) {
      return;
   }

   /* Do the actual drawing. */
   DrawBorderHelper(np);

}

/** Helper method for drawing borders. */
void DrawBorderHelper(const ClientNode *np)
{

   ColorType borderTextColor;

   long titleColor1, titleColor2;
   long outlineColor;

   int north, south, east, west;
   unsigned int width, height;
   int iconSize;

   unsigned int buttonCount;
   int titleWidth;
   Pixmap canvas;

   Assert(np);

   iconSize = GetBorderIconSize();
   GetBorderSize(&np->state, &north, &south, &east, &west);
   width = np->width + east + west;
   height = np->height + north + south;

   /* Determine the colors and gradients to use. */
   if(np->state.status & (STAT_ACTIVE | STAT_FLASH)) {

      borderTextColor = COLOR_TITLE_ACTIVE_FG;
      titleColor1 = colors[COLOR_TITLE_ACTIVE_BG1];
      titleColor2 = colors[COLOR_TITLE_ACTIVE_BG2];
      outlineColor = colors[COLOR_BORDER_ACTIVE_LINE];

   } else {

      borderTextColor = COLOR_TITLE_FG;
      titleColor1 = colors[COLOR_TITLE_BG1];
      titleColor2 = colors[COLOR_TITLE_BG2];
      outlineColor = colors[COLOR_BORDER_LINE];

   }

   /* Set parent background to reduce flicker. */
   JXSetWindowBackground(display, np->parent, titleColor2);

   canvas = JXCreatePixmap(display, np->parent, width, north, rootDepth);

   /* Clear the window with the right color. */
   JXSetForeground(display, borderGC, titleColor2);
   JXFillRectangle(display, canvas, borderGC, 0, 0, width, north);

   /* Determine how many pixels may be used for the title. */
   buttonCount = GetButtonCount(np);
   titleWidth = width;
   titleWidth -= settings.titleHeight * buttonCount;
   titleWidth -= iconSize + 7 + 6;

   /* Draw the top part (either a title or north border). */
   if((np->state.border & BORDER_TITLE) &&
      settings.titleHeight > settings.borderWidth) {

      /* Draw a title bar. */
      DrawHorizontalGradient(canvas, borderGC, titleColor1, titleColor2,
                             1, 1, width - 2, settings.titleHeight - 2);

      /* Draw the icon. */
      if(np->icon && np->width >= settings.titleHeight) {
         PutIcon(np->icon, canvas, 6, (settings.titleHeight - iconSize) / 2,
                 iconSize, iconSize);
      }

      if(np->name && np->name[0] && titleWidth > 0) {
         const int sheight = GetStringHeight(FONT_BORDER);
         RenderString(canvas, FONT_BORDER, borderTextColor,
                      iconSize + 6 + 4,
                      (settings.titleHeight - sheight) / 2,
                      titleWidth, np->name);
      }

      DrawBorderButtons(np, canvas);

   }


   /* Copy the title bar to the window. */
   JXCopyArea(display, canvas, np->parent, borderGC, 1, 1,
              width - 2, north - 1, 1, 1);

   /* Window outline.
    * These are drawn directly to the window.
    */
   JXClearArea(display, np->parent, 1, north,
               width - 2, height - north - 1, False);
   JXSetForeground(display, borderGC, outlineColor);
   if(np->state.status & STAT_SHADED) {
      DrawRoundedRectangle(np->parent, borderGC, 0, 0, width - 1, north - 1,
                           CORNER_RADIUS);
   } else {
      DrawRoundedRectangle(np->parent, borderGC, 0, 0, width - 1, height - 1,
                           CORNER_RADIUS);
   }

   JXFreePixmap(display, canvas);

}

/** Determine the number of buttons to be displayed for a client. */
unsigned int GetButtonCount(const ClientNode *np)
{

   int north, south, east, west;
   unsigned int count;
   int offset;

   if(!(np->state.border & BORDER_TITLE)) {
      return 0;
   }
   if(settings.titleHeight <= settings.borderWidth) {
      return 0;
   }

   GetBorderSize(&np->state, &north, &south, &east, &west);

   offset = np->width + west;
   if(offset <= 2 * settings.titleHeight) {
      return 0;
   }

   count = 0;
   if(np->state.border & BORDER_CLOSE) {
      count += 1;
      if(offset <= 2 * settings.titleHeight) {
         return count;
      }
      offset -= settings.titleHeight;
   }

   if(np->state.border & BORDER_MAX) {
      count += 1;
      if(offset < 2 * settings.titleHeight) {
         return count;
      }
   }

   if(np->state.border & BORDER_MIN) {
      count += 1;
   }

   return count;

}

/** Draw the buttons on a client frame. */
void DrawBorderButtons(const ClientNode *np, Pixmap canvas)
{

   long color;
   int offset;
   int north, south, east, west;

   Assert(np);

   GetBorderSize(&np->state, &north, &south, &east, &west);
   offset = np->width + west - settings.titleHeight;
   if(offset <= settings.titleHeight) {
      return;
   }

   /* Determine the colors to use. */
   if(np->state.status & (STAT_ACTIVE | STAT_FLASH)) {
      color = colors[COLOR_TITLE_ACTIVE_FG];
   } else {
      color = colors[COLOR_TITLE_FG];
   }
   JXSetForeground(display, borderGC, color);

   /* Close button. */
   if(np->state.border & BORDER_CLOSE) {
      DrawCloseButton(offset, canvas);
      offset -= settings.titleHeight;
      if(offset <= settings.titleHeight) {
         return;
      }
   }

   /* Maximize button. */
   if(np->state.border & BORDER_MAX) {
      if(np->state.status & (STAT_HMAX | STAT_VMAX)) {
         DrawMaxAButton(offset, canvas);
      } else {
         DrawMaxIButton(offset, canvas);
      }
      offset -= settings.titleHeight;
      if(offset <= settings.titleHeight) {
         return;
      }
   }

   /* Minimize button. */
   if(np->state.border & BORDER_MIN) {
      DrawMinButton(offset, canvas);
   }

}

/** Draw a close button. */
void DrawCloseButton(unsigned int offset, Pixmap canvas)
{
   XSegment segments[2];
   unsigned int size;
   unsigned int x1, y1;
   unsigned int x2, y2;

   size = (settings.titleHeight + 2) / 3;
   x1 = offset + settings.titleHeight / 2 - size / 2;
   y1 = settings.titleHeight / 2 - size / 2;
   x2 = x1 + size;
   y2 = y1 + size;

   segments[0].x1 = x1;
   segments[0].y1 = y1;
   segments[0].x2 = x2;
   segments[0].y2 = y2;

   segments[1].x1 = x2;
   segments[1].y1 = y1;
   segments[1].x2 = x1;
   segments[1].y2 = y2;

   JXSetLineAttributes(display, borderGC, 2, LineSolid,
                       CapProjecting, JoinBevel);
   JXDrawSegments(display, canvas, borderGC, segments, 2);
   JXSetLineAttributes(display, borderGC, 1, LineSolid,
                       CapNotLast, JoinMiter);

}

/** Draw an inactive maximize button. */
void DrawMaxIButton(unsigned int offset, Pixmap canvas)
{

   XSegment segments[5];
   unsigned int size;
   unsigned int x1, y1;
   unsigned int x2, y2;

   size = 2 + (settings.titleHeight + 2) / 3;
   x1 = offset + settings.titleHeight / 2 - size / 2;
   y1 = settings.titleHeight / 2 - size / 2;
   x2 = x1 + size;
   y2 = y1 + size;

   segments[0].x1 = x1;
   segments[0].y1 = y1;
   segments[0].x2 = x1 + size;
   segments[0].y2 = y1;

   segments[1].x1 = x1;
   segments[1].y1 = y1 + 1;
   segments[1].x2 = x1 + size;
   segments[1].y2 = y1 + 1;

   segments[2].x1 = x1;
   segments[2].y1 = y1;
   segments[2].x2 = x1;
   segments[2].y2 = y2;

   segments[3].x1 = x2;
   segments[3].y1 = y1;
   segments[3].x2 = x2;
   segments[3].y2 = y2;

   segments[4].x1 = x1;
   segments[4].y1 = y2;
   segments[4].x2 = x2;
   segments[4].y2 = y2;

   JXSetLineAttributes(display, borderGC, 1, LineSolid,
                       CapProjecting, JoinMiter);
   JXDrawSegments(display, canvas, borderGC, segments, 5);
   JXSetLineAttributes(display, borderGC, 1, LineSolid,
                       CapButt, JoinMiter);

}

/** Draw an active maximize button. */
void DrawMaxAButton(unsigned int offset, Pixmap canvas)
{
   XSegment segments[8];
   unsigned int size;
   unsigned int x1, y1;
   unsigned int x2, y2;
   unsigned int x3, y3;

   size = 2 + (settings.titleHeight + 2) / 3;
   x1 = offset + settings.titleHeight / 2 - size / 2;
   y1 = settings.titleHeight / 2 - size / 2;
   x2 = x1 + size;
   y2 = y1 + size;
   x3 = x1 + size / 2;
   y3 = y1 + size / 2;

   segments[0].x1 = x1;
   segments[0].y1 = y1;
   segments[0].x2 = x2;
   segments[0].y2 = y1;

   segments[1].x1 = x1;
   segments[1].y1 = y1 + 1;
   segments[1].x2 = x2;
   segments[1].y2 = y1 + 1;

   segments[2].x1 = x1;
   segments[2].y1 = y1;
   segments[2].x2 = x1;
   segments[2].y2 = y2;

   segments[3].x1 = x2;
   segments[3].y1 = y1;
   segments[3].x2 = x2;
   segments[3].y2 = y2;

   segments[4].x1 = x1;
   segments[4].y1 = y2;
   segments[4].x2 = x2;
   segments[4].y2 = y2;

   segments[5].x1 = x1;
   segments[5].y1 = y3;
   segments[5].x2 = x3;
   segments[5].y2 = y3;

   segments[6].x1 = x1;
   segments[6].y1 = y3 + 1;
   segments[6].x2 = x3;
   segments[6].y2 = y3 + 1;

   segments[7].x1 = x3;
   segments[7].y1 = y3;
   segments[7].x2 = x3;
   segments[7].y2 = y2;

   JXSetLineAttributes(display, borderGC, 1, LineSolid,
                       CapProjecting, JoinMiter);
   JXDrawSegments(display, canvas, borderGC, segments, 8);
   JXSetLineAttributes(display, borderGC, 1, LineSolid,
                       CapButt, JoinMiter);
}

/** Draw a minimize button. */
void DrawMinButton(unsigned int offset, Pixmap canvas)
{
   unsigned int size;
   unsigned int x1, y1;
   unsigned int x2, y2;
   size = (settings.titleHeight + 2) / 3;
   x1 = offset + settings.titleHeight / 2 - size / 2;
   y1 = settings.titleHeight / 2 - size / 2;
   x2 = x1 + size;
   y2 = y1 + size;
   JXSetLineAttributes(display, borderGC, 2, LineSolid,
                       CapProjecting, JoinMiter);
   JXDrawLine(display, canvas, borderGC, x1, y2, x2, y2);
   JXSetLineAttributes(display, borderGC, 1, LineSolid, CapButt, JoinMiter);
}

/** Redraw the borders on the current desktop.
 * This should be done after loading clients since the stacking order
 * may cause borders on the current desktop to become visible after moving
 * clients to their assigned desktops.
 */
void ExposeCurrentDesktop()
{

   ClientNode *np;
   int layer;

   for(layer = 0; layer < LAYER_COUNT; layer++) {
      for(np = nodes[layer]; np; np = np->next) {
         if(!(np->state.status & (STAT_HIDDEN | STAT_MINIMIZED))) {
            DrawBorder(np);
         }
      }
   }

}

/** Get the size of the borders for a client. */
void GetBorderSize(const ClientState *state,
                   int *north, int *south, int *east, int *west)
{

   Assert(state);
   Assert(north);
   Assert(south);
   Assert(east);
   Assert(west);

   /* Full screen is a special case. */
   if(state->status & STAT_FULLSCREEN) {
      *north = 0;
      *south = 0;
      *east = 0;
      *west = 0;
      return;
   }

   if(state->border & BORDER_OUTLINE) {

      *north = settings.borderWidth;
      *south = settings.borderWidth;
      *east = settings.borderWidth;
      *west = settings.borderWidth;

   } else {

      *north = 0;
      *south = 0;
      *east = 0;
      *west = 0;

   }

   if(state->border & BORDER_TITLE) {
      *north = settings.titleHeight;
   }

   if(state->status & STAT_SHADED) {
      *south = 0;
   }

}

/** Draw a rounded rectangle. */
void DrawRoundedRectangle(Drawable d, GC gc, int x, int y,
                          int width, int height, int radius)
{
#ifdef USE_SHAPE
#ifdef USE_XMU

   XmuDrawRoundedRectangle(display, d, gc, x, y, width, height,
                           radius, radius);

#else

   XSegment segments[4];
   XArc     arcs[4];

   segments[0].x1 = x + radius;           segments[0].y1 = y;
   segments[0].x2 = x + width - radius;   segments[0].y2 = y;
   segments[1].x1 = x + radius;           segments[1].y1 = y + height;
   segments[1].x2 = x + width - radius;   segments[1].y2 = y + height;
   segments[2].x1 = x;                    segments[2].y1 = y + radius;
   segments[2].x2 = x;                    segments[2].y2 = y + height - radius;
   segments[3].x1 = x + width;            segments[3].y1 = y + radius;
   segments[3].x2 = x + width;            segments[3].y2 = y + height - radius;
   JXDrawSegments(display, d, gc, segments, 4);

   arcs[0].x = x;
   arcs[0].y = y;
   arcs[0].width = radius * 2;
   arcs[0].height = radius * 2;
   arcs[0].angle1 = 90 * 64;
   arcs[0].angle2 = 90 * 64;
   arcs[1].x = x + width - radius * 2;
   arcs[1].y = y;
   arcs[1].width  = radius * 2;
   arcs[1].height = radius * 2;
   arcs[1].angle1 = 0 * 64;
   arcs[1].angle2 = 90 * 64;
   arcs[2].x = x;
   arcs[2].y = y + height - radius * 2;
   arcs[2].width  = radius * 2;
   arcs[2].height = radius * 2;
   arcs[2].angle1 = 180 * 64;
   arcs[2].angle2 = 90 * 64;
   arcs[3].x = x + width - radius * 2;
   arcs[3].y = y + height - radius * 2;
   arcs[3].width  = radius * 2;
   arcs[3].height = radius * 2;
   arcs[3].angle1 = 270 * 64;
   arcs[3].angle2 = 90 * 64;
   JXDrawArcs(display, d, gc, arcs, 4);

#endif
#else

   JXDrawRectangle(display, d, gc, x, y, width, height);
   
#endif
}

/** Fill a rounded rectangle. */
#ifdef USE_SHAPE
void FillRoundedRectangle(Drawable d, GC gc, int x, int y,
                          int width, int height, int radius)
{

#ifdef USE_XMU

   XmuFillRoundedRectangle(display, d, gc, x, y, width, height,
                           radius, radius);

#else

   XRectangle  rects[3];
   XArc        arcs[4];

   rects[0].x = x + radius;
   rects[0].y = y;
   rects[0].width = width - radius * 2;
   rects[0].height = radius;
   rects[1].x = x;
   rects[1].y = radius;
   rects[1].width = width;
   rects[1].height = height - radius * 2;
   rects[2].x = x + radius;
   rects[2].y = y + height - radius;
   rects[2].width = width - radius * 2;
   rects[2].height = radius;
   JXFillRectangles(display, d, gc, rects, 3);

   arcs[0].x = x;
   arcs[0].y = y;
   arcs[0].width = radius * 2;
   arcs[0].height = radius * 2;
   arcs[0].angle1 = 90 * 64;
   arcs[0].angle2 = 90 * 64;
   arcs[1].x = x + width - radius * 2 - 1;
   arcs[1].y = y;
   arcs[1].width  = radius * 2;
   arcs[1].height = radius * 2;
   arcs[1].angle1 = 0 * 64;
   arcs[1].angle2 = 90 * 64;
   arcs[2].x = x;
   arcs[2].y = y + height - radius * 2 - 1;
   arcs[2].width  = radius * 2;
   arcs[2].height = radius * 2;
   arcs[2].angle1 = 180 * 64;
   arcs[2].angle2 = 90 * 64;
   arcs[3].x = x + width - radius * 2 - 1;
   arcs[3].y = y + height - radius * 2 -1;
   arcs[3].width  = radius * 2;
   arcs[3].height = radius * 2;
   arcs[3].angle1 = 270 * 64;
   arcs[3].angle2 = 90 * 64;
   JXFillArcs(display, d, gc, arcs, 4);

#endif

}
#endif
 
