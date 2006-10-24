/****************************************************************************
 * Functions for dealing with window borders.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"
#include "border.h"
#include "client.h"
#include "color.h"
#include "main.h"
#include "icon.h"
#include "font.h"
#include "error.h"

typedef enum {
	BP_CLOSE,
	BP_ACTIVE_CLOSE,
	BP_MINIMIZE,
	BP_ACTIVE_MINIMIZE,
	BP_MAXIMIZE,
	BP_ACTIVE_MAXIMIZE,
	BP_MAXIMIZE_ACTIVE,
	BP_ACTIVE_MAXIMIZE_ACTIVE,
	BP_COUNT
} BorderPixmapType;

typedef unsigned char BorderPixmapDataType[32];

static BorderPixmapDataType bitmaps[BP_COUNT >> 1] = {

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

static void DrawBorderHelper(const ClientNode *np,
	unsigned int width, unsigned int height, int drawIcon);
static void DrawButtonBorder(const ClientNode *np, int offset,
	Pixmap canvas, GC gc);
static int DrawBorderButtons(const ClientNode *np, Pixmap canvas, GC gc);

/****************************************************************************
 ****************************************************************************/
void InitializeBorders() {
}

/****************************************************************************
 ****************************************************************************/
void StartupBorders() {

	XGCValues gcValues;
	unsigned long gcMask;
	long fg, bg;
	int x;

	for(x = 0; x < BP_COUNT; x++) {

		if(x & 1) {
			fg = colors[COLOR_BORDER_ACTIVE_FG];
			bg = colors[COLOR_BORDER_ACTIVE_BG];
		} else {
			fg = colors[COLOR_BORDER_FG];
			bg = colors[COLOR_BORDER_BG];
		}

		pixmaps[x] = JXCreatePixmapFromBitmapData(display, rootWindow,
			(char*)bitmaps[x >> 1], 16, 16, fg, bg, rootDepth);

	}

	gcMask = GCGraphicsExposures;
	gcValues.graphics_exposures = False;
	borderGC = JXCreateGC(display, rootWindow, gcMask, &gcValues);

}

/****************************************************************************
 ****************************************************************************/
void ShutdownBorders() {

	int x;

	JXFreeGC(display, borderGC);

	for(x = 0; x < BP_COUNT; x++) {
		JXFreePixmap(display, pixmaps[x]);
	}

}

/****************************************************************************
 ****************************************************************************/
void DestroyBorders() {
}

/****************************************************************************
 ****************************************************************************/
int GetBorderIconSize() {
	return titleHeight - 4;
}

/****************************************************************************
 ****************************************************************************/
BorderActionType GetBorderActionType(const ClientNode *np, int x, int y) {

	int north;
	int offset;
	int height, width;
	int bsize;

	Assert(np);

	if(np->state.border & BORDER_OUTLINE) {
		bsize = borderWidth;
	} else {
		bsize = 0;
	}

	if(np->state.border & BORDER_TITLE) {

		if(y > bsize && y <= bsize + titleHeight) {
			if(np->icon && np->width >= titleHeight) {
				if(x > bsize && x < bsize + titleHeight) {
					return BA_MENU;
				}
			}
			offset = np->width + bsize - titleHeight;
			if((np->state.border & BORDER_CLOSE)
				&& offset > bsize + titleHeight) {
				if(x > offset && x < offset + titleHeight) {
					return BA_CLOSE;
				}
				offset -= titleHeight;
			}
			if((np->state.border & BORDER_MAX)
				&& offset > bsize + titleHeight) {
				if(x > offset && x < offset + titleHeight) {
					return BA_MAXIMIZE;
				}
				offset -= titleHeight;
			}
			if((np->state.border & BORDER_MIN) && offset > bsize + titleHeight) {
				if(x > offset && x < offset + titleHeight) {
					return BA_MINIMIZE;
				}
			}
		}

		if(y >= bsize && y <= bsize + titleHeight) {
			if(x >= bsize && x <= np->width + bsize) {
				if(np->state.border & BORDER_MOVE) {
					return BA_MOVE;
				} else {
					return BA_NONE;
				}
			}
		}

		north = bsize + titleHeight;
	} else {
		north = bsize;
	}

	if(!(np->state.border & BORDER_RESIZE)) {
		return BA_NONE;
	}

	width = np->width;

	if(np->state.status & STAT_SHADED) {
		if(x < bsize) {
			return BA_RESIZE_W | BA_RESIZE;
		} else if(x >= width + bsize) {
			return BA_RESIZE_E | BA_RESIZE;
		} else {
			return BA_NONE;
		}
	}

	height = np->height;

	if(width >= titleHeight * 2 && height >= titleHeight * 2) {
		if(x < bsize + titleHeight && y < titleHeight + bsize) {
			return  BA_RESIZE_N | BA_RESIZE_W | BA_RESIZE;
		} else if(x < titleHeight + bsize
			&& y - north >= height - titleHeight) {
			return BA_RESIZE_S | BA_RESIZE_W | BA_RESIZE;
		} else if(x - bsize >= width - titleHeight
			&& y < titleHeight + bsize) {
			return BA_RESIZE_N | BA_RESIZE_E | BA_RESIZE;
		} else if(x - bsize >= width - titleHeight
			&& y - north >= height - titleHeight) {
			return BA_RESIZE_S | BA_RESIZE_E | BA_RESIZE;
		}
	}
	if(x < bsize) {
		return BA_RESIZE_W | BA_RESIZE;
	} else if(x >= width + bsize) {
		return BA_RESIZE_E | BA_RESIZE;
	} else if(y < bsize) {
		return BA_RESIZE_N | BA_RESIZE;
	} else if(y >= height) {
		return BA_RESIZE_S | BA_RESIZE;
	} else {
		return BA_NONE;
	}
}

/****************************************************************************
 ****************************************************************************/
void DrawBorder(const ClientNode *np, const XExposeEvent *expose) {

	XRectangle rect;
	unsigned int width;
	unsigned int height;
	int bsize;
	int drawIcon;
	int temp;

	Assert(np);

	if(shouldExit) {
		return;
	}

	if(!(np->state.status & (STAT_MAPPED | STAT_SHADED))) {
		return;
	}
	if(np->state.status & (STAT_HIDDEN | STAT_FULLSCREEN)) {
		return;
	}

	if(np->state.border & BORDER_OUTLINE) {
		bsize = borderWidth;
	} else {
		bsize = 0;
	}

	if(bsize == 0 && !(np->state.border & BORDER_TITLE)) {
		return;
	}

	if(expose) {

		if(!borderRegion) {
			borderRegion = XCreateRegion();
		}

		rect.x = (short)expose->x;
		rect.y = (short)expose->y;
		rect.width = (unsigned short)expose->width;
		rect.height = (unsigned short)expose->height;
		XUnionRectWithRegion(&rect, borderRegion, borderRegion);

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
		temp = GetBorderIconSize();
		rect.x = (short)bsize + 2;
		rect.y = (short)(bsize + titleHeight / 2 - temp / 2);
		rect.width = (unsigned short)temp;
		rect.height = (unsigned short)temp;
		if(XRectInRegion(borderRegion, rect.x, rect.y, rect.width, rect.height)
			!= RectangleOut) {

			drawIcon = 1;
			XUnionRectWithRegion(&rect, borderRegion, borderRegion);

		} else {

			drawIcon = 0;

		}

		XSetRegion(display, borderGC, borderRegion);

	} else {

		drawIcon = 1;
		XSetClipMask(display, borderGC, None);

	}

	if(np->state.status & STAT_SHADED) {
		height = titleHeight + bsize * 2;
	} else if(np->state.border & BORDER_TITLE) {
		height = np->height + titleHeight + bsize * 2;
	} else {
		height = np->height + 2 * bsize;
	}
	width = np->width + bsize * 2;

	DrawBorderHelper(np, width, height, drawIcon);

	if(expose) {
		XDestroyRegion(borderRegion);
		borderRegion = NULL;
	}

}

/****************************************************************************
 ****************************************************************************/
void DrawBorderHelper(const ClientNode *np,
	unsigned int width, unsigned int height, int drawIcon) {

	ColorType borderTextColor;
	long borderPixel, borderTextPixel;
	long pixelUp, pixelDown;
	int buttonCount, titleWidth;
	Pixmap canvas;
	GC gc;
	int iconSize;
	int bsize;

	Assert(np);

	iconSize = GetBorderIconSize();

	if(np->state.status & STAT_ACTIVE) {
		borderTextColor = COLOR_BORDER_ACTIVE_FG;
		borderPixel = colors[COLOR_BORDER_ACTIVE_BG];
		borderTextPixel = colors[COLOR_BORDER_ACTIVE_FG];
		pixelUp = colors[COLOR_BORDER_ACTIVE_UP];
		pixelDown = colors[COLOR_BORDER_ACTIVE_DOWN];
	} else {
		borderTextColor = COLOR_BORDER_FG;
		borderPixel = colors[COLOR_BORDER_BG];
		borderTextPixel = colors[COLOR_BORDER_FG];
		pixelUp = colors[COLOR_BORDER_UP];
		pixelDown = colors[COLOR_BORDER_DOWN];
	}

	if(np->state.border & BORDER_OUTLINE) {
		bsize = borderWidth;
	} else {
		bsize = 0;
	}

	canvas = np->parent;
	gc = borderGC;

	/* Set the window background to reduce perceived flicker on the
	 * parts of the window that will need to be redrawn. */
	JXSetWindowBackground(display, canvas, borderPixel);

	JXSetForeground(display, gc, borderPixel);
	JXFillRectangle(display, canvas, gc, 0, 0, width + 1, height + 1);

	buttonCount = DrawBorderButtons(np, canvas, gc);
	titleWidth = width - (titleHeight + 2) * buttonCount - bsize
		- (titleHeight + bsize + 4) - 2;

	if(np->state.border & BORDER_TITLE) {

		if(np->icon && np->width >= titleHeight && drawIcon) {
			PutIcon(np->icon, canvas, bsize + 2,
				bsize + titleHeight / 2 - iconSize / 2,
				iconSize, iconSize);
		}

		if(np->name && np->name[0] && titleWidth > 0) {
			RenderString(canvas, FONT_BORDER, borderTextColor,
				titleHeight + bsize + 4, bsize + titleHeight / 2
				- GetStringHeight(FONT_BORDER) / 2,
				titleWidth, borderRegion, np->name);
		}

	}

	if(np->state.border & BORDER_OUTLINE) {

		/* Draw title outline */
		JXSetForeground(display, gc, pixelUp);
		JXDrawLine(display, canvas, gc, borderWidth, borderWidth,
			width - borderWidth - 1, borderWidth);
		JXDrawLine(display, canvas, gc, borderWidth, borderWidth + 1,
			borderWidth, titleHeight + borderWidth - 1);

		JXSetForeground(display, gc, pixelDown);
		JXDrawLine(display, canvas, gc, borderWidth + 1,
			titleHeight + borderWidth - 1, width - borderWidth,
			titleHeight + borderWidth - 1);
		JXDrawLine(display, canvas, gc, width - borderWidth - 1,
			borderWidth + 1, width - borderWidth - 1, titleHeight + borderWidth);

		/* Draw outline */
		JXSetForeground(display, gc, pixelUp);
		JXDrawLine(display, canvas, gc, width - borderWidth,
			borderWidth, width - borderWidth, height - borderWidth);
		JXDrawLine(display, canvas, gc, borderWidth,
			height - borderWidth, width - borderWidth, height - borderWidth);

		JXSetForeground(display, gc, pixelDown);
		JXDrawLine(display, canvas, gc, borderWidth - 1,
			borderWidth - 1, width - borderWidth, borderWidth - 1);
		JXDrawLine(display, canvas, gc, borderWidth - 1, borderWidth,
			borderWidth - 1, height - borderWidth);

		JXFillRectangle(display, canvas, gc, width - 2, 0, 2, height);
		JXFillRectangle(display, canvas, gc, 0, height - 2, width, 2);
		JXSetForeground(display, gc, pixelUp);
		JXDrawLine(display, canvas, gc, 0, 0, 0, height - 1);
		JXDrawLine(display, canvas, gc, 1, 1, 1, height - 2);
		JXDrawLine(display, canvas, gc, 1, 0, width - 1, 0);
		JXDrawLine(display, canvas, gc, 1, 1, width - 2, 1);

		if((np->state.border & BORDER_RESIZE)
			&& !(np->state.status & STAT_SHADED)
			&& np->width >= 2 * titleHeight * 2
			&& np->height >= titleHeight * 2) {

			/* Draw marks */
			JXSetForeground(display, gc, pixelDown);

			/* Upper left */
			JXDrawLine(display, canvas, gc,
				titleHeight + borderWidth - 1, 2, titleHeight + borderWidth - 1,
				borderWidth - 2);
			JXDrawLine(display, canvas, gc, 2,
				titleHeight + borderWidth - 1, borderWidth - 2,
				titleHeight + borderWidth - 1);

			/* Upper right */
			JXDrawLine(display, canvas, gc,
				width - titleHeight - borderWidth - 1,
				2, width - titleHeight - borderWidth - 1, borderWidth - 2);
			JXDrawLine(display, canvas, gc, width - 3,
				titleHeight + borderWidth - 1, width - borderWidth + 1,
				titleHeight + borderWidth - 1);

			/* Lower left */
			JXDrawLine(display, canvas, gc, 2,
				height - titleHeight - borderWidth - 1, borderWidth - 2,
				height - titleHeight - borderWidth - 1);
			JXDrawLine(display, canvas, gc,
				titleHeight + borderWidth - 1, height - 3,
				titleHeight + borderWidth - 1, height - borderWidth + 1);

			/* Lower right */
			JXDrawLine(display, canvas, gc, width - 3,
				height - titleHeight - borderWidth - 1, width - borderWidth + 1,
				height - titleHeight - borderWidth - 1);
			JXDrawLine(display, canvas, gc,
				width - titleHeight - borderWidth - 1,
				height - 3, width - titleHeight - borderWidth - 1,
				height - borderWidth + 1);

			JXSetForeground(display, gc, pixelUp);

			/* Upper left */
			JXDrawLine(display, canvas, gc, titleHeight + borderWidth,
				2, titleHeight + borderWidth, borderWidth - 2);
			JXDrawLine(display, canvas, gc, 2,
				titleHeight + borderWidth, borderWidth - 2,
				titleHeight + borderWidth);

			/* Upper right */
			JXDrawLine(display, canvas, gc,
				width - titleHeight - borderWidth, 2,
				width - titleHeight - borderWidth, borderWidth - 2);
			JXDrawLine(display, canvas, gc, width - 3,
				titleHeight + borderWidth, width - borderWidth + 1,
				titleHeight + borderWidth);

			/* Lower left */
			JXDrawLine(display, canvas, gc, 2,
				height - titleHeight - borderWidth,
				borderWidth - 2, height - titleHeight - borderWidth);
			JXDrawLine(display, canvas, gc, titleHeight + borderWidth,
				height - 3, titleHeight + borderWidth, height - borderWidth + 1);

			/* Lower right */
			JXDrawLine(display, canvas, gc, width - 3,
				height - titleHeight - borderWidth, width - borderWidth + 1,
				height - titleHeight - borderWidth);
			JXDrawLine(display, canvas, gc,
				width - titleHeight - borderWidth, height - 3,
				width - titleHeight - borderWidth, height - borderWidth + 1);

		}

	}

}

/****************************************************************************
 ****************************************************************************/
void DrawButtonBorder(const ClientNode *np, int offset,
	Pixmap canvas, GC gc) {

	long up, down;
	long bsize;

	Assert(np);

	if(np->state.status & STAT_ACTIVE) {
		up = colors[COLOR_BORDER_ACTIVE_UP];
		down = colors[COLOR_BORDER_ACTIVE_DOWN];
	} else {
		up = colors[COLOR_BORDER_UP];
		down = colors[COLOR_BORDER_DOWN];
	}

	if(np->state.border & BORDER_OUTLINE) {
		bsize = borderWidth;
	} else {
		bsize = 0;
	}

	JXSetForeground(display, gc, up);
	JXDrawLine(display, canvas, gc, offset, bsize + 1,
		offset, titleHeight + bsize  - 2);

	JXSetForeground(display, gc, down);
	JXDrawLine(display, canvas, gc, offset - 1,
		bsize + 1, offset - 1, titleHeight + bsize - 2);

}

/****************************************************************************
 ****************************************************************************/
int DrawBorderButtons(const ClientNode *np, Pixmap canvas, GC gc) {

	Pixmap pixmap;
	int count = 0;
	int offset;
	int bsize;

	Assert(np);

	if(!(np->state.border & BORDER_TITLE)) {
		return count;
	}
	if(np->state.border & BORDER_OUTLINE) {
		bsize = borderWidth;
	} else {
		bsize = 0;
	}

	offset = np->width + bsize - titleHeight;

	if(offset <= bsize + titleHeight) {
		return count;
	}

	if(np->state.border & BORDER_CLOSE) {

		DrawButtonBorder(np, offset, canvas, gc);

		if(np->state.status & STAT_ACTIVE) {
			pixmap = pixmaps[BP_ACTIVE_CLOSE];
		} else {
			pixmap = pixmaps[BP_CLOSE];
		}

		JXCopyArea(display, pixmap, canvas, gc, 0, 0, 16, 16,
			offset + titleHeight / 2 - 8, bsize + titleHeight / 2 - 8);

		offset -= titleHeight;
		++count;

		if(offset <= bsize + titleHeight) {
			return count;
		}

	}

	if(np->state.border & BORDER_MAX) {

		if(np->state.status & STAT_MAXIMIZED) {
			if(np->state.status & STAT_ACTIVE) {
				pixmap = pixmaps[BP_ACTIVE_MAXIMIZE_ACTIVE];
			} else {
				pixmap = pixmaps[BP_MAXIMIZE_ACTIVE];
			}
		} else {
			if(np->state.status & STAT_ACTIVE) {
				pixmap = pixmaps[BP_ACTIVE_MAXIMIZE];
			} else {
				pixmap = pixmaps[BP_MAXIMIZE];
			}
		}
		JXCopyArea(display, pixmap, canvas, gc, 0, 0, 16, 16,
			offset + titleHeight / 2 - 8, bsize + titleHeight / 2 - 8);

		DrawButtonBorder(np, offset, canvas, gc);

		offset -= titleHeight;
		++count;

		if(offset <= bsize + titleHeight) {
			return count;
		}

	}

	if(np->state.border & BORDER_MIN) {

		DrawButtonBorder(np, offset, canvas, gc);

		if(np->state.status & STAT_ACTIVE) {
			pixmap = pixmaps[BP_ACTIVE_MINIMIZE];
		} else {
			pixmap = pixmaps[BP_MINIMIZE];
		}

		JXCopyArea(display, pixmap, canvas, gc, 0, 0, 16, 16,
			offset + titleHeight / 2 - 8, bsize + titleHeight / 2 - 8);

		++count;

	}

	return count;

}

/****************************************************************************
 * Redraw the borders on the current desktop.
 * This should be done after loading clients since the stacking order
 * may cause borders on the current desktop to become visible after moving
 * clients to their assigned desktops.
 ****************************************************************************/
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

/****************************************************************************
 ****************************************************************************/
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
		*north += titleHeight;
	}

}

/****************************************************************************
 ****************************************************************************/
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

/****************************************************************************
 ****************************************************************************/
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


