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

typedef unsigned char BorderPixmapDataType[32];

static BorderPixmapDataType bitmaps[BP_COUNT] = {

   /* Close */
   { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x30, 0x06, 0x70, 0x07, 0xE0, 0x03, 0xC0, 0x01, 0xE0, 0x03, 0x70, 0x07,
     0x30, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },

   /* Minimize */
   { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0xF8, 0x07, 0xF8, 0x07, 0x00, 0x00, 0x00, 0x00 },
 
   /* Maximize */
   { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x1F,
     0xF8, 0x1F, 0x08, 0x10, 0x08, 0x10, 0x08, 0x10, 0x08, 0x10, 0x08, 0x10,
     0xF8, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },

   /* Maximize Active */
   { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x0F,
     0xC0, 0x0F, 0x00, 0x08, 0xF0, 0x0B, 0xF0, 0x0B, 0x10, 0x0A, 0x10, 0x0A,
     0x10, 0x02, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00 }

};

static Pixmap pixmaps[BP_COUNT];

static GC borderGC;

static void DrawBorderHelper(const ClientNode *np);
static void DrawBorderButtons(const ClientNode *np, Pixmap canvas);
static int GetButtonCount(const ClientNode *np);

#ifdef USE_SHAPE
static void FillRoundedRectangle(Drawable d, GC gc, int x, int y,
                                 int width, int height, int radius);
#endif

/** Initialize non-server resources. */
void InitializeBorders()
{
}

/** Initialize server resources. */
void StartupBorders()
{

   XGCValues gcValues;
   const char *file;
   unsigned long gcMask;
   int x, hotx, hoty;
   unsigned int bmpHeight, bmpWidth;
   int found;

   for(x = 0; x < BP_COUNT; x++) {
      file = settings.borderButtonBitmaps[x];
      found = file ? 1 : 0;
      if(found) {
         found = XReadBitmapFile(display, rootWindow, file, &bmpWidth,
                                 &bmpHeight, &pixmaps[x], &hotx, &hoty)
                  == BitmapSuccess;
         if(JUNLIKELY(!found)) {
            Warning(_("bitmap could not be loaded: %s"), file);
         }
      }
      if(!found) {
         pixmaps[x] = JXCreateBitmapFromData(display, rootWindow,
                                             (char*)bitmaps[x], 16, 16);
      }
   }

   gcMask = GCGraphicsExposures;
   gcValues.graphics_exposures = False;
   borderGC = JXCreateGC(display, rootWindow, gcMask, &gcValues);

}

/** Release server resources. */
void ShutdownBorders()
{
   unsigned int x;

   JXFreeGC(display, borderGC);
   for(x = 0; x < BP_COUNT; x++) {
      JXFreePixmap(display, pixmaps[x]);
   }
}

/** Release non-server resources. */
void DestroyBorders()
{
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

   Assert(np);

   GetBorderSize(np, &north, &south, &east, &west);

   /* Check title bar actions. */
   if(np->state.border & BORDER_TITLE) {

      /* Check buttons on the title bar. */
      if(y >= south && y <= settings.titleHeight) {

         /* Menu button. */
         if(np->icon && np->width >= settings.titleHeight) {
            if(x > 0 && x <= settings.titleHeight) {
               return BA_MENU;
            }
         }

         /* Close button. */
         offset = np->width + west + east - settings.titleHeight;
         if(   (np->state.border & BORDER_CLOSE)
            && offset > settings.titleHeight) {
            if(x > offset && x < offset + settings.titleHeight) {
               return BA_CLOSE;
            }
            offset -= settings.titleHeight;
         }

         /* Maximize button. */
         if((np->state.border & BORDER_MAX) && offset > settings.titleHeight) {
            if(x > offset && x < offset + settings.titleHeight) {
               return BA_MAXIMIZE;
            }
            offset -= settings.titleHeight;
         }

         /* Minimize button. */
         if((np->state.border & BORDER_MIN) && offset > settings.titleHeight) {
            if(x > offset && x < offset + settings.titleHeight) {
               return BA_MINIMIZE;
            }
         }

      }

      /* Check for move. */
      if(y >= south && y <= settings.titleHeight) {
         if(x > 0 && x < np->width + east + west) {
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

   /* Check south east/west and north east/west resizing. */
   if(   np->width >= settings.titleHeight * 2
      && np->height >= settings.titleHeight * 2) {
      if(y > np->height + north - settings.titleHeight) {
         if(x < settings.titleHeight) {
            return BA_RESIZE_S | BA_RESIZE_W | BA_RESIZE;
         } else if(x > np->width + west - settings.titleHeight) {
            return BA_RESIZE_S | BA_RESIZE_E | BA_RESIZE;
         }
      } else if(y < settings.titleHeight) {
         if(x < settings.titleHeight) {
            return BA_RESIZE_N | BA_RESIZE_W | BA_RESIZE;
         } else if(x > np->width + west - settings.titleHeight) {
            return BA_RESIZE_N | BA_RESIZE_E | BA_RESIZE;
         }
      }
   }

   /* Check east, west, north, and south resizing. */
   if(x <= west) {
      return BA_RESIZE_W | BA_RESIZE;
   } else if(x >= np->width + west) {
      return BA_RESIZE_E | BA_RESIZE;
   } else if(y >= np->height + north) {
      return BA_RESIZE_S | BA_RESIZE;
   } else if(y <= south) {
      return BA_RESIZE_N | BA_RESIZE;
   } else {
      return BA_NONE;
   }

}

/** Draw a client border. */
void DrawBorder(const ClientNode *np, const XExposeEvent *expose)
{

   XRectangle rect;
   Pixmap canvas;
   Region borderRegion;
   int drawIcon;
   int temp;

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

   if(expose && expose->count) {
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

   int buttonCount, titleWidth;
   Pixmap canvas;

   Assert(np);

   iconSize = GetBorderIconSize();
   GetBorderSize(np, &north, &south, &east, &west);
   width = np->width + east + west;
   height = np->height + north + south;

   /* Determine the colors and gradients to use. */
   if(np->state.status & STAT_ACTIVE) {

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

   /* Shape window corners */
   if(np->state.status & STAT_SHADED) {
      ShapeRoundedRectWindow(np->parent, width, north);
   } else {
      ShapeRoundedRectWindow(np->parent, width, height);
   }

   canvas = JXCreatePixmap(display, np->parent, width, height, rootDepth);

   /* Clear the window with the right color. */
   JXSetForeground(display, borderGC, titleColor2);
   JXFillRectangle(display, canvas, borderGC, 0, 0, width, height);

   /* Determine how many pixels may be used for the title. */
   buttonCount = GetButtonCount(np);
   titleWidth = width;
   titleWidth -= settings.titleHeight * buttonCount;
   titleWidth -= iconSize + 7 + 6;

   /* Draw the top part (either a title or north border. */
   if(np->state.border & BORDER_TITLE) {

      /* Draw a title bar. */
      DrawHorizontalGradient(canvas, borderGC, titleColor1, titleColor2,
                             1, 1, width - 2, settings.titleHeight - 2);

      /* Draw the icon. */
      if(np->icon && np->width >= settings.titleHeight) {
         PutIcon(np->icon, canvas, 6, (settings.titleHeight - iconSize) / 2,
                 iconSize, iconSize);
      }

      if(np->name && np->name[0] && titleWidth > 0) {
         RenderString(canvas, FONT_BORDER, borderTextColor,
                      iconSize + 6 + 4,
                      (settings.titleHeight - GetStringHeight(FONT_BORDER)) / 2,
                      titleWidth, np->name);
      }

   }

   /* Window outline. */
   JXSetForeground(display, borderGC, outlineColor);
#ifdef USE_SHAPE
   if(np->state.status & STAT_SHADED) {
      DrawRoundedRectangle(canvas, borderGC, 0, 0, width - 1, north - 1,
                           CORNER_RADIUS);
   } else {
      DrawRoundedRectangle(canvas, borderGC, 0, 0, width - 1, height - 1,
                           CORNER_RADIUS);
   }
#else
   if(np->state.status & STAT_SHADED) {
      JXDrawRectangle(display, canvas, borderGC, 0, 0, width - 1, north - 1);
   } else {
      JXDrawRectangle(display, canvas, borderGC, 0, 0, width - 1, height - 1);
   }
#endif

   DrawBorderButtons(np, canvas);

   JXCopyArea(display, canvas, np->parent, borderGC, 0, 0,
              width, height, 0, 0);
   JXFreePixmap(display, canvas);

}

/** Determine the number of buttons to be displayed for a client. */
int GetButtonCount(const ClientNode *np)
{

   int north, south, east, west;
   int count;
   int offset;

   if(!(np->state.border & BORDER_TITLE)) {
      return 0;
   }

   GetBorderSize(np, &north, &south, &east, &west);

   offset = np->width + east + west - settings.titleHeight;
   if(offset <= settings.titleHeight) {
      return 0;
   }

   count = 0;
   if(np->state.border & BORDER_CLOSE) {
      offset -= settings.titleHeight;
      ++count;
      if(offset <= settings.titleHeight) {
         return count;
      }
   }

   if(np->state.border & BORDER_MAX) {
      offset -= settings.titleHeight;
      ++count;
      if(offset <= settings.titleHeight) {
         return count;
      }
   }

   if(np->state.border & BORDER_MIN) {
      ++count;
   }

   return count;

}

/** Draw the buttons on a client frame. */
void DrawBorderButtons(const ClientNode *np, Pixmap canvas)
{

   Pixmap pixmap;
   long color;
   int offset;
   int yoffset;
   int north, south, east, west;

   Assert(np);

   if(!(np->state.border & BORDER_TITLE)) {
      return;
   }

   GetBorderSize(np, &north, &south, &east, &west);
   offset = np->width + east + west - settings.titleHeight;
   if(offset <= settings.titleHeight) {
      return;
   }

   yoffset = settings.titleHeight / 2 - 16 / 2;

   /* Determine the colors to use. */
   if(np->state.status & STAT_ACTIVE) {
      color = colors[COLOR_TITLE_ACTIVE_FG];
   } else {
      color = colors[COLOR_TITLE_FG];
   }

   /* Close button. */
   if(np->state.border & BORDER_CLOSE) {

      pixmap = pixmaps[BP_CLOSE];

      JXSetForeground(display, borderGC, color);
      JXSetClipMask(display, borderGC, pixmap);
      JXSetClipOrigin(display, borderGC, offset + yoffset, yoffset);
      JXFillRectangle(display, canvas, borderGC,
                      offset + yoffset, yoffset, 16, 16);
      JXSetClipMask(display, borderGC, None);

      offset -= settings.titleHeight;
      if(offset <= settings.titleHeight) {
         return;
      }

   }

   /* Maximize button. */
   if(np->state.border & BORDER_MAX) {

      if(np->state.status & (STAT_HMAX | STAT_VMAX)) {
         pixmap = pixmaps[BP_MAXIMIZE_ACTIVE];
      } else {
         pixmap = pixmaps[BP_MAXIMIZE];
      }

      JXSetForeground(display, borderGC, color);
      JXSetClipMask(display, borderGC, pixmap);
      JXSetClipOrigin(display, borderGC, offset + yoffset, yoffset);
      JXFillRectangle(display, canvas, borderGC,
                      offset + yoffset, yoffset, 16, 16);
      JXSetClipMask(display, borderGC, None);

      offset -= settings.titleHeight;
      if(offset <= settings.titleHeight) {
         return;
      }

   }

   /* Minimize button. */
   if(np->state.border & BORDER_MIN) {

      pixmap = pixmaps[BP_MINIMIZE];

      JXSetForeground(display, borderGC, color);
      JXSetClipMask(display, borderGC, pixmap);
      JXSetClipOrigin(display, borderGC, offset + yoffset, yoffset);
      JXFillRectangle(display, canvas, borderGC,
                      offset + yoffset, yoffset, 16, 16);
      JXSetClipMask(display, borderGC, None);

   }

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
            DrawBorder(np, NULL);
         }
      }
   }

}

/** Get the size of the borders for a client. */
void GetBorderSize(const ClientNode *np,
                   int *north, int *south, int *east, int *west)
{

   Assert(np);
   Assert(north);
   Assert(south);
   Assert(east);
   Assert(west);

   /* Full screen is a special case. */
   if(np->state.status & STAT_FULLSCREEN) {
      *north = 0;
      *south = 0;
      *east = 0;
      *west = 0;
      return;
   }

   if(np->state.border & BORDER_OUTLINE) {

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

   if(np->state.border & BORDER_TITLE) {
      *north = settings.titleHeight;
   }

   if(np->state.status & STAT_SHADED) {
      *south = 0;
   }

}

/** Draw a rounded rectangle. */
void DrawRoundedRectangle(Drawable d, GC gc, int x, int y,
                          int width, int height, int radius)
{

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

/** Clear the shape mask of a window. */
void ResetRoundedRectWindow(const ClientNode *np)
{
#ifdef USE_SHAPE
   XRectangle rect[4];
   int north, south, east, west;

   Assert(np);

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
#endif
}
 
/** Set the shape mask on a window to give a rounded boarder. */
void ShapeRoundedRectWindow(Window w, int width, int height)
{
#ifdef USE_SHAPE

   Pixmap shapePixmap;
   GC shapeGC;

   shapePixmap = JXCreatePixmap(display, w, width, height, 1);
   shapeGC = JXCreateGC(display, shapePixmap, 0, NULL);

   JXSetForeground(display, shapeGC, 0);
   JXFillRectangle(display, shapePixmap, shapeGC, 0, 0,
                   width + 1, height + 1);

   /* Corner bound radius -1 to allow slightly better outline drawing */
   JXSetForeground(display, shapeGC, 1);
   FillRoundedRectangle(shapePixmap, shapeGC, 0, 0, width, height,
                        CORNER_RADIUS - 1);
   
   JXShapeCombineMask(display, w, ShapeBounding, 0, 0, shapePixmap, ShapeIntersect);

   JXFreeGC(display, shapeGC);
   JXFreePixmap(display, shapePixmap);

#endif
}

