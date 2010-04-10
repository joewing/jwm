/**
 * @file place.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Client placement functions.
 *
 */

#include "jwm.h"
#include "place.h"
#include "client.h"
#include "screen.h"
#include "border.h"
#include "tray.h"
#include "main.h"

typedef struct Strut {
   ClientNode *client;
   BoundingBox box;
   struct Strut *prev;
   struct Strut *next;
} Strut;

static Strut *struts = NULL;
static Strut *strutsTail = NULL;

/* desktopCount x screenCount */
/* Note that we assume x and y are 0 based for all screens here. */
static int *cascadeOffsets = NULL;

static void SubtractStrutBounds(BoundingBox *box);
static void SubtractBounds(const BoundingBox *src, BoundingBox *dest);

/** Initialize placement data. */
void InitializePlacement() {
}

/** Startup placement. */
void StartupPlacement() {

   int count;
   int x;

   count = desktopCount * GetScreenCount();
   cascadeOffsets = Allocate(count * sizeof(int));

   for(x = 0; x < count; x++) {
      cascadeOffsets[x] = borderWidth + titleHeight;
   }

}

/** Shutdown placement. */
void ShutdownPlacement() {

   Strut *sp;

   Release(cascadeOffsets);

   while(struts) {
      sp = struts->next;
      Release(struts);
      struts = sp;
   }
   strutsTail = NULL;

}

/** Destroy placement data. */
void DestroyPlacement() {
}

/** Remove struts associated with a client. */
void RemoveClientStrut(ClientNode *np) {

   Strut *sp;

   sp = struts;
   while(sp) {
      if(sp->client == np) {
         if(sp->prev) {
            sp->prev->next = sp->next;
         } else {
            struts = sp->next;
         }
         if(sp->next) {
            sp->next->prev = sp->prev;
         } else {
            strutsTail = sp->prev;
         }
         Release(sp);
         sp = struts;
      } else {
         sp = sp->next;
      }
   }

}

/** Add client specified struts to our list. */
void ReadClientStrut(ClientNode *np) {

   BoundingBox box;
   Strut *sp;
   int status;
   Atom actualType;
   int actualFormat;
   unsigned long count;
   unsigned long bytesLeft;
   unsigned char *value;
   long *lvalue;
   long leftWidth, rightWidth, topHeight, bottomHeight;
   long leftStart, leftEnd, rightStart, rightEnd;
   long topStart, topEnd, bottomStart, bottomEnd;

   RemoveClientStrut(np);

   box.x = 0;
   box.y = 0;
   box.width = 0;
   box.height = 0;

   /* First try to read _NET_WM_STRUT_PARTIAL */
   /* Format is:
    *   left_width, right_width, top_width, bottom_width,
    *   left_start_y, left_end_y, right_start_y, right_end_y,
    *   top_start_x, top_end_x, bottom_start_x, bottom_end_x
    */
   status = JXGetWindowProperty(display, np->window,
      atoms[ATOM_NET_WM_STRUT_PARTIAL], 0, 12, False, XA_CARDINAL,
      &actualType, &actualFormat, &count, &bytesLeft, &value);
   if(status == Success) {
      if(count == 12) {
         lvalue = (long*)value;
         leftWidth = lvalue[0];
         rightWidth = lvalue[1];
         topHeight = lvalue[2];
         bottomHeight = lvalue[3];
         leftStart = lvalue[4];
         leftEnd = lvalue[5];
         rightStart = lvalue[6];
         rightEnd = lvalue[7];
         topStart = lvalue[8];
         topEnd = lvalue[9];
         bottomStart = lvalue[10];
         bottomEnd = lvalue[11];

         if(leftWidth > 0) {
            box.width = leftWidth;
            box.x = leftStart;
         }

         if(rightWidth > 0) {
            box.width = rightWidth;
            box.x = rightStart;
         }

         if(topHeight > 0) {
            box.height = topHeight;
            box.y = topStart;
         }

         if(bottomHeight > 0) {
            box.height = bottomHeight;
            box.y = bottomStart;
         }

         if(box.width == 0 && box.height > 0) {
            box.width = rootWidth;
         }
         if(box.height == 0 && box.width > 0) {
            box.height = rootHeight;
         }

         sp = Allocate(sizeof(Strut));
         sp->client = np;
         sp->box = box;
         sp->prev = NULL;
         sp->next = struts;
         if(struts) {
            struts->prev = sp;
         } else {
            strutsTail = sp;
         }
         struts = sp;

      }
      JXFree(value);
      return;
   }

   /* Next try to read _NET_WM_STRUT */
   /* Format is: left_width, right_width, top_width, bottom_width */
   status = JXGetWindowProperty(display, np->window,
      atoms[ATOM_NET_WM_STRUT], 0, 4, False, XA_CARDINAL,
      &actualType, &actualFormat, &count, &bytesLeft, &value);
   if(status == Success) {
      if(count == 4) {
         lvalue = (long*)value;
         leftWidth = lvalue[0];
         rightWidth = lvalue[1];
         topHeight = lvalue[2];
         bottomHeight = lvalue[3];

         if(leftWidth > 0) {
            box.x = 0;
            box.width = leftWidth;
         }

         if(rightWidth > 0) {
            box.x = rootWidth - rightWidth;
            box.width = rightWidth;
         }

         if(topHeight > 0) {
            box.y = 0;
            box.height = topHeight;
         }

         if(bottomHeight > 0) {
            box.y = rootHeight - bottomHeight;
            box.height = bottomHeight;
         }

         sp = Allocate(sizeof(Strut));
         sp->client = np;
         sp->box = box;
         sp->prev = NULL;
         sp->next = struts;
         if(struts) {
            struts->prev = sp;
         } else {
            strutsTail = sp;
         }
         struts = sp;

      }
      JXFree(value);
      return;
   }

}

/** Get the screen bounds. */
void GetScreenBounds(const ScreenType *sp, BoundingBox *box) {

   box->x = sp->x;
   box->y = sp->y;
   box->width = sp->width;
   box->height = sp->height;

}

/** Shrink dest such that it does not intersect with src. */
void SubtractBounds(const BoundingBox *src, BoundingBox *dest) {

   BoundingBox boxes[4];

   if(src->x + src->width <= dest->x) {
      return;
   }
   if(src->y + src->height <= dest->y) {
      return;
   }
   if(dest->x + dest->width <= src->x) {
      return;
   }
   if(dest->y + dest->height <= src->y) {
      return;
   }

   /* There are four ways to do this:
    *  0. Increase the x-coordinate and decrease the width of dest.
    *  1. Increase the y-coordinate and decrease the height of dest.
    *  2. Decrease the width of dest.
    *  3. Decrease the height of dest.
    * We will chose the option which leaves the greatest area.
    * Note that negative areas are possible.
    */

   /* 0 */
   boxes[0] = *dest;
   boxes[0].x = src->x + src->width;
   boxes[0].width = dest->x + dest->width - boxes[0].x;

   /* 1 */
   boxes[1] = *dest;
   boxes[1].y = src->y + src->height;
   boxes[1].height = dest->y + dest->height - boxes[1].y;

   /* 2 */
   boxes[2] = *dest;
   boxes[2].width = src->x - dest->x;

   /* 3 */
   boxes[3] = *dest;
   boxes[3].height = src->y - dest->y;

   /* 0 and 1, winner in 0. */
   if(boxes[0].width * boxes[0].height < boxes[1].width * boxes[1].height) {
      boxes[0] = boxes[1];
   }

   /* 2 and 3, winner in 2. */
   if(boxes[2].width * boxes[2].height < boxes[3].width * boxes[3].height) {
      boxes[2] = boxes[3];
   }

   /* 0 and 2, winner in dest. */
   if(boxes[0].width * boxes[0].height < boxes[2].width * boxes[2].height) {
      *dest = boxes[2];
   } else {
      *dest = boxes[0];
   }

}

/** Subtract tray area from the bounding box. */
void SubtractTrayBounds(const TrayType *tp, BoundingBox *box,
   unsigned int layer) {

   BoundingBox src;
   BoundingBox last;

   for(; tp; tp = tp->next) {

      if(tp->layer > layer && !tp->autoHide) {

         src.x = tp->x;
         if(src.x < 0) {
            src.x = rootWidth - src.x;
         }
         src.y = tp->y;
         if(src.y < 0) {
            src.y = rootHeight - src.y;
         }
         src.width = tp->width;
         src.height = tp->height;

         last = *box;
         SubtractBounds(&src, box);
         if(box->width * box->height <= 0) {
            *box = last;
            break;
         }

      }

   }

}

/** Remove struts from the bounding box. */
void SubtractStrutBounds(BoundingBox *box) {

   Strut *sp;
   BoundingBox last;

   for(sp = struts; sp; sp = sp->next) {
      if(sp->client->state.desktop == currentDesktop
         || (sp->client->state.status & STAT_STICKY)) {
         last = *box;
         SubtractBounds(&sp->box, box);
         if(box->width * box->height <= 0) {
            *box = last;
            break;
         }
      }
   }

}

/** Place a client on the screen. */
void PlaceClient(ClientNode *np, int alreadyMapped) {

   BoundingBox box;
   int north, south, east, west;
   const ScreenType *sp;
   int cascadeIndex;
   int overflow;

   Assert(np);

   GetBorderSize(np, &north, &south, &east, &west);

   if(np->x + np->width > rootWidth || np->y + np->height > rootHeight) {
      overflow = 1;
   } else {
      overflow = 0;
   }

   sp = GetMouseScreen();
   GetScreenBounds(sp, &box);

   if(!overflow && (alreadyMapped
      || (!(np->state.status & STAT_PIGNORE)
      && (np->sizeFlags & (PPosition | USPosition))))) {

      GravitateClient(np, 0);

   } else {

      SubtractTrayBounds(GetTrays(), &box, np->state.layer);
      SubtractStrutBounds(&box);

      cascadeIndex = sp->index * desktopCount + currentDesktop;

      /* Set the cascaded location. */
      np->x = box.x + west + cascadeOffsets[cascadeIndex];
      np->y = box.y + north + cascadeOffsets[cascadeIndex];
      cascadeOffsets[cascadeIndex] += borderWidth + titleHeight;

      /* Check for cascade overflow. */
      overflow = 0;
      if(np->x + np->width - box.x > box.width) {
         overflow = 1;
      } else if(np->y + np->height - box.y > box.height) {
         overflow = 1;
      }

      if(overflow) {

         cascadeOffsets[cascadeIndex] = borderWidth + titleHeight;
         np->x = box.x + west + cascadeOffsets[cascadeIndex];
         np->y = box.y + north + cascadeOffsets[cascadeIndex];

         /* Check for client overflow. */
         overflow = 0;
         if(np->x + np->width - box.x > box.width) {
            overflow = 1;
         } else if(np->y + np->height - box.y > box.height) {
            overflow = 1;
         }

         /* Update cascade position or position client. */
         if(overflow) {
            np->x = box.x + west;
            np->y = box.y + north;
         } else {
            cascadeOffsets[cascadeIndex] += borderWidth + titleHeight;
         }

      }

   }

   if(np->state.status & STAT_FULLSCREEN) {
      JXMoveWindow(display, np->parent, sp->x, sp->y);
   } else {
      JXMoveWindow(display, np->parent, np->x - west, np->y - north);
   }

}

/** Constrain the size of the client so it fits. */
void ConstrainSize(ClientNode *np) {

   BoundingBox box;
   const ScreenType *sp;
   int north, south, east, west;
   float ratio, minr, maxr;

   Assert(np);

   /* Determine if the size needs to be constrained. */
   sp = GetCurrentScreen(np->x, np->y);
   if(np->width < sp->width && np->height < sp->height) {
      return;
   }

   /* Constrain the size. */
   GetBorderSize(np, &north, &south, &east, &west);

   GetScreenBounds(sp, &box);
   SubtractTrayBounds(GetTrays(), &box, np->state.layer);
   SubtractStrutBounds(&box);

   box.x += west;
   box.y += north;
   box.width -= east + west;
   box.height -= north + south;

   if(box.width > np->maxWidth) {
      box.width = np->maxWidth;
   }
   if(box.height > np->maxHeight) {
      box.height = np->maxHeight;
   }

   if(np->sizeFlags & PAspect) {

      ratio = (float)box.width / box.height;

      minr = (float)np->aspect.minx / np->aspect.miny;
      if(ratio < minr) {
         box.height = (int)((float)box.width / minr);
      }

      maxr = (float)np->aspect.maxx / np->aspect.maxy;
      if(ratio > maxr) {
         box.width = (int)((float)box.height * maxr);
      }

   }

   np->x = box.x;
   np->y = box.y;
   np->width = box.width - (box.width % np->xinc);
   np->height = box.height - (box.height % np->yinc);

}

/** Place a maximized client on the screen. */
void PlaceMaximizedClient(ClientNode *np, int horiz, int vert) {

   BoundingBox box;
   const ScreenType *sp;
   int north, south, east, west;
   float ratio, minr, maxr;

   np->oldx = np->x;
   np->oldy = np->y;
   np->oldWidth = np->width;
   np->oldHeight = np->height;

   GetBorderSize(np, &north, &south, &east, &west);

   sp = GetCurrentScreen(
      np->x + (east + west + np->width) / 2,
      np->y + (north + south + np->height) / 2);
   GetScreenBounds(sp, &box);
   SubtractTrayBounds(GetTrays(), &box, np->state.layer);
   SubtractStrutBounds(&box);

   box.x += west;
   box.y += north;
   box.width -= east + west;
   box.height -= north + south;

   if(box.width > np->maxWidth) {
      box.width = np->maxWidth;
   }
   if(box.height > np->maxHeight) {
      box.height = np->maxHeight;
   }

   if(np->sizeFlags & PAspect) {

      ratio = (float)box.width / box.height;

      minr = (float)np->aspect.minx / np->aspect.miny;
      if(ratio < minr) {
         box.height = (int)((float)box.width / minr);
      }

      maxr = (float)np->aspect.maxx / np->aspect.maxy;
      if(ratio > maxr) {
         box.width = (int)((float)box.height * maxr);
      }

   }

   /* If maximizing horizontally, update width. */
   if(horiz) {
      np->x = box.x;
      np->width = box.width - ((box.width - np->baseWidth) % np->xinc);
      np->state.status |= STAT_HMAX;
   }

   /* If maximizing vertically, update height. */
   if(vert) {
      np->y = box.y;
      np->height = box.height - ((box.height - np->baseHeight) % np->yinc);
      np->state.status |= STAT_VMAX;
   }

}

/** Determine which way to move the client for the border. */
void GetGravityDelta(const ClientNode *np, int *x, int  *y) {

   int north, south, east, west;

   Assert(np);
   Assert(x);
   Assert(y);

   GetBorderSize(np, &north, &south, &east, &west);

   switch(np->gravity) {
   case NorthWestGravity:
      *y = -north;
      *x = -west;
      break;
   case NorthGravity:
      *y = -north;
      break;
   case NorthEastGravity:
      *y = -north;
      *x = west;
      break;
   case WestGravity:
      *x = -west;
      break;
   case CenterGravity:
      *y = (north + south) / 2;
      *x = (east + west) / 2;
      break;
   case EastGravity:
      *x = west;
      break;
   case SouthWestGravity:
      *y = south;
      *x = -west;
      break;
   case SouthGravity:
      *y = south;
      break;
   case SouthEastGravity:
      *y = south;
      *x = west;
      break;
   default: /* Static */
      *x = 0;
      *y = 0;
      break;
   }

}

/** Move the window in the specified direction for reparenting. */
void GravitateClient(ClientNode *np, int negate) {

   int deltax, deltay;

   Assert(np);

   GetGravityDelta(np, &deltax, &deltay);

   if(negate) {
      np->x += deltax;
      np->y += deltay;
   } else {
      np->x -= deltax;
      np->y -= deltay;
   }

}

