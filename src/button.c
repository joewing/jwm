/***************************************************************************
 * Functions to handle drawing buttons.
 * Copyright (C) 2004 Joe Wingbermuehle
 ***************************************************************************/

#include "jwm.h"

static Drawable drawable;
static GC gc;
static FontType font;
static int width, height;
static AlignmentType alignment;
static int yoffset;
static int textOffset;

/***************************************************************************
 ***************************************************************************/
void SetButtonDrawable(Drawable d, GC g) {
	drawable = d;
	gc = g;
	textOffset = 0;
}

/***************************************************************************
 ***************************************************************************/
void SetButtonFont(FontType f) {
	font = f;
	yoffset = 1 + height / 2 - (fonts[f]->ascent + fonts[f]->descent) / 2;
}

/***************************************************************************
 ***************************************************************************/
void SetButtonSize(int w, int h) {
	width = w;
	height = h;
	if(fonts[font]) {
		yoffset = 1 + height / 2
			- (fonts[font]->ascent + fonts[font]->descent) / 2;
	}
}

/***************************************************************************
 ***************************************************************************/
void SetButtonAlignment(AlignmentType a) {
	alignment = a;
}

/***************************************************************************
 ***************************************************************************/
void SetButtonTextOffset(int o) {
	textOffset = o;
}

/***************************************************************************
 ***************************************************************************/
void DrawButton(int x, int y, ButtonType type, const char *str) {
	long outlinePixel;
	long topPixel, bottomPixel;
	RampType ramp;
	int xoffset;
	int len;

	switch(type) {
	case BUTTON_MENU_ACTIVE:
		ramp = RAMP_MENU_ACTIVE;
		outlinePixel = colors[COLOR_MENU_ACTIVE_DOWN];
		topPixel = colors[COLOR_MENU_ACTIVE_UP];
		bottomPixel = colors[COLOR_MENU_ACTIVE_DOWN];
		break;
	case BUTTON_TRAY:
		ramp = RAMP_TRAY;
		outlinePixel = colors[COLOR_BORDER_DOWN];
		topPixel = colors[COLOR_BORDER_UP];
		bottomPixel = colors[COLOR_BORDER_DOWN];
		break;
	case BUTTON_TRAY_ACTIVE:
		ramp = RAMP_TRAY_ACTIVE;
		outlinePixel = colors[COLOR_TRAY_ACTIVE_DOWN];
		topPixel = colors[COLOR_TRAY_ACTIVE_DOWN];
		bottomPixel = colors[COLOR_TRAY_ACTIVE_UP];
		break;
	case BUTTON_MENU:
	default:
		ramp = RAMP_MENU;
		outlinePixel = colors[COLOR_MENU_DOWN];
		topPixel = colors[COLOR_MENU_UP];
		bottomPixel = colors[COLOR_MENU_DOWN];
		break;
	}

	JXSetForeground(display, gc, ramps[ramp][0]);
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

	if(str) {
		len = strlen(str);
	} else {
		len = 0;
	}

	if(len) {
		switch(alignment) {
		case ALIGN_CENTER:
			xoffset = 1 + width / 2 - JXTextWidth(fonts[font], str, len) / 2;
			break;
		default:
			xoffset = 4 + textOffset;
			width -= textOffset;
			break;
		}
		RenderString(drawable, gc, font, ramp, x + xoffset, y + yoffset,
			width - 8, str);
	}

}

