/***************************************************************************
 * Functions to handle themes.
 * Copyright (C) 2006 Joe Wingbermuehle
 ***************************************************************************/

#include "jwm.h"
#include "main.h"
#include "theme.h"
#include "misc.h"
#include "image.h"
#include "error.h"

#define OFFSET_TL 0
#define OFFSET_TR 1
#define OFFSET_BL 2
#define OFFSET_BR 3
#define OFFSET_T  4
#define OFFSET_B  5
#define OFFSET_L  6
#define OFFSET_R  7
#define OFFSET_F  8

#define HasShape( part ) \
	( parts[part].image && parts[part].image->shape != None )

PartNode parts[PART_COUNT] = {

	{ "title1.xpm",    2 },
	{ "title2.xpm",    2 },
	{ "title3.xpm",    2 },
	{ "title4.xpm",    2 },
	{ "title5.xpm",    2 },
	{ "title6.xpm",    2 },
	{ "title7.xpm",    2 },
	{ "title8.xpm",    2 },
	{ "title9.xpm",    2 },
	{ "title10.xpm",   2 },
	{ "title11.xpm",   2 },

	{ "borderT.xpm",   2 },
	{ "borderTL.xpm",  2 },
	{ "borderTR.xpm",  2 },

	{ "borderL.xpm",   2 },
	{ "borderR.xpm",   2 },
	{ "borderB.xpm",   2 },
	{ "borderBL.xpm",  2 },
	{ "borderBR.xpm",  2 },

	{ "min.xpm",       2 },
	{ "max.xpm",       2 },
	{ "close.xpm",     2 },

	{ "buttonTL.xpm",  3 },
	{ "buttonTR.xpm",  3 },
	{ "buttonBL.xpm",  3 },
	{ "buttonBR.xpm",  3 },
	{ "buttonT.xpm",   3 },
	{ "buttonB.xpm",   3 },
	{ "buttonL.xpm",   3 },
	{ "buttonR.xpm",   3 },
	{ "buttonF.xpm",   3 },

	{ "trayTL.xpm",    1 },
	{ "trayTR.xpm",    1 },
	{ "trayBL.xpm",    1 },
	{ "trayBR.xpm",    1 },
	{ "trayT.xpm",     1 },
	{ "trayB.xpm",     1 },
	{ "trayL.xpm",     1 },
	{ "trayR.xpm",     1 },
	{ "trayF.xpm",     1 },

	{ "menuTL.xpm",    1 },
	{ "menuTR.xpm",    1 },
	{ "menuBL.xpm",    1 },
	{ "menuBR.xpm",    1 },
	{ "menuT.xpm",     1 },
	{ "menuB.xpm",     1 },
	{ "menuL.xpm",     1 },
	{ "menuR.xpm",     1 },
	{ "menuF.xpm",     1 },
	{ "menuS.xpm",     1 },
	{ "submenu.xpm",   2 }

};

static char *themePath;

static void LoadThemePart(PartType part);
static void DrawPart(PartType part, Drawable d, GC g,
	int x, int y, int index);
static void DrawShape(PartType part, Drawable d, GC g, int x, int y);

/***************************************************************************
 ***************************************************************************/
void InitializeTheme() {

	themePath = NULL;

}

/***************************************************************************
 ***************************************************************************/
void StartupTheme() {

	int x;

	if(themePath) {

		for(x = 0; x < PART_COUNT; x++) {
			LoadThemePart(x);
		}

	} else {

		Warning("no theme specified");

	}

}

/***************************************************************************
 ***************************************************************************/
void LoadThemePart(PartType part) {

	char *fullName;

	fullName = AllocateStack(strlen(themePath) + strlen(parts[part].name) + 2);
	strcpy(fullName, themePath);
	strcat(fullName, "/");
	strcat(fullName, parts[part].name);
	parts[part].image = LoadThemeImage(fullName);
	ReleaseStack(fullName);

	if(parts[part].image != NULL) {
		parts[part].width = parts[part].image->width;
		parts[part].height = parts[part].image->height / parts[part].count;
	} else {
		parts[part].width = 0;
		parts[part].height = 0;
		Debug("theme image %s not found", parts[part].name);
	}

}

/***************************************************************************
 ***************************************************************************/
void ShutdownTheme() {

	int x;

	for(x = 0; x < PART_COUNT; x++) {
		if(parts[x].image) {
			DestroyThemeImage(parts[x].image);
			parts[x].image = NULL;
		}
	}

}

/***************************************************************************
 ***************************************************************************/
void DestroyTheme() {

	if(themePath) {
		Release(themePath);
	}

}

/***************************************************************************
 ***************************************************************************/
void DrawThemeOutline(PartType part, Drawable d, GC g,
	int xoffset, int yoffset, int width, int height, int index) {

	int x;

	/* Top left. */
	if(parts[part + OFFSET_TL].image) {
		DrawPart(part + OFFSET_TL, d, g, xoffset, yoffset, index);
	}

	/* Top. */
	if(parts[part + OFFSET_T].image) {
		x = parts[part + OFFSET_TL].width;
		while(x < width) {
			DrawPart(part + OFFSET_T, d, g, x + xoffset, yoffset, index);
			x += parts[part + OFFSET_T].width;
		}
	}

	/* Top right. */
	if(parts[part + OFFSET_TR].image) {
		DrawPart(part + OFFSET_TR, d, g,
			width - parts[part + OFFSET_TR].width + xoffset, yoffset, index);
	}

	/* Left. */
	if(parts[part + OFFSET_L].image) {
		x = parts[part + OFFSET_TL].height;
		while(x < height) {
			DrawPart(part + OFFSET_L, d, g, xoffset, x + yoffset, index);
			x += parts[part + OFFSET_L].height;
		}
	}

	/* Right. */
	if(parts[part + OFFSET_R].image) {
		x = parts[part + OFFSET_TR].height;
		while(x < height) {
			DrawPart(part + OFFSET_R, d, g,
				width - parts[part + OFFSET_R].width + xoffset,
				x + yoffset, index);
			x += parts[part + OFFSET_R].height;
		}
	}

	/* Bottom left. */
	if(parts[part + OFFSET_BL].image) {
		DrawPart(part + OFFSET_BL, d, g,
			xoffset, height - parts[part + OFFSET_BL].height + yoffset, index);
	}

	/* Bottom. */
	if(parts[part + OFFSET_B].image) {
		x = parts[part + OFFSET_BL].width;
		while(x < width) {
			DrawPart(part + OFFSET_B, d, g,
				x + xoffset,
				height - parts[part + OFFSET_B].height + yoffset, index);
			x += parts[part + OFFSET_B].width;
		}
	}

	/* Bottom right. */
	if(parts[part + OFFSET_BR].image) {
		DrawPart(part + OFFSET_BR, d, g,
			width - parts[part + OFFSET_BR].width + xoffset,
			height - parts[part + OFFSET_BR].height + yoffset, index);
	}

}

/***************************************************************************
 ***************************************************************************/
void DrawThemeBackground(PartType part, ColorType color, Drawable d, GC g,
	int x, int y, int width, int height, int index) {

	int xc, yc;

	if(parts[part + OFFSET_F].image) {
		for(yc = 0; yc < height;) {
			for(xc = 0; xc < width;) {
				DrawPart(part + OFFSET_F, d, g, x + xc, y + yc, index);
				xc += parts[part + OFFSET_F].width;
			}
			yc += parts[part + OFFSET_F].height;
		}
	} else {
		JXSetForeground(display, g, colors[color]);
		JXFillRectangle(display, d, g, x, y, width, height);
	}

}

/***************************************************************************
 ***************************************************************************/
void ApplyThemeShape(PartType part, Window w, int width, int height) {
#ifdef USE_SHAPE

	int x;
	int shapeUsed;
	Pixmap shape;
	GC gc;

	shapeUsed = 0;

	shape = JXCreatePixmap(display, w, width, height, 1);
	gc = JXCreateGC(display, shape, 0, NULL);

	JXSetForeground(display, gc, 1);
	JXFillRectangle(display, shape, gc, 0, 0, width, height);

	/* Top. */
	if(HasShape(part + OFFSET_T)) {
		for(x = 0; x < width;) {
			DrawShape(part + OFFSET_T, shape, gc, x, 0);
			x += parts[part + OFFSET_T].width;
		}
		shapeUsed = 1;
	}

	/* Left. */
	if(HasShape(part + OFFSET_L)) {
		for(x = 0; x < height;) {
			DrawShape(part + OFFSET_L, shape, gc, 0, x);
			x += parts[part + OFFSET_L].height;
		}
		shapeUsed = 1;
	}

	/* Right. */
	if(HasShape(part + OFFSET_R)) {
		for(x = 0; x < height;) {
			DrawShape(part + OFFSET_R, shape, gc,
				height - parts[part + OFFSET_R].height, x);
			x += parts[part + OFFSET_R].height;
		}
		shapeUsed = 1;
	}

	/* Bottom. */
	if(HasShape(part + OFFSET_B)) {
		for(x = 0; x < width;) {
			DrawShape(part + OFFSET_B, shape, gc,
				x, height - parts[part + OFFSET_B].height);
			x += parts[part + OFFSET_B].width;
		}
		shapeUsed = 1;
	}

	/* Top left. */
	if(HasShape(part + OFFSET_TL)) {
		DrawShape(part + OFFSET_TL, shape, gc, 0, 0);
		shapeUsed = 1;
	}

	/* Top right. */
	if(HasShape(part + OFFSET_TR)) {
		DrawShape(part + OFFSET_TR, shape, gc,
			width - parts[part + OFFSET_TR].width, 0);
		shapeUsed = 1;
	}

	/* Bottom left. */
	if(HasShape(part + OFFSET_BL)) {
		DrawShape(part + OFFSET_BL, shape, gc,
			0, height - parts[part + OFFSET_BL].height);
		shapeUsed = 1;
	}

	/* Bottom right. */
	if(HasShape(part + OFFSET_BR)) {
		DrawShape(part + OFFSET_BR, shape, gc,
			width - parts[part + OFFSET_BR].width,
			height - parts[part + OFFSET_BR].height);
		shapeUsed = 1;
	}

	if(shapeUsed) {
		JXShapeCombineMask(display, w, ShapeBounding, 0, 0, shape, ShapeSet);
	}

	JXFreeGC(display, gc);
	JXFreePixmap(display, shape);

#endif
}

/***************************************************************************
 ***************************************************************************/
void DrawPart(PartType part, Drawable d, GC g, int x, int y, int index) {

	JXCopyArea(display, parts[part].image->image, d, g,
		0, index * parts[part].height,
		parts[part].width, parts[part].height, x, y);

}

/***************************************************************************
 ***************************************************************************/
void DrawShape(PartType part, Drawable d, GC g, int x, int y) {

	JXCopyArea(display, parts[part].image->shape, d, g, 0, 0,
		parts[part].width, parts[part].height, x, y);

}

/***************************************************************************
 ***************************************************************************/
void SetThemePath(const char *path, const char *name) {

	if(path && name) {
		themePath = Allocate(strlen(path) + strlen(name) + 2);
		sprintf(themePath, "%s/%s", path, name);
		ExpandPath(&themePath);
	}

}

