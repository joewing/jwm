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
#include "image.h"
#include "font.h"
#include "error.h"
#include "misc.h"
#include "theme.h"

typedef enum {

	ACT_MENU,

	ACT_MIN,
	ACT_MAX,
	ACT_CLOSE,

	ACT_COUNT

} ButtonActionType;

typedef struct {

	const char *action;
	int x, y;

} ButtonActionNode;

static ButtonActionNode buttons[ACT_COUNT] = {
	{ "menu",  0, 0 },
	{ "min",   0, 0 },
	{ "max",   0, 0 },
	{ "close", 0, 0 }
};

static int topLeftWidth, topRightWidth;
static int topLeftHeight, topRightHeight;
static int bottomLeftWidth, bottomRightWidth;
static int bottomLeftHeight, bottomRightHeight;
static int borderUsesShape;

static void DrawButtons(const ClientNode *np);
static int GetButtonWidth(const ClientNode *np);

static int DrawPart(const ClientNode *np, PartType part, int x, int y);
static int DrawShape(const ClientNode *np, Drawable d, GC g,
	PartType part, int x, int y);

#ifdef USE_SHAPE
static void ApplySimpleBorderShape(const ClientNode *np);
static void ApplyComplexBorderShape(const ClientNode *np);
#endif

/****************************************************************************
 ****************************************************************************/
void InitializeBorders() {

	int x;

	borderUsesShape = 0;
	
	for(x = 0; x < ACT_COUNT; x++) {
		buttons[x].x = 0;
		buttons[x].y = 0;
	}

}

/****************************************************************************
 ****************************************************************************/
void StartupBorders() {

	int x;

	for(x = PART_BORDER_START; x <= PART_BORDER_STOP; x++) {
		if(parts[x].image->shape != None) {
			borderUsesShape = 1;
			break;
		}
	}

	if(parts[PART_TL].image) {
		topLeftWidth = parts[PART_TL].width;
		topLeftHeight = parts[PART_TL].height;
	} else {
		topLeftWidth = parts[PART_L].width;
		topLeftHeight = parts[PART_T].height;
	}
	if(parts[PART_TR].image) {
		topRightWidth = parts[PART_TR].width;
		topRightHeight = parts[PART_TR].height;
	} else {
		topRightWidth = parts[PART_R].width;
		topRightHeight = parts[PART_T].height;
	}
	if(parts[PART_BL].image) {
		bottomLeftWidth = parts[PART_BL].width;
		bottomLeftHeight = parts[PART_BL].height;
	} else {
		bottomLeftWidth = parts[PART_L].width;
		bottomLeftHeight = parts[PART_B].height;
	}
	if(parts[PART_BR].image) {
		bottomRightWidth = parts[PART_BR].width;
		bottomRightHeight = parts[PART_BR].height;
	} else {
		bottomRightWidth = parts[PART_R].width;
		bottomRightHeight = parts[PART_B].height;
	}

}

/****************************************************************************
 ****************************************************************************/
void ShutdownBorders() {

}

/****************************************************************************
 ****************************************************************************/
void DestroyBorders() {

}

/****************************************************************************
 ****************************************************************************/
int GetBorderIconSize() {

	int size;

	size = parts[PART_T2].width + abs(buttons[ACT_MENU].x);
	if(size > parts[PART_T2].height) {
		size = parts[PART_T2].height;
	}

	if(size > 4) {
		return size - 4;
	} else {
		return 4;
	}
}

/****************************************************************************
 ****************************************************************************/
BorderActionType GetBorderActionType(const ClientNode *np, int x, int y) {

	int north, south, east, west;
	int top;
	int offset;
	int temp;

	GetBorderSize(np, &north, &south, &east, &west);
	if(north > parts[PART_CLOSE].height) {
		top = (north - parts[PART_CLOSE].height) / 2;
	} else {
		top = north;
	}

	if(np->state.border & BORDER_TITLE) {

		offset = parts[PART_T1].width + buttons[ACT_MENU].x;
		if(y > buttons[ACT_MENU].y
			&& y <= buttons[ACT_MENU].y + parts[PART_T2].height) {
			if(x > offset && x < offset + parts[PART_T2].width) {
				return BA_MENU;
			}

		}

		offset = np->width + east + west - parts[PART_T11].width;
		if(np->state.border & BORDER_CLOSE) {
			offset -= parts[PART_CLOSE].width;
			if(y > buttons[ACT_CLOSE].y
				&& y <= buttons[ACT_CLOSE].y + parts[PART_CLOSE].height) {
				temp = offset + buttons[ACT_CLOSE].x;
				if(x > temp && x <= temp + parts[PART_CLOSE].width) {
					return BA_CLOSE;
				}
			}
		}
		if(np->state.border & BORDER_MAX) {
			offset -= parts[PART_MAX].width;
			if(y > buttons[ACT_MAX].y
				&& y <= buttons[ACT_MAX].y + parts[PART_MAX].height) {
				temp = offset + buttons[ACT_MAX].x;
				if(x > temp && x <= temp + parts[PART_MAX].width) {
					return BA_MAXIMIZE;
				}
			}
		}
		if(np->state.border & BORDER_MIN) {
			offset -= parts[PART_MIN].width;
			if(y > buttons[ACT_MIN].y
				&& y <= buttons[ACT_MIN].y + parts[PART_MIN].height) {
				temp = offset + buttons[ACT_MIN].x;
				if(x > temp && x <= temp + parts[PART_MIN].width) {
					return BA_MINIMIZE;
				}
			}
		}

		if(y > top && y <= north) {

			if(x > topLeftWidth && x <= np->width + east + west - topRightWidth) {
				if(np->state.border & BORDER_MOVE) {
					return BA_MOVE;
				} else {
					return BA_NONE;
				}
			}

		}

	}

	if(!(np->state.border & BORDER_RESIZE)) {
		return BA_NONE;
	}

	if(np->state.status & STAT_SHADED) {
		if(x < west) {
			return BA_RESIZE_W | BA_RESIZE;
		} else if(x >= np->width + west) {
			return BA_RESIZE_E | BA_RESIZE;
		} else {
			return BA_NONE;
		}
	}

	if(x < topLeftWidth && y < topLeftHeight) {
		return  BA_RESIZE_N | BA_RESIZE_W | BA_RESIZE;
	} else if(x < bottomLeftWidth
		&& y >= north + south + np->height - bottomLeftHeight) {
		return BA_RESIZE_S | BA_RESIZE_W | BA_RESIZE;
	} else if(x >= np->width + east + west - topRightWidth
		&& y < topRightHeight) {
		return BA_RESIZE_N | BA_RESIZE_E | BA_RESIZE;
	} else if(x >= np->width + east + west - bottomRightWidth
		&& y >= np->height + north + south - bottomRightHeight) {
		return BA_RESIZE_S | BA_RESIZE_E | BA_RESIZE;
	}

	if(x < west) {
		return BA_RESIZE_W | BA_RESIZE;
	} else if(x >= np->width + west) {
		return BA_RESIZE_E | BA_RESIZE;
	} else if(y < top) {
		return BA_RESIZE_N | BA_RESIZE;
	} else if(y >= north + np->height) {
		return BA_RESIZE_S | BA_RESIZE;
	} else {
		return BA_NONE;
	}
}

/****************************************************************************
 ****************************************************************************/
void DrawBorder(const ClientNode *np) {

	int x, y;
	int start, stop;
	int titleLength;
	int buttonWidth;
	int north;
	ColorType fg;

	if(shouldExit) {
		return;
	}

	if(!(np->state.status & (STAT_MAPPED | STAT_SHADED))) {
		return;
	}
	if(np->state.status & STAT_HIDDEN) {
		return;
	}

	if(!(np->state.border & BORDER_OUTLINE)
		&& !(np->state.border & BORDER_TITLE)) {
		return;
	}

	if(!(np->state.status & STAT_SHAPE) || (np->state.status & STAT_SHADED)) {
		ApplyBorderShape(np);
	}

	if(np->state.border & BORDER_TITLE) {

		buttonWidth = GetButtonWidth(np);

		/* Left. */
		start = DrawPart(np, PART_T1, 0, 0);

		/* Draw the window icon. */
		if(np->icon && parts[PART_T2].width > 0) {

			if(parts[PART_T2].width < parts[PART_T2].height) {
				x = parts[PART_T2].width;
			} else {
				x = parts[PART_T2].height;
			}

			DrawPart(np, PART_T2, start, 0);

			PutIcon(np->icon, np->parent, np->parentGC,
				start + buttons[ACT_MENU].x, buttons[ACT_MENU].y, x, x);

			start += parts[PART_T2].width;

		}


		start += DrawPart(np, PART_T3, start, 0);

		/* Skip PART_T4 for now, this will be used when the title
		 * is centered or right aligned. */

		start += DrawPart(np, PART_T5, start, 0);

		/* Draw the title. */
		if(np->name) {

			titleLength = GetStringWidth(FONT_BORDER, np->name);
			if(parts[PART_T6].width > 0) {
				for(x = 0; x < titleLength;) {
					x += DrawPart(np, PART_T6, x + start, 0);
				}
			}

			y = parts[PART_T6].height / 2 - GetStringHeight(FONT_BORDER) / 2;
			if(y < 0) {
				y = 0;
			}
			if(np->state.status & STAT_ACTIVE) {
				fg = COLOR_BORDER_ACTIVE_FG;
			} else {
				fg = COLOR_BORDER_FG;
			}
			RenderString(np->parent, np->parentGC, FONT_BORDER, fg,
				start, y, titleLength, np->name);
			start += titleLength;

		}

		start += DrawPart(np, PART_T7, start, 0);

		/* PART_T8 will be used to fill any left-over space. */
		stop = parts[PART_L].width + parts[PART_R].width + np->width;
		stop -= parts[PART_T11].width;
		stop -= buttonWidth;
		stop -= parts[PART_T9].width;
		if(parts[PART_T8].width > 0) {
			for(x = start; x < stop;) {
				x += DrawPart(np, PART_T8, x, 0);
			}
		}
		start = stop;

		start += DrawPart(np, PART_T9, start, 0);

		/* PART_T10 will be used to fill the area under the buttons. */
		if(parts[PART_T10].width > 0) {
			for(x = 0; x < buttonWidth;) {
				x += DrawPart(np, PART_T10, x + start, 0);
			}
		}
		start += buttonWidth;

		/* Right. */
		DrawPart(np, PART_T11, start, 0);

		north = parts[PART_T6].height;

	} else if(np->state.border & BORDER_OUTLINE) {

		start = DrawPart(np, PART_TL, 0, 0);
		stop = parts[PART_L].width + parts[PART_R].width + np->width;
		stop -= parts[PART_TR].width;

		if(parts[PART_T].width > 0) {
			for(x = start; x < stop;) {
				x += DrawPart(np, PART_T, x, 0);
			}
		}

		DrawPart(np, PART_TR, stop, 0);

		north = parts[PART_T].height;

	} else {

		north = 0;

	}

	if(np->state.border & BORDER_OUTLINE) {

		/* Left. */
		if(parts[PART_L].height > 0) {
			stop = np->height + north + parts[PART_B].height;
			for(y = parts[PART_T1].height; y < stop;) {
				DrawPart(np, PART_L, 0, y);
				y += parts[PART_L].height;
			}
		}

		/* Right. */
		if(parts[PART_R].height > 0) {
			x = np->width + parts[PART_L].width;
			for(y = north; y < np->height + north + parts[PART_B].height;) {
				DrawPart(np, PART_R, x, y);
				y += parts[PART_R].height;
			}
		}

		/* Bottom left. */
		y = np->height + north + parts[PART_B].height;
		y -= parts[PART_BL].height;
		start = DrawPart(np, PART_BL, 0, y);

		/* Bottom. */
		if(parts[PART_B].width > 0) {
			y = np->height + north;
			stop = np->width + parts[PART_L].width + parts[PART_R].width;
			for(x = start; x < stop;) {
				x += DrawPart(np, PART_B, x, y);
			}
		}

		/* Bottom right. */
		x = np->width + parts[PART_L].width + parts[PART_R].width;
		x -= parts[PART_BR].width;
		y = np->height + north + parts[PART_B].height;
		y -= parts[PART_BR].height;
		DrawPart(np, PART_BR, x, y);

	}

	DrawButtons(np);

}

/****************************************************************************
 ****************************************************************************/
int GetButtonWidth(const ClientNode *np) {

	int result = 0;

	if(np->state.border & BORDER_CLOSE) {
		result = parts[PART_CLOSE].width;
	}
	if(np->state.border & BORDER_MIN) {
		result += parts[PART_MIN].width;
	}
	if(np->state.border & BORDER_MAX) {
		result += parts[PART_MAX].width;
	}

	return result;

}

/****************************************************************************
 ****************************************************************************/
void DrawButtons(const ClientNode *np) {

	int start;

	if(!(np->state.border & BORDER_TITLE)) {
		return;
	}

	start = np->width + parts[PART_L].width + parts[PART_R].width;
	start -= parts[PART_T11].width;

	if(np->state.border & BORDER_CLOSE) {
		start -= parts[PART_CLOSE].width;
		DrawPart(np, PART_CLOSE, start + buttons[ACT_CLOSE].x,
			buttons[ACT_CLOSE].y);
	}
	if(np->state.border & BORDER_MAX) {
		start -= parts[PART_MAX].width;
		DrawPart(np, PART_MAX, start + buttons[ACT_MAX].x,
			buttons[ACT_MAX].y);
	}
	if(np->state.border & BORDER_MIN) {
		start -= parts[PART_MIN].width;
		DrawPart(np, PART_MIN, start + buttons[ACT_MIN].x,
			buttons[ACT_MIN].y);
	}

}

/****************************************************************************
 ****************************************************************************/
void ApplyBorderShape(const ClientNode *np) {
#ifdef USE_SHAPE

	if(borderUsesShape) {
		ApplyComplexBorderShape(np);
	} else if(np->state.status & STAT_SHAPE) {
		ApplySimpleBorderShape(np);
	}

#endif
}

/****************************************************************************
 ****************************************************************************/
#ifdef USE_SHAPE
void ApplyComplexBorderShape(const ClientNode *np) {

	Pixmap shape;
	GC gc;
	int north, south, east, west;
	int buttonWidth;
	int titleLength;
	int x, y;
	int start, stop;

	GetBorderSize(np, &north, &south, &east, &west);

	if(np->state.status & STAT_SHADED) {

		shape = JXCreatePixmap(display, np->parent,
			np->width + east + west, north + south, 1);
		gc = JXCreateGC(display, shape, 0, NULL);

		JXSetForeground(display, gc, 1);
		JXFillRectangle(display, shape, gc, 0, 0,
			np->width + east + west, north + south);

	} else {

		shape = JXCreatePixmap(display, np->parent,
			np->width + east + west, np->height + north + south, 1);
		gc = JXCreateGC(display, shape, 0, NULL);

		JXSetForeground(display, gc, 1);
		JXFillRectangle(display, shape, gc, 0, 0,
			np->width + east + west, np->height + north + south);

	}

	if(np->state.border & BORDER_TITLE) {

		buttonWidth = GetButtonWidth(np);

		/* Left. */
		start = DrawShape(np, shape, gc, PART_T1, 0, 0);

		/* Window icon. */
		if(np->icon && parts[PART_T2].width > 0) {

			DrawShape(np, shape, gc, PART_T2, start, 0);

			start += parts[PART_T2].width;

		}


		start += DrawShape(np, shape, gc, PART_T3, start, 0);

		/* Skip PART_T4 for now, this will be used when the title
		 * is centered or right aligned. */

		start += DrawShape(np, shape, gc, PART_T5, start, 0);

		/* Title */
		if(np->name) {

			titleLength = GetStringWidth(FONT_BORDER, np->name);
			if(parts[PART_T6].width > 0) {
				for(x = 0; x < titleLength;) {
					x += DrawShape(np, shape, gc, PART_T6, x + start, 0);
				}
			}

			start += titleLength;

		}

		start += DrawShape(np, shape, gc, PART_T7, start, 0);

		/* PART_T8 will be used to fill any left-over space. */
		stop = parts[PART_L].width + parts[PART_R].width + np->width;
		stop -= parts[PART_T11].width;
		stop -= buttonWidth;
		stop -= parts[PART_T9].width;
		if(parts[PART_T8].width > 0) {
			for(x = start; x < stop;) {
				x += DrawShape(np, shape, gc, PART_T8, x, 0);
			}
		}
		start = stop;

		start += DrawShape(np, shape, gc, PART_T9, start, 0);

		/* PART_T10 will be used to fill the area under the buttons. */
		if(parts[PART_T10].width > 0) {
			for(x = 0; x < buttonWidth;) {
				x += DrawShape(np, shape, gc, PART_T10, x + start, 0);
			}
		}
		start += buttonWidth;

		/* Right. */
		DrawShape(np, shape, gc, PART_T11, start, 0);

	} else if(np->state.border & BORDER_OUTLINE) {

		start = DrawShape(np, shape, gc, PART_TL, 0, 0);
		stop = parts[PART_L].width + parts[PART_R].width + np->width;
		stop -= parts[PART_TR].width;

		if(parts[PART_T].width > 0) {
			for(x = start; x < stop;) {
				x += DrawShape(np, shape, gc, PART_T, x, 0);
			}
		}

		DrawShape(np, shape, gc, PART_TR, stop, 0);

	}

	if(np->state.border & BORDER_OUTLINE) {

		/* Left. */
		if(parts[PART_L].height > 0) {
			stop = np->height + north + parts[PART_B].height;
			for(y = parts[PART_T1].height; y < stop;) {
				DrawShape(np, shape, gc, PART_L, 0, y);
				y += parts[PART_L].height;
			}
		}

		/* Right. */
		if(parts[PART_R].height > 0) {
			x = np->width + parts[PART_L].width;
			for(y = north; y < np->height + north + parts[PART_B].height;) {
				DrawShape(np, shape, gc, PART_R, x, y);
				y += parts[PART_R].height;
			}
		}

		/* Bottom left. */
		y = np->height + north + parts[PART_B].height;
		y -= parts[PART_BL].height;
		start = DrawShape(np, shape, gc, PART_BL, 0, y);

		/* Bottom. */
		if(parts[PART_B].width > 0) {
			y = np->height + north;
			stop = np->width + parts[PART_L].width + parts[PART_R].width;
			for(x = start; x < stop;) {
				x += DrawShape(np, shape, gc, PART_B, x, y);
			}
		}

		/* Bottom right. */
		x = np->width + parts[PART_L].width + parts[PART_R].width;
		x -= parts[PART_BR].width;
		y = np->height + north + parts[PART_B].height;
		y -= parts[PART_BR].height;
		DrawShape(np, shape, gc, PART_BR, x, y);

	}

	if(!(np->state.status & STAT_SHADED) && (np->state.status & STAT_SHAPE)) {

		JXSetForeground(display, gc, 0);
		JXFillRectangle(display, shape, gc, west, north,
			np->width, np->height);

	}

	JXShapeCombineMask(display, np->parent, ShapeBounding, 0, 0,
		shape, ShapeSet);

	if(!(np->state.status & STAT_SHADED) && (np->state.status & STAT_SHAPE)) {

		JXShapeCombineShape(display, np->parent, ShapeBounding, west, north,
			np->window, ShapeBounding, ShapeUnion);

	}

	JXFreeGC(display, gc);
	JXFreePixmap(display, shape);

}
#endif

/****************************************************************************
 ****************************************************************************/
#ifdef USE_SHAPE
void ApplySimpleBorderShape(const ClientNode *np) {

	XRectangle rect[4];
	int north, south, east, west;

	GetBorderSize(np, &north, &south, &east, &west);

	if(np->state.status & STAT_SHADED) {

		rect[0].x = 0;
		rect[0].y = 0;
		rect[0].width = np->width + east + west;
		rect[0].height = np->height + north + south;

		JXShapeCombineRectangles(display, np->parent, ShapeBounding,
			0, 0, rect, 1, ShapeSet, Unsorted);

	} else {

		JXShapeCombineShape(display, np->parent, ShapeBounding, west, north,
			np->window, ShapeBounding, ShapeSet);

		if(north > 0) {

			/* Top */
			rect[0].x = 0;
			rect[0].y = 0;
			rect[0].width = np->width + west * 2;
			rect[0].height = north;

			/* Left */
			rect[1].x = 0;
			rect[1].y = 0;
			rect[1].width = west;
			rect[1].height = np->height + west + north;

			/* Right */
			rect[2].x = np->width + west;
			rect[2].y = 0;
			rect[2].width = west;
			rect[2].height = np->height + west + north;

			/* Bottom */
			rect[3].x = 0;
			rect[3].y = np->height + north;
			rect[3].width = np->width + west * 2;
			rect[3].height = west;

			JXShapeCombineRectangles(display, np->parent, ShapeBounding,
				0, 0, rect, 4, ShapeUnion, Unsorted);

		}

	}

}
#endif

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
				DrawBorder(np);
			}
		}
	}

}

/****************************************************************************
 ****************************************************************************/
int DrawPart(const ClientNode *np, PartType part, int x, int y) {

	int offset;

	if(parts[part].image) {

		if(np->state.status & STAT_ACTIVE) {
			offset = 0;
		} else {
			offset = parts[part].height;
		}

		JXCopyArea(display, parts[part].image->image, np->parent,
			np->parentGC, 0, offset,
			parts[part].width, parts[part].height, x, y);

	}

	return parts[part].width;

}

/****************************************************************************
 ****************************************************************************/
int DrawShape(const ClientNode *np, Drawable d, GC g, PartType part,
	int x, int y) {

	int offset;

	if(parts[part].image && parts[part].image->shape != None) {

		if(np->state.status & STAT_ACTIVE) {
			offset = 0;
		} else {
			offset = parts[part].height;
		}

		JXCopyArea(display, parts[part].image->shape, d, g, 0, offset,
			parts[part].image->width, parts[part].height, x, y);

	}

	return parts[part].width;

}

/****************************************************************************
 ****************************************************************************/
void GetBorderSize(const struct ClientNode *np,
	int *north, int *south, int *east, int *west) {

	Assert(np);

	if(np->state.border & BORDER_TITLE) {
		*north = parts[PART_T6].height;
	} else if(np->state.border & BORDER_OUTLINE) {
		*north = parts[PART_T].height;
	} else {
		*north = 0;
	}

	if(np->state.border & BORDER_OUTLINE) {
		*west = parts[PART_L].width;
		*east = parts[PART_R].width;
		if(np->state.status & STAT_SHADED) {
			*south = 0;
		} else {
			*south = parts[PART_B].height;
		}
	} else {
		*west = 0;
		*east = 0;
		*south = 0;
	}

}

/****************************************************************************
 ****************************************************************************/
void SetWindowButtonLocation(const char *action, int x, int y) {

	int index;

	for(index = 0; index < ACT_COUNT; index++) {
		if(!strcmp(buttons[index].action, action)) {
			buttons[index].x = x;
			buttons[index].y = y;
			return;
		}
	}

	Warning("invalid window button action: \"%s\"", action);

}

