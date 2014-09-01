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
#include "border.h"
#include "font.h"
#include "color.h"
#include "main.h"
#include "icon.h"
#include "image.h"
#include "gradient.h"

static void GetScaledIconSize(IconNode *ip, int maxsize,
                              int *width, int *height);

/** Draw a button. */
void DrawButton(ButtonNode *bp)
{

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
   x = bp->x;
   y = bp->y;
   width = bp->width;
   height = bp->height;
   gc = JXCreateGC(display, drawable, 0, NULL);

   /* Determine the colors to use. */
   switch(bp->type) {
   case BUTTON_LABEL:
      fg = COLOR_MENU_FG;
      bg1 = colors[COLOR_MENU_BG];
      bg2 = colors[COLOR_MENU_BG];
      break;
   case BUTTON_MENU_ACTIVE:
      fg = COLOR_MENU_ACTIVE_FG;
      bg1 = colors[COLOR_MENU_ACTIVE_BG1];
      bg2 = colors[COLOR_MENU_ACTIVE_BG2];
      break;
   case BUTTON_TRAY:
      fg = COLOR_TRAYBUTTON_FG;
      bg1 = colors[COLOR_TRAYBUTTON_BG1];
      bg2 = colors[COLOR_TRAYBUTTON_BG2];
      break;
   case BUTTON_TRAY_ACTIVE:
      fg = COLOR_TRAYBUTTON_ACTIVE_FG;
      bg1 = colors[COLOR_TRAYBUTTON_ACTIVE_BG1];
      bg2 = colors[COLOR_TRAYBUTTON_ACTIVE_BG2];
      break;
   case BUTTON_TASK:
      fg = COLOR_TASK_FG;
      bg1 = colors[COLOR_TASK_BG1];
      bg2 = colors[COLOR_TASK_BG2];
      break;
   case BUTTON_TASK_ACTIVE:
      fg = COLOR_TASK_ACTIVE_FG;
      bg1 = colors[COLOR_TASK_ACTIVE_BG1];
      bg2 = colors[COLOR_TASK_ACTIVE_BG2];
      break;
   case BUTTON_MENU:
   default:
      fg = COLOR_MENU_FG;
      bg1 = colors[COLOR_MENU_BG];
      bg2 = colors[COLOR_MENU_BG];
      break;
   }

   /* Draw the background. */
   if(bp->fill) {

      /* Draw the button background. */
      JXSetForeground(display, gc, bg1);
      if(bg1 == bg2) {
         /* single color */
         JXFillRectangle(display, drawable, gc, x, y, width, height);
      } else {
         /* gradient */
         DrawHorizontalGradient(drawable, gc, bg1, bg2,
                                x, y, width, height);
      }

   }

   /* Determine the size of the icon (if any) to display. */
   iconWidth = 0;
   iconHeight = 0;
   if(bp->icon) {
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
   if(bp->alignment == ALIGN_CENTER) {
      xoffset = (width - iconWidth - textWidth + 1) / 2;
      if(xoffset < 0) {
         xoffset = 0;
      }
   } else {
      xoffset = 3;
   }

   /* Display the icon. */
   if(bp->icon) {
      yoffset = (height - iconHeight + 1) / 2;
      PutIcon(bp->visual, bp->icon, drawable, colors[fg],
              x + xoffset, y + yoffset,
              iconWidth, iconHeight);
      xoffset += iconWidth + 2;
   }

   /* Display the label. */
   if(bp->text && textWidth) {
      yoffset = (height - textHeight + 1) / 2;
      RenderString(bp->visual, drawable, bp->font, fg,
                   x + xoffset, y + yoffset,
                   textWidth, bp->text);
   }

   JXFreeGC(display, gc);

}

/** Reset a button node with default values. */
void ResetButton(ButtonNode *bp, Drawable d, const VisualData *visual)
{

   Assert(bp);

   bp->type = BUTTON_MENU;
   bp->visual = visual;
   bp->drawable = d;
   bp->font = FONT_TRAY;
   bp->alignment = ALIGN_LEFT;
   bp->x = 0;
   bp->y = 0;
   bp->width = 1;
   bp->height = 1;
   bp->icon = NULL;
   bp->text = NULL;
   bp->fill = 1;

}

/** Get the scaled size of an icon for a button. */
void GetScaledIconSize(IconNode *ip, int maxsize,
                       int *width, int *height)
{

   int ratio;

   Assert(ip);
   Assert(width);
   Assert(height);

   if(ip == &emptyIcon) {
      *width = maxsize;
      *height = maxsize;
      return;
   }

   Assert(ip->image->height > 0);

   /* Fixed point with 16-bit fraction. */
   ratio = (ip->image->width << 16) / ip->image->height;

   if(ip->image->width > ip->image->height) {

      /* Compute size wrt width */
      *width = maxsize;
      *height = (*width << 16) / ratio;

   } else {

      /* Compute size wrt height */
      *height = maxsize;
      *width = (*height * ratio) >> 16;

   }

}

