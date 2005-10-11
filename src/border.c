/****************************************************************************
 * Functions for dealing with window borders.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"

static unsigned char x_bitmap[] = {
	0x00, 0x00, 0x00, 0x00, 0x0C, 0x30, 0x18, 0x18, 0x30, 0x0C, 0x60, 0x06,
	0xC0, 0x03, 0x80, 0x01, 0xC0, 0x03, 0x60, 0x06, 0x30, 0x0C, 0x18, 0x18,
	0x0C, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static unsigned char min_bitmap[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x3F,
	0xFC, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static unsigned char max_bitmap[] = {
	0x00, 0x00, 0x00, 0x00, 0xFC, 0x3F, 0xFC, 0x3F, 0x04, 0x20, 0x04, 0x20,
	0x04, 0x20, 0x04, 0x20, 0x04, 0x20, 0x04, 0x20, 0x04, 0x20, 0x04, 0x20,
	0xFC, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static unsigned char maxa_bitmap[] = {
	0x00, 0x00, 0x00, 0x00, 0xFC, 0x3F, 0xFC, 0x3F, 0x04, 0x20, 0xE4, 0x27,
	0xE4, 0x27, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0xE4, 0x27, 0x04, 0x20,
	0xFC, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static Pixmap closePixmap;
static Pixmap closeActivePixmap;
static Pixmap minPixmap;
static Pixmap minActivePixmap;
static Pixmap maxPixmap;
static Pixmap maxActivePixmap;
static Pixmap maxaPixmap;
static Pixmap maxaActivePixmap;

static Pixmap buffer = None;
static GC bufferGC = None;
static int bufferWidth;
static int bufferHeight;

static void DrawBorderHelper(const ClientNode *np, unsigned int height);
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
	long fg, bg;         /* Inactive foreground/background colors */
	long afg, abg;       /* Active foreground/background colors */

	afg = colors[COLOR_BORDER_ACTIVE_FG];
	abg = colors[COLOR_BORDER_ACTIVE_BG];
	fg = colors[COLOR_BORDER_FG];
	bg = colors[COLOR_BORDER_BG];

	closePixmap = JXCreatePixmapFromBitmapData(display, rootWindow,
		(char*)x_bitmap, 16, 16, fg, bg, rootDepth);
	closeActivePixmap = XCreatePixmapFromBitmapData(display, rootWindow,
		(char*)x_bitmap, 16, 16, afg, abg, rootDepth);

	minPixmap = JXCreatePixmapFromBitmapData(display, rootWindow,
		(char*)min_bitmap, 16, 16, fg, bg, rootDepth);
	minActivePixmap = JXCreatePixmapFromBitmapData(display, rootWindow,
		(char*)min_bitmap, 16, 16, afg, abg, rootDepth);

	maxPixmap = JXCreatePixmapFromBitmapData(display, rootWindow,
		(char*)max_bitmap, 16, 16, fg, bg, rootDepth);
	maxActivePixmap = JXCreatePixmapFromBitmapData(display, rootWindow,
		(char*)max_bitmap, 16, 16, afg, abg, rootDepth);

	maxaPixmap = JXCreatePixmapFromBitmapData(display, rootWindow,
		(char*)maxa_bitmap, 16, 16, fg, bg, rootDepth);
	maxaActivePixmap = JXCreatePixmapFromBitmapData(display, rootWindow,
		(char*)maxa_bitmap, 16, 16, afg, abg, rootDepth);

}

/****************************************************************************
 ****************************************************************************/
void ShutdownBorders() {

	JXFreePixmap(display, closePixmap);
	JXFreePixmap(display, closeActivePixmap);

	JXFreePixmap(display, minPixmap);
	JXFreePixmap(display, minActivePixmap);

	JXFreePixmap(display, maxPixmap);
	JXFreePixmap(display, maxActivePixmap);

	JXFreePixmap(display, maxaPixmap);
	JXFreePixmap(display, maxaActivePixmap);

	if(buffer != None) {
		JXFreeGC(display, bufferGC);
		JXFreePixmap(display, buffer);
		buffer = None;
		bufferGC = None;
		bufferWidth = 0;
		bufferHeight = 0;
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

	if(!(np->borderFlags & BORDER_OUTLINE)) {
		return BA_NONE;
	}

	if(np->borderFlags & BORDER_TITLE) {

		if(y > borderWidth && y <= borderWidth + titleHeight) {
			offset = np->width + borderWidth - titleHeight;
			if(np->borderFlags & BORDER_CLOSE) {
				if(x > offset && x < offset + titleHeight) {
					return BA_CLOSE;
				}
				offset -= titleHeight;
			}
			if(np->borderFlags & BORDER_MAX) {
				if(x > offset && x < offset + titleHeight) {
					return BA_MAXIMIZE;
				}
				offset -= titleHeight;
			}
			if(np->borderFlags & BORDER_MIN) {
				if(x > offset && x < offset + titleHeight) {
					return BA_MINIMIZE;
				}
			}
		}

		if(y >= borderWidth && y <= titleSize) {
			if(x >= borderWidth && x <= np->width + borderWidth) {
				if(np->borderFlags & BORDER_MOVE) {
					return BA_MOVE;
				} else {
					return BA_NONE;
				}
			}
		}

		north = titleSize;
	} else {
		north = borderWidth;
	}

	if(!(np->borderFlags & BORDER_RESIZE)) {
		return BA_NONE;
	}

	width = np->width;

	if(np->statusFlags & STAT_SHADED) {
		if(x < borderWidth) {
			return BA_RESIZE_W | BA_RESIZE;
		} else if(x >= width + borderWidth) {
			return BA_RESIZE_E | BA_RESIZE;
		} else {
			return BA_NONE;
		}
	}

	height = np->height;

	if(x < borderWidth + titleHeight && y < titleHeight + borderWidth) {
		return  BA_RESIZE_N | BA_RESIZE_W | BA_RESIZE;
	} else if(x < titleHeight + borderWidth
		&& y - north >= height - titleHeight) {
		return BA_RESIZE_S | BA_RESIZE_W | BA_RESIZE;
	} else if(x - borderWidth >= width - titleHeight
		&& y < titleHeight + borderWidth) {
		return BA_RESIZE_N | BA_RESIZE_E | BA_RESIZE;
	} else if(x - borderWidth >= width - titleHeight
		&& y - north >= height - titleHeight) {
		return BA_RESIZE_S | BA_RESIZE_E | BA_RESIZE;
	} else if(x < borderWidth) {
		return BA_RESIZE_W | BA_RESIZE;
	} else if(x >= width + borderWidth) {
		return BA_RESIZE_E | BA_RESIZE;
	} else if(y < borderWidth) {
		return BA_RESIZE_N | BA_RESIZE;
	} else if(y >= height) {
		return BA_RESIZE_S | BA_RESIZE;
	} else {
		return BA_NONE;
	}
}

/****************************************************************************
 ****************************************************************************/
void DrawBorder(const ClientNode *np) {

	unsigned int height;

	if(shouldExit) {
		return;
	}

	if(np->statusFlags & (STAT_MINIMIZED | STAT_WITHDRAWN | STAT_HIDDEN)) {
		return;
	}

	if(!(np->borderFlags & BORDER_TITLE)
		&& !(np->borderFlags & BORDER_OUTLINE)) {
		return;
	}

	if(np->statusFlags & STAT_SHADED) {
		height = titleSize + borderWidth;
	} else if(np->borderFlags & BORDER_TITLE) {
		height = np->height + titleSize + borderWidth;
	} else {
		height = np->height + 2 * borderWidth;
	}

	DrawBorderHelper(np, height);

}

/****************************************************************************
 ****************************************************************************/
void DrawBorderHelper(const ClientNode *np, unsigned int height) {

	unsigned int width;
	ColorType borderTextColor;
	long borderPixel, borderTextPixel;
	long pixelUp, pixelDown;
	int buttonCount, titleWidth;
	Pixmap canvas;
	GC gc;
	int iconSize;

	width = np->width + borderWidth + borderWidth;
	iconSize = GetBorderIconSize();

	if(np->statusFlags & STAT_ACTIVE) {
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

	if(buffer == None || bufferWidth < width || bufferHeight < height) {

		if(buffer != None) {
			XFreeGC(display, bufferGC);
			XFreePixmap(display, buffer);
		}

		bufferWidth = width;
		bufferHeight = height;
		buffer = JXCreatePixmap(display, np->parent, bufferWidth, bufferHeight,
			rootDepth);
		bufferGC = JXCreateGC(display, buffer, 0, NULL);

	}

	canvas = buffer;
	gc = bufferGC;

	JXSetWindowBackground(display, np->parent, borderPixel);
	JXSetForeground(display, bufferGC, borderPixel);
	JXFillRectangle(display, buffer, bufferGC, 0, 0, width, height);

	buttonCount = DrawBorderButtons(np, canvas, gc);
	titleWidth = width - (titleHeight + 2) * buttonCount - borderWidth
		- (titleSize + 4) - 2;

	if(np->borderFlags & BORDER_TITLE) {

		if(np->icon) {
			PutIcon(np->icon, canvas, gc,
				borderWidth + 2,
				borderWidth + titleHeight / 2 - iconSize / 2,
				iconSize);
		}

		if(np->name && np->name[0] && titleWidth > 0) {
			RenderString(canvas, gc, FONT_BORDER, borderTextColor,
				titleSize + 4, borderWidth + titleHeight / 2
				- GetStringHeight(FONT_BORDER) / 2,
				titleWidth, np->name);
		}

	}

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

	if((np->borderFlags & BORDER_RESIZE) && !(np->statusFlags & STAT_SHADED)) {

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

	JXCopyArea(display, canvas, np->parent, np->parentGC, 0, 0,
		width, height, 0, 0);

}

/****************************************************************************
 ****************************************************************************/
void DrawButtonBorder(const ClientNode *np, int offset,
	Pixmap canvas, GC gc) {

	long up, down;

	Assert(np);

	if(np->statusFlags & STAT_ACTIVE) {
		up = colors[COLOR_BORDER_ACTIVE_UP];
		down = colors[COLOR_BORDER_ACTIVE_DOWN];
	} else {
		up = colors[COLOR_BORDER_UP];
		down = colors[COLOR_BORDER_DOWN];
	}

	JXSetForeground(display, gc, up);
	JXDrawLine(display, canvas, gc, offset, borderWidth + 1,
		offset, titleHeight + borderWidth  - 2);

	JXSetForeground(display, gc, down);
	JXDrawLine(display, canvas, gc, offset - 1,
		borderWidth + 1, offset - 1, titleHeight + borderWidth - 2);

}

/****************************************************************************
 ****************************************************************************/
int DrawBorderButtons(const ClientNode *np, Pixmap canvas, GC gc) {
	Pixmap pixmap;
	int count = 0;
	int offset;

	Assert(np);

	if(!(np->borderFlags & BORDER_TITLE)) {
		return count;
	}

	offset = np->width + borderWidth - titleHeight;

	if(offset <= titleSize) {
		return count;
	}

	if(np->borderFlags & BORDER_CLOSE) {

		DrawButtonBorder(np, offset, canvas, gc);

		if(np->statusFlags & STAT_ACTIVE) {
			pixmap = closeActivePixmap;
		} else {
			pixmap = closePixmap;
		}

		JXCopyArea(display, pixmap, canvas, gc, 0, 0, 16, 16,
			offset + titleHeight / 2 - 8, borderWidth + titleHeight / 2 - 8);

		offset -= titleHeight;
		++count;

		if(offset <= titleSize) {
			return count;
		}

	}

	if(np->borderFlags & BORDER_MAX) {

		if(np->statusFlags & STAT_MAXIMIZED) {
			if(np->statusFlags & STAT_ACTIVE) {
				pixmap = maxaActivePixmap;
			} else {
				pixmap = maxaPixmap;
			}
		} else {
			if(np->statusFlags & STAT_ACTIVE) {
				pixmap = maxActivePixmap;
			} else {
				pixmap = maxPixmap;
			}
		}
		JXCopyArea(display, pixmap, canvas, gc, 0, 0, 16, 16,
			offset + titleHeight / 2 - 8, borderWidth + titleHeight / 2 - 8);

		DrawButtonBorder(np, offset, canvas, gc);

		offset -= titleHeight;
		++count;

		if(offset <= titleSize) {
			return count;
		}

	}

	if(np->borderFlags & BORDER_MIN) {

		DrawButtonBorder(np, offset, canvas, gc);

		if(np->statusFlags & STAT_ACTIVE) {
			pixmap = minActivePixmap;
		} else {
			pixmap = minPixmap;
		}

		JXCopyArea(display, pixmap, canvas, gc, 0, 0, 16, 16,
			offset + titleHeight / 2 - 8, borderWidth + titleHeight / 2 - 8);

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
			if(!(np->statusFlags & (STAT_HIDDEN | STAT_MINIMIZED))) {
				DrawBorder(np);
			}
		}
	}

}

/****************************************************************************
 ****************************************************************************/
void SetBorderWidth(const char *str) {
	int width;

	Assert(str);

	width = atoi(str);
	if(width < MIN_BORDER_WIDTH || width > MAX_BORDER_WIDTH) {
		borderWidth = DEFAULT_BORDER_WIDTH;
		Warning("invalid border width specified: %d", width);
	} else {
		borderWidth = width;
	}
	titleSize = titleHeight + borderWidth;

}

/****************************************************************************
 ****************************************************************************/
void SetTitleHeight(const char *str) {
	int height;

	Assert(str);

	height = atoi(str);
	if(height < MIN_TITLE_HEIGHT || height > MAX_TITLE_HEIGHT) {
		titleHeight = DEFAULT_TITLE_HEIGHT;
		Warning("invalid title height specified: %d", height);
	} else {
		titleHeight = height;
	}
	titleSize = titleHeight + borderWidth;

}


