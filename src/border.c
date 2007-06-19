/**
 * Functions for dealing with window borders.
 * Copyright (C) 2004 Joe Wingbermuehle
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

typedef enum {
   BP_CLOSE,
   BP_MINIMIZE,
   BP_MAXIMIZE,
   BP_MAXIMIZE_ACTIVE,
   BP_COUNT
} BorderPixmapType;

typedef unsigned char BorderPixmapDataType[32];

static BorderPixmapDataType bitmaps[BP_COUNT] = {

   /* Close */
   { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x38, 0x38, 0x70, 0x1C,
     0xE0, 0x0E, 0xC0, 0x07, 0x80, 0x03, 0xC0, 0x07, 0xE0, 0x0E, 0x70, 0x1C,
     0x38, 0x38, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00 },

   /* Minimize */
   { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x07,
     0xF8, 0x07, 0xF8, 0x07, 0x00, 0x00, 0x00, 0x00 },

   /* Maximize */
   { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x3F, 0xF8, 0x3F, 0xF8, 0x3F,
     0x08, 0x20, 0x08, 0x20, 0x08, 0x20, 0x08, 0x20, 0x08, 0x20, 0x08, 0x20,
     0x08, 0x20, 0xF8, 0x3F, 0x00, 0x00, 0x00, 0x00 },

   /* Maximize Active */
   { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x1F, 0xC0, 0x1F,
     0x00, 0x10, 0xF8, 0x13, 0xF8, 0x13, 0x08, 0x12, 0x08, 0x1A, 0x08, 0x02,
     0x08, 0x02, 0xF8, 0x03, 0x00, 0x00, 0x00, 0x00 }

};

static Pixmap pixmaps[BP_COUNT];

static Region borderRegion = NULL;
static GC borderGC;

static void DrawBorderHelper(const ClientNode *np, int drawIcon);
static void DrawBorderButtons(const ClientNode *np, Pixmap canvas, GC gc);
static int GetButtonCount(const ClientNode *np);

/** Initialize non-server resources. */
void InitializeBorders() {
}

/** Initialize server resources. */
void StartupBorders() {

   XGCValues gcValues;
   unsigned long gcMask;
   int x;

   for(x = 0; x < BP_COUNT; x++) {

      pixmaps[x] = JXCreateBitmapFromData(display, rootWindow,
         (char*)bitmaps[x], 16, 16);

   }

   gcMask = GCGraphicsExposures;
   gcValues.graphics_exposures = False;
   borderGC = JXCreateGC(display, rootWindow, gcMask, &gcValues);

}

/** Release server resources. */
void ShutdownBorders() {

   int x;

   JXFreeGC(display, borderGC);

   for(x = 0; x < BP_COUNT; x++) {
      JXFreePixmap(display, pixmaps[x]);
   }

}

/** Release non-server resources. */
void DestroyBorders() {
}

/** Get the size of the icon to display on a window. */
int GetBorderIconSize() {
   return titleHeight - 6;
}

/** Determine the border action to take given coordinates. */
BorderActionType GetBorderActionType(const ClientNode *np, int x, int y) {

   int north, south, east, west;
   int offset;

   Assert(np);

   GetBorderSize(np, &north, &south, &east, &west);

   /* Check title bar actions. */
   if(np->state.border & BORDER_TITLE) {

      /* Check buttons on the title bar. */
      if(y > 0 && y <= titleHeight) {

         /* Menu button. */
         if(np->icon && np->width >= titleHeight) {
            if(x > 0 && x <= titleHeight) {
               return BA_MENU;
            }
         }

         /* Close button. */
         offset = np->width + west + east - titleHeight;
         if((np->state.border & BORDER_CLOSE) && offset > titleHeight) {
            if(x > offset && x < offset + titleHeight) {
               return BA_CLOSE;
            }
            offset -= titleHeight;
         }

         /* Maximize button. */
         if((np->state.border & BORDER_MAX) && offset > titleHeight) {
            if(x > offset && x < offset + titleHeight) {
               return BA_MAXIMIZE;
            }
            offset -= titleHeight;
         }

         /* Minimize button. */
         if((np->state.border & BORDER_MIN) && offset > titleHeight) {
            if(x > offset && x < offset + titleHeight) {
               return BA_MINIMIZE;
            }
         }

      }

      /* Check for move. */
      if(y > 0 && y <= titleHeight) {
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

   /* Check south east/west resizing. */
   if(np->width >= titleHeight * 2 && np->height >= titleHeight * 2) {
      if(y > np->height + north - titleHeight) {
         if(x < titleHeight) {
            return BA_RESIZE_S | BA_RESIZE_W | BA_RESIZE;
         } else if(x > np->width + west - titleHeight) {
            return BA_RESIZE_S | BA_RESIZE_E | BA_RESIZE;
         }
      }
   }

   /* Check east, west, and south resizing. */
   if(x <= west) {
      return BA_RESIZE_W | BA_RESIZE;
   } else if(x >= np->width + west) {
      return BA_RESIZE_E | BA_RESIZE;
   } else if(y >= np->height + north) {
      return BA_RESIZE_S | BA_RESIZE;
   } else {
      return BA_NONE;
   }

}

/** Draw a client border. */
void DrawBorder(const ClientNode *np, const XExposeEvent *expose) {

   XRectangle rect;
   int drawIcon;
   int temp;

   Assert(np);

   /* Don't draw any more if we are shutting down. */
   if(shouldExit) {
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

   if(expose) {

      /* An expose event caused this draw.
       * Only draw what needs to be drawn to reduce flicker.
       */

      /* Create the region to use if needed. */
      if(!borderRegion) {
         borderRegion = XCreateRegion();
      }

      /* Add the exposed area to the region. */
      rect.x = (short)expose->x;
      rect.y = (short)expose->y;
      rect.width = (unsigned short)expose->width;
      rect.height = (unsigned short)expose->height;
      XUnionRectWithRegion(&rect, borderRegion, borderRegion);

      /* We return now if there are more expose events coming. */
      if(expose->count) {
         return;
      }

      /* Determine if the icon should be redrawn. This is needed
       * since icons need a separate GC for applying shape masks.
       * Note that if the icon were naively redrawn, icons with
       * alpha channels would acquire artifacts since the area under
       * them would not be cleared. So if any part of the icon needs
       * to be redrawn, we clear the area and redraw the whole icon.
       */
      drawIcon = 0;
      if(np->icon && (np->state.border & BORDER_TITLE)) {
         temp = GetBorderIconSize();
         rect.x = 3;
         rect.y = (short)(titleHeight / 2 - temp / 2);
         rect.width = (unsigned short)temp;
         rect.height = (unsigned short)temp;
         if(XRectInRegion(borderRegion,
            rect.x, rect.y, rect.width, rect.height) != RectangleOut) {

            drawIcon = 1;
            XUnionRectWithRegion(&rect, borderRegion, borderRegion);

         }
      }

      /* Time to redraw the border. Set the clip mask. */
      XSetRegion(display, borderGC, borderRegion);

   } else {

      /* An expose event did not occur. Redraw everything. */
      drawIcon = 1;
      XSetClipMask(display, borderGC, None);

   }

   /* Do the actual drawing. */
   DrawBorderHelper(np, drawIcon);

   /* We no longer need the region, release it. */
   if(expose) {
      XDestroyRegion(borderRegion);
      borderRegion = NULL;
   }

}

/** Helper method for drawing borders. */
void DrawBorderHelper(const ClientNode *np, int drawIcon) {

   ColorType borderTextColor;
   long borderTextPixel;

   long titleColor1, titleColor2;
   long cornerColor;
   long outlineColor;

   int north, south, east, west;
   unsigned int width, height;
   int iconSize;

   int buttonCount, titleWidth;
   Pixmap canvas;
   GC gc;

   Assert(np);

   iconSize = GetBorderIconSize();
   GetBorderSize(np, &north, &south, &east, &west);
   width = np->width + east + west;
   height = np->height + north + south;

   /* Determine the colors and gradients to use. */
   if(np->state.status & STAT_ACTIVE) {

      borderTextColor = COLOR_TITLE_ACTIVE_FG;
      borderTextPixel = colors[COLOR_TITLE_ACTIVE_FG];
      cornerColor = colors[COLOR_CORNER_ACTIVE_BG];
      titleColor1 = colors[COLOR_TITLE_ACTIVE_BG1];
      titleColor2 = colors[COLOR_TITLE_ACTIVE_BG2];
      outlineColor = colors[COLOR_BORDER_ACTIVE_LINE];

   } else {

      borderTextColor = COLOR_TITLE_FG;
      borderTextPixel = colors[COLOR_TITLE_FG];
      cornerColor = colors[COLOR_CORNER_BG];
      titleColor1 = colors[COLOR_TITLE_BG1];
      titleColor2 = colors[COLOR_TITLE_BG2];
      outlineColor = colors[COLOR_BORDER_LINE];

   }

   canvas = np->parent;
   gc = borderGC;

   /* Set the window background color (to reduce flickering). */
   JXSetWindowBackground(display, canvas, titleColor2);

   /* Draw the outside border (clear the window with the right color). */
   JXSetForeground(display, gc, titleColor2);
   JXFillRectangle(display, canvas, gc, 0, 0, width, height);

   /* Determine how many pixels may be used for the title. */
   buttonCount = GetButtonCount(np);
   titleWidth = width;
   titleWidth -= titleHeight * buttonCount;
   titleWidth -= iconSize + 4 + 4;

   /* Draw the top part (either a title or north border. */
   if(np->state.border & BORDER_TITLE) {

      /* Draw a title bar. */
      DrawHorizontalGradient(canvas, gc, titleColor1, titleColor2,
         1, 1, width - 2, titleHeight - 2);

      /* Draw the icon. */
      if(np->icon && np->width >= titleHeight && drawIcon) {
         PutIcon(np->icon, canvas, 3, titleHeight / 2 - iconSize / 2,
            iconSize, iconSize);
      }

      if(np->name && np->name[0] && titleWidth > 0) {
         RenderString(canvas, FONT_BORDER, borderTextColor,
            iconSize + 3 + 2,
            titleHeight / 2 - GetStringHeight(FONT_BORDER) / 2,
            titleWidth, borderRegion, np->name);
      }

   }

   /* Draw the east, west, and south borders. */
   if((np->state.border & BORDER_OUTLINE)
      && !(np->state.status & STAT_SHADED)) {

      /* Draw corners if there's room. */
      if((np->state.border & BORDER_RESIZE)
         && !(np->state.status & STAT_SHADED)
         && np->width >= titleHeight * 2
         && np->height >= titleHeight * 2) {

         JXSetForeground(display, gc, cornerColor);

         /* South west. */
         JXFillRectangle(display, canvas, gc,
            0, height - titleHeight, titleHeight, titleHeight);

         /* South east. */
         JXFillRectangle(display, canvas, gc,
            width - titleHeight, height - titleHeight,
            titleHeight, titleHeight);

         /* Corner outline. */
         JXSetForeground(display, gc, outlineColor);
         JXDrawRectangle(display, canvas, gc,
            0, height - titleHeight, titleHeight, titleHeight + 1);
         JXDrawRectangle(display, canvas, gc,
            width - titleHeight, height - titleHeight,
            titleHeight - 1, titleHeight - 1);

      }

   }

   /* Window outline. */
   JXSetForeground(display, gc, outlineColor);
   if(np->state.status & STAT_SHADED) {
      XDrawRectangle(display, canvas, gc, 0, 0, width - 1, north - 1);
   } else {
      XDrawRectangle(display, canvas, gc, 0, 0, width - 1, height - 1);
      XDrawRectangle(display, canvas, gc, west - 1, north - 1,
         np->width + 1, np->height + 1);
   }

   DrawBorderButtons(np, canvas, gc);

}

/** Determine the number of buttons to be displayed for a client. */
int GetButtonCount(const ClientNode *np) {

   int north, south, east, west;
   int count;
   int offset;

   if(!(np->state.border & BORDER_TITLE)) {
      return 0;
   }

   GetBorderSize(np, &north, &south, &east, &west);

   offset = np->width + east + west - titleHeight;
   if(offset <= titleHeight) {
      return 0;
   }

   count = 0;
   if(np->state.border & BORDER_CLOSE) {
      offset -= titleHeight;
      ++count;
      if(offset <= titleHeight) {
         return count;
      }
   }

   if(np->state.border & BORDER_MAX) {
      offset -= titleHeight;
      ++count;
      if(offset <= titleHeight) {
         return count;
      }
   }

   if(np->state.border & BORDER_MIN) {
      ++count;
   }

   return count;

}

/** Draw the buttons on a client frame. */
void DrawBorderButtons(const ClientNode *np, Pixmap canvas, GC gc) {

   Pixmap pixmap;
   long color;
   long outlineColor;
   int offset;
   int yoffset;
   int north, south, east, west;

   Assert(np);

   if(!(np->state.border & BORDER_TITLE)) {
      return;
   }

   GetBorderSize(np, &north, &south, &east, &west);
   offset = np->width + east + west - titleHeight;
   if(offset <= titleHeight) {
      return;
   }

   yoffset = titleHeight / 2 - 16 / 2;

   /* Determine the colors to use. */
   if(np->state.status & STAT_ACTIVE) {
      color = colors[COLOR_TITLE_ACTIVE_FG];
      outlineColor = colors[COLOR_BORDER_ACTIVE_LINE];
   } else {
      color = colors[COLOR_TITLE_FG];
      outlineColor = colors[COLOR_BORDER_LINE];
   }

   /* Close button. */
   if(np->state.border & BORDER_CLOSE) {

      JXSetForeground(display, gc, outlineColor);
      JXDrawLine(display, canvas, gc, offset, 0, offset, titleHeight);

      pixmap = pixmaps[BP_CLOSE];

      JXSetForeground(display, gc, color);
      JXSetClipMask(display, gc, pixmap);
      JXSetClipOrigin(display, gc, offset + yoffset, yoffset);
      JXFillRectangle(display, canvas, gc, offset + yoffset, yoffset, 16, 16);
      JXSetClipMask(display, gc, None);

      offset -= titleHeight;
      if(offset <= titleHeight) {
         return;
      }

   }

   /* Maximize button. */
   if(np->state.border & BORDER_MAX) {

      JXSetForeground(display, gc, outlineColor);
      JXDrawLine(display, canvas, gc, offset, 0, offset, titleHeight);

      if(np->state.status & (STAT_HMAX | STAT_VMAX)) {
         pixmap = pixmaps[BP_MAXIMIZE_ACTIVE];
      } else {
         pixmap = pixmaps[BP_MAXIMIZE];
      }

      JXSetForeground(display, gc, color);
      XSetClipMask(display, gc, pixmap);
      XSetClipOrigin(display, gc, offset + yoffset, yoffset);
      XFillRectangle(display, canvas, gc, offset + yoffset, yoffset, 16, 16);
      XSetClipMask(display, gc, None);

      offset -= titleHeight;
      if(offset <= titleHeight) {
         return;
      }

   }

   /* Minimize button. */
   if(np->state.border & BORDER_MIN) {

      JXSetForeground(display, gc, outlineColor);
      JXDrawLine(display, canvas, gc, offset, 0, offset, titleHeight);

      pixmap = pixmaps[BP_MINIMIZE];

      JXSetForeground(display, gc, color);
      XSetClipMask(display, gc, pixmap);
      XSetClipOrigin(display, gc, offset + yoffset, yoffset);
      XFillRectangle(display, canvas, gc, offset + yoffset, yoffset, 16, 16);
      XSetClipMask(display, gc, None);

   }

}

/** Redraw the borders on the current desktop.
 * This should be done after loading clients since the stacking order
 * may cause borders on the current desktop to become visible after moving
 * clients to their assigned desktops.
 */
void ExposeCurrentDesktop() {

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
   int *north, int *south, int *east, int *west) {

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

      *north = borderWidth;
      *south = borderWidth;
      *east = borderWidth;
      *west = borderWidth;

   } else {

      *north = 0;
      *south = 0;
      *east = 0;
      *west = 0;

   }

   if(np->state.border & BORDER_TITLE) {
      *north = titleHeight;
   }

   if(np->state.status & STAT_SHADED) {
      *south = 0;
   }

}

/** Set the size of window borders. */
void SetBorderWidth(const char *str) {

   int width;

   if(str) {

      width = atoi(str);
      if(width < MIN_BORDER_WIDTH || width > MAX_BORDER_WIDTH) {
         borderWidth = DEFAULT_BORDER_WIDTH;
         Warning("invalid border width specified: %d", width);
      } else {
         borderWidth = width;
      }

   }

}

/** Set the height of the title bar. */
void SetTitleHeight(const char *str) {

   int height;

   if(str) {

      height = atoi(str);
      if(height < MIN_TITLE_HEIGHT || height > MAX_TITLE_HEIGHT) {
         titleHeight = DEFAULT_TITLE_HEIGHT;
         Warning("invalid title height specified: %d", height);
      } else {
         titleHeight = height;
      }

   }

}

