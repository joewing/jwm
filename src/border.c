/**
 * @file border.c
 * @author Joe Wingbermuehle
 * @date 2004-2015
 *
 * @brief Functions for dealing with window borders.
 * 
 */

#include "jwm.h"
#include "border.h"
#include "client.h"
#include "clientlist.h"
#include "color.h"
#include "icon.h"
#include "font.h"
#include "misc.h"
#include "settings.h"
#include "grab.h"

static char *buttonNames[BI_COUNT];
static IconNode *buttonIcons[BI_COUNT];

static void DrawBorderHelper(const ClientNode *np);
static void DrawBorderHandles(const ClientNode *np,
                              Pixmap canvas, GC gc);
static void DrawBorderButtons(const ClientNode *np,
                              Pixmap canvas, GC gc);
static char DrawBorderIcon(BorderIconType t,
                           unsigned xoffset, unsigned yoffset,
                           Pixmap canvas, long fg);
static void DrawCloseButton(unsigned xoffset, unsigned yoffset,
                            Pixmap canvas, GC gc, long fg);
static void DrawMaxIButton(unsigned xoffset, unsigned yoffset,
                           Pixmap canvas, GC gc, long fg);
static void DrawMaxAButton(unsigned xoffset, unsigned yoffset,
                           Pixmap canvas, GC gc, long fg);
static void DrawMinButton(unsigned xoffset, unsigned yoffset,
                          Pixmap canvas, GC gc, long fg);
static unsigned GetButtonCount(const ClientNode *np);

#ifdef USE_SHAPE
static void FillRoundedRectangle(Drawable d, GC gc, int x, int y,
                                 int width, int height, int radius);
#endif

/** Initialize structures. */
void InitializeBorders(void)
{
   memset(buttonNames, 0, sizeof(buttonNames));
}

/** Initialize server resources. */
void StartupBorders(void)
{
   unsigned int i;

   for(i = 0; i < BI_COUNT; i++) {
      if(buttonNames[i]) {
         buttonIcons[i] = LoadNamedIcon(buttonNames[i], 1, 1);
         Release(buttonNames[i]);
         buttonNames[i] = NULL;
      } else {
         buttonIcons[i] = NULL;
      }
   }

   /* Always load a menu icon for windows without one. */
   if(buttonIcons[BI_MENU] == NULL) {
      buttonIcons[BI_MENU] = GetDefaultIcon();
   }
}

/** Destroy structures. */
void DestroyBorders(void)
{
   unsigned i;
   for(i = 0; i < BI_COUNT; i++)
   {
      if(buttonNames[i]) {
         Release(buttonNames[i]);
         buttonNames[i] = NULL;
      }
   }
}

/** Get the size of the icon to display on a window. */
int GetBorderIconSize(void)
{
   if(settings.windowDecorations == DECO_MOTIF) {
      return settings.titleHeight - 4;
   } else {
      return settings.titleHeight - 6;
   }
}

/** Determine the border action to take given coordinates. */
BorderActionType GetBorderActionType(const ClientNode *np, int x, int y)
{

   int north, south, east, west;
   unsigned int resizeMask;
   const unsigned int titleHeight = settings.titleHeight;

   GetBorderSize(&np->state, &north, &south, &east, &west);

   /* Check title bar actions. */
   if((np->state.border & BORDER_TITLE) &&
      titleHeight > settings.borderWidth) {

      /* Check buttons on the title bar. */
      int offset = np->width + west;
      if(y >= south && y <= titleHeight + south) {

         /* Menu button. */
         if(np->width >= titleHeight) {
            if(x > west && x <= titleHeight + west) {
               return BA_MENU;
            }
         }

         /* Close button. */
         if((np->state.border & BORDER_CLOSE) && offset > 2 * titleHeight) {
            if(x > offset - titleHeight && x < offset) {
               return BA_CLOSE;
            }
            offset -= titleHeight + 1;
         }

         /* Maximize button. */
         if((np->state.border & BORDER_MAX) && offset > 2 * titleHeight) {
            if(x > offset - titleHeight && x < offset) {
               return BA_MAXIMIZE;
            }
            offset -= titleHeight + 1;
         }

         /* Minimize button. */
         if((np->state.border & BORDER_MIN) && offset > 2 * titleHeight) {
            if(x > offset - titleHeight && x < offset) {
               return BA_MINIMIZE;
            }
         }

      }

      /* Check for move. */
      if(y >= south && y <= titleHeight + south) {
         if(x > west && x < offset) {
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
   if(np->state.maxFlags & MAX_HORIZ) {
      resizeMask &= ~(BA_RESIZE_E | BA_RESIZE_W);
   }
   if(np->state.maxFlags & MAX_VERT) {
      resizeMask &= ~(BA_RESIZE_N | BA_RESIZE_S);
   }
   if(np->state.status & STAT_SHADED) {
      resizeMask &= ~(BA_RESIZE_N | BA_RESIZE_S);
   }

   /* Check south east/west and north east/west resizing. */
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

   if(np->parent == None) {
      JXMoveResizeWindow(display, np->window, np->x, np->y,
                         np->width, np->height);
      return;
   }

   GrabServer();

   /* Determine the size of the window. */
   GetBorderSize(&np->state, &north, &south, &east, &west);
   width = np->width + east + west;
   if(np->state.status & STAT_SHADED) {
      height = north + south;
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
   if(settings.cornerRadius > 0 || (np->state.status & STAT_SHAPED)) {

      /* First set the shape to the window border. */
      shapePixmap = JXCreatePixmap(display, np->parent, width, height, 1);
      shapeGC = JXCreateGC(display, shapePixmap, 0, NULL);

      /* Make the whole area transparent. */
      JXSetForeground(display, shapeGC, 0);
      JXFillRectangle(display, shapePixmap, shapeGC, 0, 0, width, height);

      /* Draw the window area without the corners. */
      /* Corner bound radius -1 to allow slightly better outline drawing */
      JXSetForeground(display, shapeGC, 1);
      if(((np->state.status & STAT_FULLSCREEN) || np->state.maxFlags) &&
         !(np->state.status & (STAT_SHADED))) {
         JXFillRectangle(display, shapePixmap, shapeGC, 0, 0, width, height);
      } else {
         FillRoundedRectangle(shapePixmap, shapeGC, 0, 0, width, height,
                              settings.cornerRadius - 1);
      }

      /* Apply the client window. */
      if(!(np->state.status & STAT_SHADED) &&
          (np->state.status & STAT_SHAPED)) {

         XRectangle *rects;
         int count;
         int ordering;

         /* Cut out an area for the client window. */
         JXSetForeground(display, shapeGC, 0);
         JXFillRectangle(display, shapePixmap, shapeGC, west, north,
                         np->width, np->height);

         /* Fill in the visible area. */
         rects = JXShapeGetRectangles(display, np->window, ShapeBounding,
                                      &count, &ordering);
         if(JLIKELY(rects)) {
            int i;
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
   }
#endif

   UngrabServer();

}

/** Draw a client border. */
void DrawBorder(ClientNode *np)
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

   /* Create the frame if needed. */
   ReparentClient(np);

   /* Return if there is no border. */
   if(np->parent == None) {
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

   unsigned int buttonCount;
   int titleWidth;
   Pixmap canvas;
   GC gc;

   Assert(np);

   GetBorderSize(&np->state, &north, &south, &east, &west);
   width = np->width + east + west;
   height = np->height + north + south;

   /* Determine the colors and gradients to use. */
   if(np->state.status & (STAT_ACTIVE | STAT_FLASH)) {

      borderTextColor = COLOR_TITLE_ACTIVE_FG;
      titleColor1 = colors[COLOR_TITLE_ACTIVE_BG1];
      titleColor2 = colors[COLOR_TITLE_ACTIVE_BG2];
      outlineColor = colors[COLOR_TITLE_ACTIVE_DOWN];

   } else {

      borderTextColor = COLOR_TITLE_FG;
      titleColor1 = colors[COLOR_TITLE_BG1];
      titleColor2 = colors[COLOR_TITLE_BG2];
      outlineColor = colors[COLOR_TITLE_DOWN];

   }

   /* Set parent background to reduce flicker. */
   JXSetWindowBackground(display, np->parent, titleColor2);

   canvas = JXCreatePixmap(display, np->parent, width, north, rootDepth);
   gc = JXCreateGC(display, canvas, 0, NULL);

   /* Clear the window with the right color. */
   JXSetForeground(display, gc, titleColor2);
   JXFillRectangle(display, canvas, gc, 0, 0, width, north);

   /* Determine how many pixels may be used for the title. */
   buttonCount = GetButtonCount(np);
   titleWidth = width - east - west - 5;
   titleWidth -= settings.titleHeight * (buttonCount + 1);
   titleWidth -= settings.windowDecorations == DECO_MOTIF
               ? (buttonCount + 1) : 0;

   /* Draw the top part (either a title or north border). */
   if((np->state.border & BORDER_TITLE) &&
      settings.titleHeight > settings.borderWidth) {

      const unsigned startx = west + 1;
      const unsigned starty = settings.windowDecorations == DECO_MOTIF
                            ? (south - 1) : 0;

      /* Draw a title bar. */
      DrawHorizontalGradient(canvas, gc, titleColor1, titleColor2,
                             0, 1, width, settings.titleHeight - 2);

      /* Draw the icon. */
#ifdef USE_ICONS
      if(np->width >= settings.titleHeight) {
         const int iconSize = GetBorderIconSize();
         IconNode *icon = np->icon ? np->icon : buttonIcons[BI_MENU];
         PutIcon(icon, canvas, colors[borderTextColor],
                 startx, starty + (settings.titleHeight - iconSize) / 2,
                 iconSize, iconSize);
      }
#endif

      if(np->name && np->name[0] && titleWidth > 0) {
         const int sheight = GetStringHeight(FONT_BORDER);
         const int textWidth = GetStringWidth(FONT_BORDER, np->name);
         unsigned titlex, titley;
         int xoffset = 0;
         switch (settings.titleTextAlignment) {
         case ALIGN_CENTER:
            xoffset = (titleWidth - textWidth) / 2;
            break;
         case ALIGN_RIGHT:
            xoffset = (titleWidth - textWidth);
            break;
         }
         xoffset = Max(xoffset, 0);
         titlex = startx + settings.titleHeight + xoffset
                + (settings.windowDecorations == DECO_MOTIF ? 4 : 0);
         titley = starty + (settings.titleHeight - sheight) / 2;
         RenderString(canvas, FONT_BORDER, borderTextColor,
                      titlex, titley, titleWidth, np->name);
      }

      DrawBorderButtons(np, canvas, gc);

   }

   /* Copy the pixmap (for the title bar) to the window. */

   /* Copy the pixmap for the title bar and clear the part of
    * the window to be drawn directly. */
   if(settings.windowDecorations == DECO_MOTIF) {
      JXCopyArea(display, canvas, np->parent, gc, 2, 2,
         width - 4, north - 2, 2, 2);
      JXClearArea(display, np->parent,
         2, north, width - 4, height - north - 2, False);
   } else {
      JXCopyArea(display, canvas, np->parent, gc, 1, 1,
         width - 2, north - 1, 1, 1);
      JXClearArea(display, np->parent,
         1, north, width - 2, height - north - 1, False);
   }

   /* Window outline. */
   if(settings.windowDecorations == DECO_MOTIF) {
      DrawBorderHandles(np, np->parent, gc);
   } else {
      JXSetForeground(display, gc, outlineColor);
      if(np->state.status & STAT_SHADED) {
         DrawRoundedRectangle(np->parent, gc, 0, 0, width - 1, north - 1,
                              settings.cornerRadius);
      } else if(np->state.maxFlags & MAX_HORIZ) {
         if(!(np->state.maxFlags & (MAX_TOP | MAX_VERT))) {
            /* Top */
            JXDrawLine(display, np->parent, gc, 0, 0, width, 0);
         }
         if(!(np->state.maxFlags & (MAX_BOTTOM | MAX_VERT))) {
            /* Bottom */
            JXDrawLine(display, np->parent, gc,
                       0, height - 1, width, height - 1);
         }
      } else if(np->state.maxFlags & MAX_VERT) {
         if(!(np->state.maxFlags & (MAX_LEFT | MAX_HORIZ))) {
            /* Left */
            JXDrawLine(display, np->parent, gc, 0, 0, 0, height);
         }
         if(!(np->state.maxFlags & (MAX_RIGHT | MAX_HORIZ))) {
            /* Right */
            JXDrawLine(display, np->parent, gc, width - 1, 0,
               width - 1, height);
         }
      } else {
         DrawRoundedRectangle(np->parent, gc, 0, 0, width - 1, height - 1,
                              settings.cornerRadius);
      }
   }

   JXFreePixmap(display, canvas);
   JXFreeGC(display, gc);

}

/** Draw window handles. */
void DrawBorderHandles(const ClientNode *np, Pixmap canvas, GC gc)
{
   XSegment segments[8];
   long pixelUp, pixelDown;
   int width, height;
   int north, south, east, west;
   unsigned offset = 0;

   /* Determine the window size. */
   GetBorderSize(&np->state, &north, &south, &east, &west);
   width = np->width + east + west;
   if(np->state.status & STAT_SHADED) {
      height = north + south;
   } else {
      height = np->height + north + south;
   }

   /* Determine the colors to use. */
   if(np->state.status & (STAT_ACTIVE | STAT_FLASH)) {
      pixelUp = colors[COLOR_TITLE_ACTIVE_UP];
      pixelDown = colors[COLOR_TITLE_ACTIVE_DOWN];
   } else {
      pixelUp = colors[COLOR_TITLE_UP];
      pixelDown = colors[COLOR_TITLE_DOWN];
   }

   if(!(np->state.maxFlags & MAX_VERT)) {
      /* Top title border. */
      segments[offset].x1 = west;
      segments[offset].y1 = settings.borderWidth;
      segments[offset].x2 = width - east - 1;
      segments[offset].y2 = settings.borderWidth;
      offset += 1;
   }

   if(!(np->state.maxFlags & MAX_HORIZ)) {
      /* Right title border. */
      segments[offset].x1 = west;
      segments[offset].y1 = south + 1;
      segments[offset].x2 = east;
      segments[offset].y2 = settings.titleHeight + south - 1;
      offset += 1;

      /* Inside right border. */
      segments[offset].x1 = width - east;
      segments[offset].y1 = south;
      segments[offset].x2 = width - east;
      segments[offset].y2 = height - south;
      offset += 1;
   }

   /* Inside bottom border. */
   segments[offset].x1 = west;
   segments[offset].y1 = height - south;
   segments[offset].x2 = width - east;
   segments[offset].y2 = height - south;
   offset += 1;

   if(!(np->state.maxFlags & MAX_HORIZ)) {
      /* Left border. */
      segments[offset].x1 = 0;
      segments[offset].y1 = 0;
      segments[offset].x2 = 0;
      segments[offset].y2 = height - 1;
      offset += 1;
      segments[offset].x1 = 1;
      segments[offset].y1 = 1;
      segments[offset].x2 = 1;
      segments[offset].y2 = height - 2;
      offset += 1;
   }

   if(!(np->state.maxFlags & MAX_VERT)) {
      /* Top border. */
      segments[offset].x1 = 1;
      segments[offset].y1 = 0;
      segments[offset].x2 = width - 1;
      segments[offset].y2 = 0;
      offset += 1;
      segments[offset].x1 = 1;
      segments[offset].y1 = 1;
      segments[offset].x2 = width - 2;
      segments[offset].y2 = 1;
      offset += 1;
   }

   /* Draw pixel-up segments. */
   JXSetForeground(display, gc, pixelUp);
   JXDrawSegments(display, canvas, gc, segments, offset);
   offset = 0;

   /* Bottom title border. */
   segments[offset].x1 = west + 1;
   segments[offset].y1 = north - 1;
   segments[offset].x2 = width - east - 1;
   segments[offset].y2 = north - 1;
   offset += 1;

   if(!(np->state.maxFlags & MAX_HORIZ)) {
      /* Right title border. */
      segments[offset].x1 = width - east - 1;
      segments[offset].y1 = south + 1;
      segments[offset].x2 = width - east - 1;
      segments[offset].y2 = north - 1;
      offset += 1;
   }

   if(!(np->state.maxFlags & MAX_VERT)) {
      /* Inside top border. */
      segments[offset].x1 = west - 1;
      segments[offset].y1 = settings.borderWidth - 1;
      segments[offset].x2 = width - east;
      segments[offset].y2 = settings.borderWidth - 1;
      offset += 1;
   }

   if(!(np->state.maxFlags & MAX_HORIZ)) {
      /* Inside left border. */
      segments[offset].x1 = west - 1;
      segments[offset].y1 = south;
      segments[offset].x2 = west - 1;
      segments[offset].y2 = height - south;
      offset += 1;
   }

   if(!(np->state.maxFlags & MAX_HORIZ)) {
      /* Right border. */
      segments[offset].x1 = width - 1;
      segments[offset].y1 = 0;
      segments[offset].x2 = width - 1;
      segments[offset].y2 = height - 1;
      offset += 1;
      segments[offset].x1 = width - 2;
      segments[offset].y1 = 1;
      segments[offset].x2 = width - 2;
      segments[offset].y2 = height - 2;
      offset += 1;
   }

   if(!(np->state.maxFlags & MAX_VERT)) {
      /* Bottom border. */
      segments[offset].x1 = 0;
      segments[offset].y1 = height - 1;
      segments[offset].x2 = width;
      segments[offset].y2 = height - 1;
      offset += 1;
      segments[offset].x1 = 1;
      segments[offset].y1 = height - 2;
      segments[offset].x2 = width - 1;
      segments[offset].y2 = height - 2;
      offset += 1;
   }

   /* Draw pixel-down segments. */
   JXSetForeground(display, gc, pixelDown);
   JXDrawSegments(display, canvas, gc, segments, offset);
   offset = 0;

   /* Draw marks */
   if((np->state.border & BORDER_RESIZE)
      && !(np->state.status & STAT_SHADED)
      && !(np->state.maxFlags)) {

      /* Upper left */
      segments[0].x1 = settings.titleHeight + settings.borderWidth - 1;
      segments[0].y1 = 0;
      segments[0].x2 = settings.titleHeight + settings.borderWidth - 1;
      segments[0].y2 = settings.borderWidth;
      segments[1].x1 = 0;
      segments[1].y1 = settings.titleHeight + settings.borderWidth - 1;
      segments[1].x2 = settings.borderWidth;
      segments[1].y2 = settings.titleHeight + settings.borderWidth - 1;

      /* Upper right. */
      segments[2].x1 = width - settings.borderWidth;
      segments[2].y1 = settings.titleHeight + settings.borderWidth - 1;
      segments[2].x2 = width;
      segments[2].y2 = settings.titleHeight + settings.borderWidth - 1;
      segments[3].x1 = width - settings.titleHeight - settings.borderWidth - 1;
      segments[3].y1 = 0;
      segments[3].x2 = width - settings.titleHeight - settings.borderWidth - 1;
      segments[3].y2 = settings.borderWidth;

      /* Lower left */
      segments[4].x1 = 0;
      segments[4].y1 = height - settings.titleHeight - settings.borderWidth - 1;
      segments[4].x2 = settings.borderWidth;
      segments[4].y2 = height - settings.titleHeight - settings.borderWidth - 1;
      segments[5].x1 = settings.titleHeight + settings.borderWidth - 1;
      segments[5].y1 = height - settings.borderWidth;
      segments[5].x2 = settings.titleHeight + settings.borderWidth - 1;
      segments[5].y2 = height;

      /* Lower right */
      segments[6].x1 = width - settings.borderWidth;
      segments[6].y1 = height - settings.titleHeight - settings.borderWidth - 1;
      segments[6].x2 = width;
      segments[6].y2 = height - settings.titleHeight - settings.borderWidth - 1;
      segments[7].x1 = width - settings.titleHeight - settings.borderWidth - 1;
      segments[7].y1 = height - settings.borderWidth;
      segments[7].x2 = width - settings.titleHeight - settings.borderWidth - 1;
      segments[7].y2 = height;

      /* Draw pixel-down segments. */
      JXSetForeground(display, gc, pixelDown);
      JXDrawSegments(display, canvas, gc, segments, 8);

      /* Upper left */
      segments[0].x1 = settings.titleHeight + settings.borderWidth;
      segments[0].y1 = 0;
      segments[0].x2 = settings.titleHeight + settings.borderWidth;
      segments[0].y2 = settings.borderWidth;
      segments[1].x1 = 0;
      segments[1].y1 = settings.titleHeight + settings.borderWidth;
      segments[1].x2 = settings.borderWidth;
      segments[1].y2 = settings.titleHeight + settings.borderWidth;

      /* Upper right */
      segments[2].x1 = width - settings.titleHeight - settings.borderWidth;
      segments[2].y1 = 0;
      segments[2].x2 = width - settings.titleHeight - settings.borderWidth;
      segments[2].y2 = settings.borderWidth;
      segments[3].x1 = width - settings.borderWidth;
      segments[3].y1 = settings.titleHeight + settings.borderWidth;
      segments[3].x2 = width;
      segments[3].y2 = settings.titleHeight + settings.borderWidth;

      /* Lower left */
      segments[4].x1 = 0;
      segments[4].y1 = height - settings.titleHeight - settings.borderWidth;
      segments[4].x2 = settings.borderWidth;
      segments[4].y2 = height - settings.titleHeight - settings.borderWidth;
      segments[5].x1 = settings.titleHeight + settings.borderWidth;
      segments[5].y1 = height - settings.borderWidth;
      segments[5].x2 = settings.titleHeight + settings.borderWidth;
      segments[5].y2 = height;

      /* Lower right */
      segments[6].x1 = width - settings.borderWidth;
      segments[6].y1 = height - settings.titleHeight - settings.borderWidth;
      segments[6].x2 = width;
      segments[6].y2 = height - settings.titleHeight - settings.borderWidth;
      segments[7].x1 = width - settings.titleHeight - settings.borderWidth;
      segments[7].y1 = height - settings.borderWidth;
      segments[7].x2 = width - settings.titleHeight - settings.borderWidth;
      segments[7].y2 = height;

      /* Draw pixel-up segments. */
      JXSetForeground(display, gc, pixelUp);
      JXDrawSegments(display, canvas, gc, segments, 8);

   }
}

/** Determine the number of buttons to be displayed for a client. */
unsigned GetButtonCount(const ClientNode *np)
{

   int north, south, east, west;
   unsigned count;
   unsigned buttonWidth;
   int available;

   if(!(np->state.border & BORDER_TITLE)) {
      return 0;
   }
   if(settings.titleHeight <= settings.borderWidth) {
      return 0;
   }

   buttonWidth = settings.titleHeight;
   buttonWidth += settings.windowDecorations == DECO_MOTIF ? 1 : 0;

   GetBorderSize(&np->state, &north, &south, &east, &west);

   count = 0;
   available = np->width - buttonWidth;
   if(available < buttonWidth) {
      return count;
   }

   if(np->state.border & BORDER_CLOSE) {
      count += 1;
      available -= buttonWidth;
      if(available < buttonWidth) {
         return count;
      }
   }

   if(np->state.border & BORDER_MAX) {
      count += 1;
      available -= buttonWidth;
      if(available < buttonWidth) {
         return count;
      }
   }

   if(np->state.border & BORDER_MIN) {
      count += 1;
   }

   return count;
}

/** Draw the buttons on a client frame. */
void DrawBorderButtons(const ClientNode *np, Pixmap canvas, GC gc)
{
   long color;
   long pixelUp, pixelDown;
   int xoffset, yoffset;
   int north, south, east, west;
   int minx;

   GetBorderSize(&np->state, &north, &south, &east, &west);
   xoffset = np->width + west - settings.titleHeight;
   minx = settings.titleHeight + east;
   if(xoffset <= minx) {
      return;
   }

   /* Determine the colors to use. */
   if(np->state.status & (STAT_ACTIVE | STAT_FLASH)) {
      color = colors[COLOR_TITLE_ACTIVE_FG];
      pixelUp = colors[COLOR_TITLE_ACTIVE_UP];
      pixelDown = colors[COLOR_TITLE_ACTIVE_DOWN];
   } else {
      color = colors[COLOR_TITLE_FG];
      pixelUp = colors[COLOR_TITLE_UP];
      pixelDown = colors[COLOR_TITLE_DOWN];
   }

   if(settings.windowDecorations == DECO_MOTIF) {
      JXSetForeground(display, gc, pixelDown);
      JXDrawLine(display, canvas, gc,
                      west + settings.titleHeight - 1,
                      south,
                      west + settings.titleHeight - 1,
                      south + settings.titleHeight);
      JXSetForeground(display, gc, pixelUp);
      JXDrawLine(display, canvas, gc,
                 west + settings.titleHeight,
                 south,
                 west + settings.titleHeight,
                 south + settings.titleHeight);
   }

   /* Close button. */
   yoffset = settings.windowDecorations == DECO_MOTIF ? (south - 1) : 0;
   if(np->state.border & BORDER_CLOSE) {

      JXSetForeground(display, gc, color);
      DrawCloseButton(xoffset, yoffset, canvas, gc, color);

      if(settings.windowDecorations == DECO_MOTIF) {
         JXSetForeground(display, gc, pixelDown);
         JXDrawLine(display, canvas, gc, xoffset - 1,
                    south, xoffset - 1,
                    south + settings.titleHeight);
         JXSetForeground(display, gc, pixelUp);
         JXDrawLine(display, canvas, gc, xoffset,
                    south, xoffset, south + settings.titleHeight);
         xoffset -= 1;
      }

      xoffset -= settings.titleHeight;
      if(xoffset <= minx) {
         return;
      }
   }

   /* Maximize button. */
   if(np->state.border & BORDER_MAX) {

      JXSetForeground(display, gc, color);
      if(np->state.maxFlags) {
         DrawMaxAButton(xoffset, yoffset, canvas, gc, color);
      } else {
         DrawMaxIButton(xoffset, yoffset, canvas, gc, color);
      }

      if(settings.windowDecorations == DECO_MOTIF) {
         JXSetForeground(display, gc, pixelDown);
         JXDrawLine(display, canvas, gc, xoffset - 1,
                    south, xoffset - 1,
                    south + settings.titleHeight);
         JXSetForeground(display, gc, pixelUp);
         JXDrawLine(display, canvas, gc, xoffset,
                    south, xoffset, south + settings.titleHeight);
         xoffset -= 1;
      }

      xoffset -= settings.titleHeight;
      if(xoffset <= minx) {
         return;
      }
   }

   /* Minimize button. */
   if(np->state.border & BORDER_MIN) {

      JXSetForeground(display, gc, color);
      DrawMinButton(xoffset, yoffset, canvas, gc, color);

      if(settings.windowDecorations == DECO_MOTIF) {
         JXSetForeground(display, gc, pixelDown);
         JXDrawLine(display, canvas, gc, xoffset - 1,
                    south, xoffset - 1,
                    south + settings.titleHeight);
         JXSetForeground(display, gc, pixelUp);
         JXDrawLine(display, canvas, gc, xoffset,
                    south, xoffset, south + settings.titleHeight);
         xoffset -= 1;
      }
   }
}

/** Attempt to draw a border icon. */
char DrawBorderIcon(BorderIconType t,
                    unsigned xoffset, unsigned yoffset,
                    Pixmap canvas, long fg)
{
   if(buttonIcons[t]) {
      PutIcon(buttonIcons[t], canvas, fg, xoffset + 2, yoffset + 2,
              settings.titleHeight - 4, settings.titleHeight - 4);
      return 1;
   } else {
      return 0;
   }
}

/** Draw a close button. */
void DrawCloseButton(unsigned xoffset, unsigned yoffset,
                     Pixmap canvas, GC gc, long fg)
{
   XSegment segments[2];
   unsigned size;
   unsigned x1, y1;
   unsigned x2, y2;

   if(DrawBorderIcon(BI_CLOSE, xoffset, yoffset, canvas, fg)) {
      return;
   }

   size = (settings.titleHeight + 2) / 3;
   x1 = xoffset + settings.titleHeight / 2 - size / 2;
   y1 = yoffset + settings.titleHeight / 2 - size / 2;
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

   JXSetLineAttributes(display, gc, 2, LineSolid,
                       CapProjecting, JoinBevel);
   JXDrawSegments(display, canvas, gc, segments, 2);
   JXSetLineAttributes(display, gc, 1, LineSolid,
                       CapNotLast, JoinMiter);

}

/** Draw an inactive maximize button. */
void DrawMaxIButton(unsigned xoffset, unsigned yoffset,
                    Pixmap canvas, GC gc, long fg)
{

   XSegment segments[5];
   unsigned int size;
   unsigned int x1, y1;
   unsigned int x2, y2;

   if(DrawBorderIcon(BI_MAX, xoffset, yoffset, canvas, fg)) {
      return;
   }

   size = 2 + (settings.titleHeight + 2) / 3;
   x1 = xoffset + settings.titleHeight / 2 - size / 2;
   y1 = yoffset + settings.titleHeight / 2 - size / 2;
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

   JXSetLineAttributes(display, gc, 1, LineSolid,
                       CapProjecting, JoinMiter);
   JXDrawSegments(display, canvas, gc, segments, 5);
   JXSetLineAttributes(display, gc, 1, LineSolid,
                       CapButt, JoinMiter);

}

/** Draw an active maximize button. */
void DrawMaxAButton(unsigned xoffset, unsigned yoffset,
                    Pixmap canvas, GC gc, long fg)
{
   XSegment segments[8];
   unsigned size;
   unsigned x1, y1;
   unsigned x2, y2;
   unsigned x3, y3;

   if(DrawBorderIcon(BI_MAX_ACTIVE, xoffset, yoffset, canvas, fg)) {
      return;
   }

   size = 2 + (settings.titleHeight + 2) / 3;
   x1 = xoffset + settings.titleHeight / 2 - size / 2;
   y1 = yoffset + settings.titleHeight / 2 - size / 2;
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

   JXSetLineAttributes(display, gc, 1, LineSolid,
                       CapProjecting, JoinMiter);
   JXDrawSegments(display, canvas, gc, segments, 8);
   JXSetLineAttributes(display, gc, 1, LineSolid,
                       CapButt, JoinMiter);
}

/** Draw a minimize button. */
void DrawMinButton(unsigned xoffset, unsigned yoffset,
                   Pixmap canvas, GC gc, long fg)
{

   unsigned int size;
   unsigned int x1, y1;
   unsigned int x2, y2;

   if(DrawBorderIcon(BI_MIN, xoffset, yoffset, canvas, fg)) {
      return;
   }

   size = (settings.titleHeight + 2) / 3;
   x1 = xoffset + settings.titleHeight / 2 - size / 2;
   y1 = yoffset + settings.titleHeight / 2 - size / 2;
   x2 = x1 + size;
   y2 = y1 + size;
   JXSetLineAttributes(display, gc, 2, LineSolid,
                       CapProjecting, JoinMiter);
   JXDrawLine(display, canvas, gc, x1, y2, x2, y2);
   JXSetLineAttributes(display, gc, 1, LineSolid, CapButt, JoinMiter);

}

/** Redraw the borders on the current desktop.
 * This should be done after loading clients since the stacking order
 * may cause borders on the current desktop to become visible after moving
 * clients to their assigned desktops.
 */
void ExposeCurrentDesktop(void)
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

      if(state->border & BORDER_TITLE) {
         *north = settings.titleHeight;
      } else if(settings.windowDecorations == DECO_MOTIF) {
         *north = 0;
      } else {
         *north = settings.borderWidth;
      }
      if(state->maxFlags & MAX_VERT) {
         *south = 0;
      } else {
         if(settings.windowDecorations == DECO_MOTIF) {
            *north += settings.borderWidth;
            *south = settings.borderWidth;
         } else {
            if(state->status & STAT_SHADED) {
               *south = 0;
            } else {
               *south = settings.borderWidth;
            }
         }
      }

      if(state->maxFlags & MAX_HORIZ) {
         *east = 0;
         *west = 0;
      } else {
         *east = settings.borderWidth;
         *west = settings.borderWidth;
      }

   } else {

      *north = 0;
      *south = 0;
      *east = 0;
      *west = 0;

   }
}

/** Draw a rounded rectangle. */
void DrawRoundedRectangle(Drawable d, GC gc, int x, int y,
                          int width, int height, int radius)
{
#ifdef USE_SHAPE
#ifdef USE_XMU

   if(radius > 0) {
      XmuDrawRoundedRectangle(display, d, gc, x, y, width, height,
                              radius, radius);
   } else {
      JXDrawRectangle(display, d, gc, x, y, width, height);
   }

#else

   if(radius > 0) {
      XSegment segments[4];
      XArc     arcs[4];

      segments[0].x1 = x + radius;         segments[0].y1 = y;
      segments[0].x2 = x + width - radius; segments[0].y2 = y;
      segments[1].x1 = x + radius;         segments[1].y1 = y + height;
      segments[1].x2 = x + width - radius; segments[1].y2 = y + height;
      segments[2].x1 = x;                  segments[2].y1 = y + radius;
      segments[2].x2 = x;                  segments[2].y2 = y + height - radius;
      segments[3].x1 = x + width;          segments[3].y1 = y + radius;
      segments[3].x2 = x + width;          segments[3].y2 = y + height - radius;
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
   } else {
      JXDrawRectangle(display, d, gc, x, y, width, height);
   }

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

/** Set the icon to use for a button. */
void SetBorderIcon(BorderIconType t, const char *name)
{
   if(buttonNames[t]) {
      Release(buttonNames[t]);
   }
   buttonNames[t] = CopyString(name);
}
 
