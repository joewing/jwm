/***************************************************************************
 * Functions to handle drawing buttons.
 * Copyright (C) 2004 Joe Wingbermuehle
 ***************************************************************************/

#include "jwm.h"
#include "button.h"
#include "font.h"
#include "color.h"
#include "main.h"
#include "image.h"
#include "theme.h"
#include "icon.h"

#define BUTTON_PADDING 4

static int northOffset = 0;
static int southOffset = 0;
static int eastOffset = 0;
static int westOffset = 0;

/***************************************************************************
 ***************************************************************************/
void DrawButton(const ButtonData *button) {

	XRectangle rect;
	int usableWidth, usableHeight;
	int iconSize;
	int textSize;
	int iconOffset, textOffset;
	int yoffset;
	int x, y;

	usableWidth = button->width - eastOffset - westOffset;
	usableHeight = button->height - northOffset - southOffset;

	if(usableWidth < usableHeight) {
		iconSize = usableWidth;
	} else {
		iconSize = usableHeight;
	}

	if(button->type != BUTTON_NONE) {

		/* Draw the button outline. */
		rect.x = button->x;
		rect.y = button->y;
		rect.width = button->width;
		rect.height = button->height;
		XSetClipRectangles(display, button->gc, 0, 0, &rect, 1, Unsorted);

		DrawThemeOutline(PART_BUTTON, button->background,
			button->drawable, button->gc,
			button->x, button->y,
			button->width, button->height, (int)button->type);

		JXSetClipMask(display, button->gc, None);

	} else if(button->fillPart) {

		for(y = button->y; y < button->y + button->height;) {
			for(x = button->x; x < button->x + button->width;) {
				JXCopyArea(display, button->fillPart->image->image,
					button->drawable, button->gc, 0, 0,
					button->fillPart->width, button->fillPart->height, x, y);
				x += button->fillPart->width;
			}
			y += button->fillPart->height;
		}

	} else {

		JXSetForeground(display, button->gc, colors[button->background]);
		JXFillRectangle(display, button->drawable, button->gc,
			button->x, button->y, button->width, button->height);

	}

	if(iconSize <= 0) {
		return;
	}

	if(button->text) {
		textSize = GetStringWidth(button->font, button->text);
		if(button->icon) {
			if(textSize > usableWidth - iconSize - BUTTON_PADDING) {
				textSize = usableWidth - iconSize - BUTTON_PADDING;
			}
		} else {
			if(textSize > usableWidth) {
				textSize = usableWidth;
			}
		}
	} else {
		textSize = 0;
	}

	if(button->alignment == ALIGN_LEFT) {
		if(button->icon) {
			iconOffset = button->x + westOffset;
			textOffset = iconOffset + iconSize + BUTTON_PADDING;
		} else {
			iconOffset = 0;
			textOffset = button->x + westOffset;
		}
	} else { /* ALIGN_CENTER */
		if(button->icon) {
			iconOffset = usableWidth / 2;
			iconOffset -= (iconSize + textSize + BUTTON_PADDING) / 2;
			iconOffset += button->x + westOffset;
			textOffset = iconOffset + BUTTON_PADDING;
		} else {
			textOffset = usableWidth / 2 - textSize / 2;
			textOffset += button->x + westOffset;
		}
	}

	if(button->icon) {

		yoffset = button->y + northOffset;
		yoffset += usableHeight / 2 - iconSize / 2;

		PutIcon(button->icon, button->drawable, button->gc,
			iconOffset, yoffset, iconSize, iconSize);

	}

	if(button->text && textSize > 0) {

		yoffset = button->y + northOffset;
		yoffset += usableHeight / 2 - GetStringHeight(button->font) / 2;

		RenderString(button->drawable, button->gc, button->font,
			button->color, textOffset, yoffset, textSize, button->text);

	}

}

/***************************************************************************
 ***************************************************************************/
void ResetButton(ButtonData *button, Drawable d, GC g) {

	button->type = BUTTON_NORMAL;
	button->alignment = ALIGN_LEFT;
	button->font = FONT_BORDER;
	button->color = COLOR_BORDER_FG;
	button->background = COLOR_BORDER_BG;
	button->fillPart = NULL;
	button->x = 0;
	button->y = 0;
	button->width = 0;
	button->height = 0;
	button->drawable = d;
	button->gc = g;
	button->icon = NULL;
	button->text = NULL;

}

/***************************************************************************
 ***************************************************************************/
void SetButtonOffsets(int north, int south, int east, int west) {

	northOffset = north;
	southOffset = south;
	eastOffset = east;
	westOffset = west;

}

/***************************************************************************
 ***************************************************************************/
void GetButtonOffsets(int *north, int *south, int *east, int *west) {

	*north = northOffset;
	*south = southOffset;
	*east = eastOffset;
	*west = westOffset;

}

