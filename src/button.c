/**
 * @file button.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions for rendering buttons.
 *
 */

#include "jwm.h"
#include "button.h"
#include "border.h"
#include "main.h"
#include "icon.h"
#include "image.h"
#include "misc.h"
#include "settings.h"

/** Draw a button. */
void DrawButton(ButtonNode *bp)
{

   ColorType fg;
   long bg1, bg2;
   long up, down;
   DecorationsType decorations;

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
      up = colors[COLOR_MENU_UP];
      down = colors[COLOR_MENU_DOWN];
      decorations = settings.menuDecorations;
      break;
   case BUTTON_MENU_ACTIVE:
      fg = COLOR_MENU_ACTIVE_FG;
      bg1 = colors[COLOR_MENU_ACTIVE_BG1];
      bg2 = colors[COLOR_MENU_ACTIVE_BG2];
      down = colors[COLOR_MENU_ACTIVE_UP];
      up = colors[COLOR_MENU_ACTIVE_DOWN];
      decorations = settings.menuDecorations;
      break;
   case BUTTON_TRAY:
      fg = COLOR_TRAYBUTTON_FG;
      bg1 = colors[COLOR_TRAYBUTTON_BG1];
      bg2 = colors[COLOR_TRAYBUTTON_BG2];
      up = colors[COLOR_TRAYBUTTON_UP];
      down = colors[COLOR_TRAYBUTTON_DOWN];
      decorations = settings.trayDecorations;
      break;
   case BUTTON_TRAY_ACTIVE:
      fg = COLOR_TRAYBUTTON_ACTIVE_FG;
      bg1 = colors[COLOR_TRAYBUTTON_ACTIVE_BG1];
      bg2 = colors[COLOR_TRAYBUTTON_ACTIVE_BG2];
      down = colors[COLOR_TRAYBUTTON_ACTIVE_UP];
      up = colors[COLOR_TRAYBUTTON_ACTIVE_DOWN];
      decorations = settings.trayDecorations;
      break;
   case BUTTON_TASK:
      fg = COLOR_TASKLIST_FG;
      bg1 = colors[COLOR_TASKLIST_BG1];
      bg2 = colors[COLOR_TASKLIST_BG2];
      up = colors[COLOR_TASKLIST_UP];
      down = colors[COLOR_TASKLIST_DOWN];
      decorations = settings.taskListDecorations;
      break;
   case BUTTON_TASK_ACTIVE:
      fg = COLOR_TASKLIST_ACTIVE_FG;
      bg1 = colors[COLOR_TASKLIST_ACTIVE_BG1];
      bg2 = colors[COLOR_TASKLIST_ACTIVE_BG2];
      down = colors[COLOR_TASKLIST_ACTIVE_UP];
      up = colors[COLOR_TASKLIST_ACTIVE_DOWN];
      decorations = settings.taskListDecorations;
      break;
   case BUTTON_MENU:
   default:
      fg = COLOR_MENU_FG;
      bg1 = colors[COLOR_MENU_BG];
      bg2 = colors[COLOR_MENU_BG];
      up = colors[COLOR_MENU_UP];
      down = colors[COLOR_MENU_DOWN];
      decorations = settings.menuDecorations;
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
         DrawGradient(drawable, gc, bg1, bg2,
                                x, y, width, height, GRADIENT_VERTICAL);
      }

   }

   /* Draw the border. */
   if(bp->border) {
      if(decorations == DECO_MOTIF) {
         JXSetForeground(display, gc, up);
         JXDrawLine(display, drawable, gc, x, y, x + width - 1, y);
         JXDrawLine(display, drawable, gc, x, y, x, y + height - 1);
         JXSetForeground(display, gc, down);
         JXDrawLine(display, drawable, gc, x, y + height - 1,
                    x + width - 1, y + height - 1);
         JXDrawLine(display, drawable, gc, x + width - 1, y,
                    x + width - 1, y + height - 1);
      } else {
         JXSetForeground(display, gc, down);
         JXDrawRectangle(display, drawable, gc, x, y, width - 1, height - 1);
      }
   }

   /* Determine the size of the icon (if any) to display. */
   iconWidth = 0;
   iconHeight = 0;
   if(bp->icon) {
      if(!bp->icon->width || !bp->icon->height) {
         iconWidth = Min(width - BUTTON_BORDER * 2, height - BUTTON_BORDER * 2);
         iconHeight = iconWidth;
      } else {
         const int ratio = (bp->icon->width << 16) / bp->icon->height;
         int maxIconWidth = width - BUTTON_BORDER * 2;
         if(bp->text) {
            /* Showing text, keep the icon square. */
            maxIconWidth = Min(width, height) - BUTTON_BORDER * 2;
         }
         iconHeight = height - BUTTON_BORDER * 2;
         iconWidth = (iconHeight * ratio) >> 16;
         if(iconWidth > maxIconWidth) {
            iconWidth = maxIconWidth;
            iconHeight = (iconWidth << 16) / ratio;
         }
      }
   }

   /* Determine how much room is left for text. */
   textWidth = 0;
   textHeight = 0;
   if(bp->text && (width > height || !bp->icon)) {
      const int borderWidth = BUTTON_BORDER * (bp->icon ? 3 : 2);
      textWidth = GetStringWidth(bp->font, bp->text);
      textHeight = GetStringHeight(bp->font);
      if(textWidth + iconWidth + borderWidth > width) {
         textWidth = width - iconWidth - borderWidth;
      }
      textWidth = textWidth < 0 ? 0 : textWidth;
   }

   /* Determine the offset of the text in the button. */
   if(bp->alignment == ALIGN_CENTER || width <= height) {
      xoffset = (width - iconWidth - textWidth + 1) / 2;
      if(xoffset < 0) {
         xoffset = 0;
      }
   } else {
      xoffset = BUTTON_BORDER;
   }

   /* Display the icon. */
   if(bp->icon) {
      yoffset = (height - iconHeight + 1) / 2;
      PutIcon(bp->icon, drawable, colors[fg],
              x + xoffset, y + yoffset,
              iconWidth, iconHeight);
      xoffset += iconWidth + BUTTON_BORDER;
   }

   /* Display the label. */
   if(textWidth > 0) {
      yoffset = (height - textHeight + 1) / 2;
      RenderString(drawable, bp->font, fg,
                   x + xoffset, y + yoffset,
                   textWidth, bp->text);
   }

   JXFreeGC(display, gc);

}

/** Reset a button node with default values. */
void ResetButton(ButtonNode *bp, Drawable d)
{

   Assert(bp);

   bp->type = BUTTON_MENU;
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
   bp->border = 0;

}
