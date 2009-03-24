/**
 * @file button.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions to handle drawing buttons.
 *
 */

#include "jwm.h"
#include "button.h"
#include "font.h"
#include "color.h"
#include "main.h"
#include "icon.h"
#include "image.h"
#include "gradient.h"

static void GetScaledIconSize(IconNode *ip, int maxsize,
   int *width, int *height);

/** Draw a button. */
void DrawButton(ButtonNode *bp) {

   long outlinePixel;
   long topPixel, bottomPixel;
   ColorType fg;
   long bg1, bg2;

   Drawable drawable;
   GC gc;
   int x, y;
   int width, height;
   int xoffset, yoffset;

   int iconWidth, iconHeight;
   int textWidth, textHeight;
   
   Assert(bp);

   drawable = bp->drawable;
   gc = bp->gc;
   x = bp->x;
   y = bp->y;
   width = bp->width;
   height = bp->height;

   /* Determine the colors to use. */
   switch(bp->type) {
   case BUTTON_LABEL:
      fg = COLOR_MENU_FG;
      bg1 = colors[COLOR_MENU_BG];
      bg2 = colors[COLOR_MENU_BG];
      outlinePixel = colors[COLOR_MENU_BG];
      topPixel = colors[COLOR_MENU_BG];
      bottomPixel = colors[COLOR_MENU_BG];
      break;
   case BUTTON_MENU_ACTIVE:
      fg = COLOR_MENU_ACTIVE_FG;
      bg1 = colors[COLOR_MENU_ACTIVE_BG1];
      bg2 = colors[COLOR_MENU_ACTIVE_BG2];
      if(bg1 == bg2) {
         outlinePixel = colors[COLOR_MENU_ACTIVE_OL];
      } else {
         outlinePixel = colors[COLOR_MENU_ACTIVE_DOWN];
      }
      topPixel = colors[COLOR_MENU_ACTIVE_UP];
      bottomPixel = colors[COLOR_MENU_ACTIVE_DOWN];
      break;
   case BUTTON_TASK:
      fg = COLOR_TASK_FG;
      bg1 = colors[COLOR_TASK_BG1];
      bg2 = colors[COLOR_TASK_BG2];
      topPixel = colors[COLOR_TASK_UP];
      bottomPixel = colors[COLOR_TASK_DOWN];
      outlinePixel = bottomPixel;
      break;
   case BUTTON_TASK_ACTIVE:
      fg = COLOR_TASK_ACTIVE_FG;
      bg1 = colors[COLOR_TASK_ACTIVE_BG1];
      bg2 = colors[COLOR_TASK_ACTIVE_BG2];
      topPixel = colors[COLOR_TASK_ACTIVE_DOWN];
      bottomPixel = colors[COLOR_TASK_ACTIVE_UP];
      outlinePixel = bottomPixel;
      break;
   case BUTTON_MENU:
   default:
      fg = COLOR_MENU_FG;
      bg1 = colors[COLOR_MENU_BG];
      bg2 = colors[COLOR_MENU_BG];
      outlinePixel = colors[COLOR_MENU_DOWN];
      topPixel = colors[COLOR_MENU_UP];
      bottomPixel = colors[COLOR_MENU_DOWN];
      break;
   }

   /* Draw the background. */
   switch(bp->type) {
   case BUTTON_TASK:
      /* Flat taskbuttons (only icon) for widths < 48 */
      if(width < 48) {
         break;
      }
      /* conditional fallthrough is intended */  
   default:

      /* Draw the button background. */
      JXSetForeground(display, gc, bg1);
      if(bg1 == bg2) {
         /* single color */
         JXFillRectangle(display, drawable, gc,
            x + 1, y + 1, width - 1, height - 1);
      } else {
         /* gradient */
         DrawHorizontalGradient(drawable, gc, bg1, bg2,
            x + 1, y + 1, width - 2, height - 1);
      }

      /* Draw the outline. */
      JXSetForeground(display, gc, outlinePixel);
#ifdef USE_XMU
      XmuDrawRoundedRectangle(display, drawable, gc, x, y, 
         width, height, 3, 3);
#else
      JXDrawRectangle(display, drawable, gc, x, y, width, height);
#endif

      break;
   }

   /* Determine the size of the icon (if any) to display. */
   iconWidth = 0;
   iconHeight = 0;
   if (bp->icon) {
      if(width < height) {
         GetScaledIconSize(bp->icon, width - 5, &iconWidth, &iconHeight);
      } else {
         GetScaledIconSize(bp->icon, height - 5, &iconWidth, &iconHeight);
      }
   }

   /* Determine how much room is left for text. */
   textWidth = 0;
   textHeight = 0;
   if(bp->text) {
      textWidth = GetStringWidth(bp->font, bp->text);
      textHeight = GetStringHeight(bp->font);
      if(textWidth + iconWidth + 8 > width) {
         textWidth = width - iconWidth - 8;
         if(textWidth < 0) {
            textWidth = 0;
         }
      }
   }

   /* Determine the offset of the text in the button. */
   switch(bp->alignment) {
   case ALIGN_CENTER:
      xoffset = width / 2 - (iconWidth + textWidth) / 2;
      if(xoffset < 0) {
         xoffset = 0;
      }
      break;
   case ALIGN_LEFT:
   default:
      xoffset = 3;
      break;
   }

   /* Display the icon. */
   if(bp->icon) {
      yoffset = height / 2 - iconHeight / 2;
      PutIcon(bp->icon, drawable, x + xoffset, y + yoffset,
         iconWidth, iconHeight);
      xoffset += iconWidth + 2;
   }

   /* Display the label. */
   if(bp->text && textWidth) {
      yoffset = height / 2 - textHeight / 2;
      RenderString(drawable, bp->font, fg, x + xoffset, y + yoffset,
         textWidth, NULL, bp->text);
   }

}

/** Reset a button node with default values. */
void ResetButton(ButtonNode *bp, Drawable d, GC g) {

   Assert(bp);

   bp->type = BUTTON_MENU;
   bp->drawable = d;
   bp->gc = g;
   bp->font = FONT_TRAY;
   bp->alignment = ALIGN_LEFT;
   bp->x = 0;
   bp->y = 0;
   bp->width = 1;
   bp->height = 1;
   bp->icon = NULL;
   bp->text = NULL;

}

/** Get the scaled size of an icon for a button. */
void GetScaledIconSize(IconNode *ip, int maxsize,
   int *width, int *height) {

   double ratio;

   Assert(ip);
   Assert(width);
   Assert(height);

   /* width to height */
   Assert(ip->image->height > 0);
   ratio = (double)ip->image->width / ip->image->height;

   if(ip->image->width > ip->image->height) {

      /* Compute size wrt width */
      *width = maxsize * ratio;
      *height = *width / ratio;

   } else {

      /* Compute size wrt height */
      *height = maxsize / ratio;
      *width = *height * ratio;

   }

}

