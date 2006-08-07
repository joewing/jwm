/***************************************************************************
 * Functions to handle drawing buttons.
 * Copyright (C) 2004 Joe Wingbermuehle
 ***************************************************************************/

#include "jwm.h"
#include "button.h"
#include "font.h"
#include "color.h"
#include "main.h"
#include "icon.h"
#include "image.h"

static void GetScaledIconSize(IconNode *ip, int maxsize,
	int *width, int *height);

/***************************************************************************
 ***************************************************************************/
void DrawButton(ButtonNode *bp) {

	long outlinePixel;
	long topPixel, bottomPixel;
	ColorType fg, bg;

	Drawable drawable;
	GC gc;
	int x, y;
	int width, height;
	int xoffset, yoffset;

	int iconWidth, iconHeight;
	int textWidth, textHeight;

	drawable = bp->drawable;
	gc = bp->gc;
	x = bp->x;
	y = bp->y;
	width = bp->width;
	height = bp->height;

	switch(bp->type) {
	case BUTTON_LABEL:
		fg = COLOR_MENU_FG;
		bg = COLOR_MENU_BG;
		outlinePixel = colors[COLOR_MENU_BG];
		topPixel = colors[COLOR_MENU_BG];
		bottomPixel = colors[COLOR_MENU_BG];
		break;
	case BUTTON_MENU_ACTIVE:
		fg = COLOR_MENU_ACTIVE_FG;
		bg = COLOR_MENU_ACTIVE_BG;
		outlinePixel = colors[COLOR_MENU_ACTIVE_DOWN];
		topPixel = colors[COLOR_MENU_ACTIVE_UP];
		bottomPixel = colors[COLOR_MENU_ACTIVE_DOWN];
		break;
	case BUTTON_TASK:
		fg = COLOR_TASK_FG;
		bg = COLOR_TASK_BG;
		outlinePixel = colors[COLOR_TASK_DOWN];
		topPixel = colors[COLOR_TASK_UP];
		bottomPixel = colors[COLOR_TASK_DOWN];
		break;
	case BUTTON_TASK_ACTIVE:
		fg = COLOR_TASK_ACTIVE_FG;
		bg = COLOR_TASK_ACTIVE_BG;
		outlinePixel = colors[COLOR_TASK_ACTIVE_DOWN];
		topPixel = colors[COLOR_TASK_ACTIVE_DOWN];
		bottomPixel = colors[COLOR_TASK_ACTIVE_UP];
		break;
	case BUTTON_MENU:
	default:
		fg = COLOR_MENU_FG;
		bg = COLOR_MENU_BG;
		outlinePixel = colors[COLOR_MENU_DOWN];
		topPixel = colors[COLOR_MENU_UP];
		bottomPixel = colors[COLOR_MENU_DOWN];
		break;
	}

	JXSetForeground(display, gc, colors[bg]);
	JXFillRectangle(display, drawable, gc, x + 2, y + 2, width - 3, height - 3);

	JXSetForeground(display, gc, outlinePixel);
	JXDrawLine(display, drawable, gc, x + 1, y, x + width - 1, y);
	JXDrawLine(display, drawable, gc, x + 1, y + height, x + width - 1,
		y + height);
	JXDrawLine(display, drawable, gc, x, y + 1, x, y + height - 1);
	JXDrawLine(display, drawable, gc, x + width, y + 1, x + width,
		y + height - 1);

	JXSetForeground(display, gc, topPixel);
	JXDrawLine(display, drawable, gc, x + 1, y + 1, x + width - 2, y + 1);
	JXDrawLine(display, drawable, gc, x + 1, y + 2, x + 1, y + height - 2);

	JXSetForeground(display, gc, bottomPixel);
	JXDrawLine(display, drawable, gc, x + 1, y + height - 1, x + width - 1,
		y + height - 1);
	JXDrawLine(display, drawable, gc, x + width - 1, y + 1, x + width - 1,
		y + height - 2);

	iconWidth = 0;
	iconHeight = 0;
	if(bp->icon) {

		if(width < height) {
			GetScaledIconSize(bp->icon, width - 2, &iconWidth, &iconHeight);
		} else {
			GetScaledIconSize(bp->icon, height - 2, &iconWidth, &iconHeight);
		}

	}

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

	switch(bp->alignment) {
	case ALIGN_RIGHT:
		xoffset = width - iconWidth - textWidth + 4;
		if(xoffset < 4) {
			xoffset = 4;
		}
		break;
	case ALIGN_CENTER:
		xoffset = width / 2 - (iconWidth + textWidth) / 2;
		if(xoffset < 0) {
			xoffset = 0;
		}
		break;
	case ALIGN_LEFT:
	default:
		xoffset = 4;
		break;
	}

	if(bp->icon) {
		yoffset = height / 2 - iconHeight / 2;
		PutIcon(bp->icon, drawable, gc, x + xoffset, y + yoffset,
			iconWidth, iconHeight);
		xoffset += iconWidth + 2;
	}

	if(bp->text) {
		yoffset = height / 2 - textHeight / 2;
		RenderString(drawable, gc, bp->font, fg, x + xoffset, y + yoffset,
			textWidth - 2, NULL, bp->text);
	}

}

/***************************************************************************
 ***************************************************************************/
void ResetButton(ButtonNode *bp, Drawable d, GC g) {

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

/***************************************************************************
 ***************************************************************************/
void GetScaledIconSize(IconNode *ip, int maxsize,
	int *width, int *height) {

	double ratio;

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

