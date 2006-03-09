/****************************************************************************
 * Functions to handle loading colors.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"
#include "main.h"
#include "color.h"
#include "error.h"
#include "misc.h"

/* Note: COLOR_HASH_SIZE = 2 ** (3 * COLOR_HASH_BITS) */
#define COLOR_HASH_BITS 4
#define COLOR_HASH_SIZE 4096

typedef struct ColorNode {
	unsigned short red, green, blue;
	unsigned long pixel;
	struct ColorNode *next;
} ColorNode;

typedef struct {
	ColorType type;
	const char *value;
} DefaultColorNode;

static ColorNode **colorHash = NULL;

static const float COLOR_DELTA = 0.45;

unsigned long colors[COLOR_COUNT];
unsigned long rgbColors[COLOR_COUNT];
unsigned long white;
unsigned long black;

#ifdef USE_XFT
static XftColor *xftColors[COLOR_COUNT] = { NULL };
#endif

static DefaultColorNode DEFAULT_COLORS[] = {
	{ COLOR_BORDER_BG,          "gray"    },
	{ COLOR_BORDER_FG,          "black"   },
	{ COLOR_BORDER_ACTIVE_BG,   "red"     },
	{ COLOR_BORDER_ACTIVE_FG,   "white"   },
	{ COLOR_TRAY_BG,            "gray"    },
	{ COLOR_TRAY_FG,            "black"   },
	{ COLOR_TASK_BG,            "gray"    },
	{ COLOR_TASK_FG,            "black"   },
	{ COLOR_TASK_ACTIVE_BG,     "red"     },
	{ COLOR_TASK_ACTIVE_FG,     "white"   },
	{ COLOR_PAGER_BG,           "black"   },
	{ COLOR_PAGER_FG,           "gray"    },
	{ COLOR_PAGER_ACTIVE_BG,    "red"     },
	{ COLOR_PAGER_ACTIVE_FG,    "red"     },
	{ COLOR_PAGER_OUTLINE,      "black"   },
	{ COLOR_MENU_BG,            "gray"    },
	{ COLOR_MENU_FG,            "black"   },
	{ COLOR_MENU_ACTIVE_BG,     "red"     },
	{ COLOR_MENU_ACTIVE_FG,     "white"   },
	{ COLOR_POPUP_BG,           "yellow"  },
	{ COLOR_POPUP_FG,           "black"   },
	{ COLOR_POPUP_OUTLINE,      "black"   },
	{ COLOR_TRAYBUTTON_FG,      "black"   },
	{ COLOR_TRAYBUTTON_BG,      "gray"    },
	{ COLOR_CLOCK_FG,           "black"   },
	{ COLOR_CLOCK_BG,           "gray"    },
	{ COLOR_COUNT,              NULL      }
};

static int ParseColor(ColorType type, const char *value);
static void SetDefaultColor(ColorType type); 

static unsigned long ReadHex(const char *hex);

static unsigned long GetRGBFromXColor(const XColor *c);
static XColor GetXColorFromRGB(unsigned long rgb);

static int GetColorByName(const char *str, XColor *c);
static unsigned long FindClosestColor(const XColor *c);
static void InitializeNames();

static void LightenColor(ColorType oldColor, ColorType newColor);
static void DarkenColor(ColorType oldColor, ColorType newColor);

static char **names = NULL;

/****************************************************************************
 ****************************************************************************/
unsigned long GetRGBFromXColor(const XColor *c) {
	float red, green, blue;
	unsigned long rgb;

	red = (float)c->red / 65535.0;
	green = (float)c->green / 65535.0;
	blue = (float)c->blue / 65535.0;

	rgb = (unsigned long)(red * 255.0) << 16;
	rgb |= (unsigned long)(green * 255.0) << 8;
	rgb |= (unsigned long)(blue * 255.0);

	return rgb;
}

/****************************************************************************
 ****************************************************************************/
XColor GetXColorFromRGB(unsigned long rgb) {

	XColor ret = { 0 };

	ret.flags = DoRed | DoGreen | DoBlue;
	ret.red = (unsigned short)(((rgb >> 16) & 0xFF) * 257);
	ret.green = (unsigned short)(((rgb >> 8) & 0xFF) * 257);
	ret.blue = (unsigned short)((rgb & 0xFF) * 257);

	return ret;

}

/****************************************************************************
 ****************************************************************************/
void InitializeColors() {
	int x;

	colorHash = Allocate(sizeof(ColorNode*) * COLOR_HASH_SIZE);
	for(x = 0; x < COLOR_HASH_SIZE; x++) {
		colorHash[x] = NULL;
	}

}

/****************************************************************************
 ****************************************************************************/
void StartupColors() {
	int x;

	/* Inherit unset colors from the tray for tray items. */
	if(names) {
		if(!names[COLOR_TASK_BG]) {
			names[COLOR_TASK_BG] = CopyString(names[COLOR_TRAY_BG]);
		}
		if(!names[COLOR_TRAYBUTTON_BG]) {
			names[COLOR_TRAYBUTTON_BG] = CopyString(names[COLOR_TRAY_BG]);
		}
		if(!names[COLOR_CLOCK_BG]) {
			names[COLOR_CLOCK_BG] = CopyString(names[COLOR_TRAY_BG]);
		}
		if(!names[COLOR_TASK_FG]) {
			names[COLOR_TASK_FG] = CopyString(names[COLOR_TRAY_FG]);
		}
		if(!names[COLOR_TRAYBUTTON_FG]) {
			names[COLOR_TRAYBUTTON_FG] = CopyString(names[COLOR_TRAY_FG]);
		}
		if(!names[COLOR_CLOCK_FG]) {
			names[COLOR_CLOCK_FG] = CopyString(names[COLOR_TRAY_FG]);
		}
	}

	for(x = 0; x < COLOR_COUNT; x++) {
		if(names && names[x]) {
			if(!ParseColor(x, names[x])) {
				SetDefaultColor(x);
			}
		} else {
			SetDefaultColor(x);
		}
	}

	if(names) {
		for(x = 0; x < COLOR_COUNT; x++) {
			if(names[x]) {
				Release(names[x]);
			}
		}
		Release(names);
		names = NULL;
	}

	LightenColor(COLOR_BORDER_BG, COLOR_BORDER_UP);
	DarkenColor(COLOR_BORDER_BG, COLOR_BORDER_DOWN);

	LightenColor(COLOR_BORDER_ACTIVE_BG, COLOR_BORDER_ACTIVE_UP);
	DarkenColor(COLOR_BORDER_ACTIVE_BG, COLOR_BORDER_ACTIVE_DOWN);

	LightenColor(COLOR_TRAY_BG, COLOR_TRAY_UP);
	DarkenColor(COLOR_TRAY_BG, COLOR_TRAY_DOWN);

	LightenColor(COLOR_TASK_BG, COLOR_TASK_UP);
	DarkenColor(COLOR_TASK_BG, COLOR_TASK_DOWN);

	LightenColor(COLOR_TASK_ACTIVE_BG, COLOR_TASK_ACTIVE_UP);
	DarkenColor(COLOR_TASK_ACTIVE_BG, COLOR_TASK_ACTIVE_DOWN);

	LightenColor(COLOR_MENU_BG, COLOR_MENU_UP);
	DarkenColor(COLOR_MENU_BG, COLOR_MENU_DOWN);

	LightenColor(COLOR_MENU_ACTIVE_BG, COLOR_MENU_ACTIVE_UP);
	DarkenColor(COLOR_MENU_ACTIVE_BG, COLOR_MENU_ACTIVE_DOWN);

	white = WhitePixel(display, rootScreen);
	black = BlackPixel(display, rootScreen);

}

/****************************************************************************
 ****************************************************************************/
void ShutdownColors() {
	ColorNode *np;
	int x;

	if(colorHash) {
		for(x = 0; x < COLOR_HASH_SIZE; x++) {
			for(np = colorHash[x]; np; np = np->next) {
				JXFreeColors(display, rootColormap, &np->pixel, 1, 0);
			}
		}
	}

#ifdef USE_XFT
	for(x = 0; x < COLOR_COUNT; x++) {
		if(xftColors[x]) {
			JXftColorFree(display, rootVisual, rootColormap, xftColors[x]);
			Release(xftColors[x]);
			xftColors[x] = NULL;
		}
	}
#endif

}

/****************************************************************************
 ****************************************************************************/
void DestroyColors() {
	ColorNode *np;
	int x;

	if(names) {
		for(x = 0; x < COLOR_COUNT; x++) {
			if(names[x]) {
				Release(names[x]);
			}
		}
		Release(names);
		names = NULL;
	}

	if(colorHash) {

		for(x = 0; x < COLOR_HASH_SIZE; x++) {
			while(colorHash[x]) {
				np = colorHash[x]->next;
				Release(colorHash[x]);
				colorHash[x] = np;
			}
		}

		Release(colorHash);
		colorHash = NULL;

	}

}

/****************************************************************************
 ****************************************************************************/
void SetColor(ColorType c, const char *value) {

	if(!value) {
		Warning("empty color tag");
		return;
	}
	Assert(value);

	InitializeNames();

	if(names[c]) {
		Release(names[c]);
	}

	names[c] = Allocate(strlen(value) + 1);
	strcpy(names[c], value);

}

/****************************************************************************
 ****************************************************************************/
int ParseColor(ColorType type, const char *value) {
	XColor temp;
	unsigned long rgb;

	if(!value) {
		return 0;
	}

	if(value[0] == '#' && strlen(value) == 7) {
		rgb = ReadHex(value + 1);
		temp.red = ((rgb >> 16) & 0xFF) * 257;
		temp.green = ((rgb >> 8) & 0xFF) * 257;
		temp.blue = (rgb & 0xFF) * 257;
		temp.flags = DoRed | DoGreen | DoBlue;
		GetColor(&temp);
	} else {
		if(!GetColorByName(value, &temp)) {
			Warning("bad color: \"%s\"", value);
			return 0;
		}
	}
	colors[type] = temp.pixel;
	rgbColors[type] = GetRGBFromXColor(&temp);

	return 1;
}

/****************************************************************************
 ****************************************************************************/
void SetDefaultColor(ColorType type) {
	int x;

	for(x = 0; DEFAULT_COLORS[x].value; x++) {
		if(DEFAULT_COLORS[x].type == type) {
			ParseColor(type, DEFAULT_COLORS[x].value);
			return;
		}
	}

}

/****************************************************************************
 ****************************************************************************/
void InitializeNames() {
	int x;

	if(names == NULL) {
		names = Allocate(sizeof(char*) * COLOR_COUNT);
		for(x = 0; x < COLOR_COUNT; x++) {
			names[x] = NULL;
		}
	}
}

/****************************************************************************
 ****************************************************************************/
unsigned long ReadHex(const char *hex) {
	unsigned long value = 0;
	int x;

	for(x = 0; hex[x]; x++) {
		value *= 16;
		if(hex[x] >= '0' && hex[x] <= '9') {
			value += hex[x] - '0';
		} else if(hex[x] >= 'A' && hex[x] <= 'F') {
			value += hex[x] - 'A' + 10;
		} else if(hex[x] >= 'a' && hex[x] <= 'f') {
			value += hex[x] - 'a' + 10;
		}
	}

	return value;
}

/****************************************************************************
 ****************************************************************************/
void LightenColor(ColorType oldColor, ColorType newColor) {

	XColor temp;
	float red, green, blue;
	float delta = 1.0 + COLOR_DELTA;

	temp = GetXColorFromRGB(rgbColors[oldColor]);

	red = (float)temp.red / 65535.0;
	green = (float)temp.green / 65535.0;
	blue = (float)temp.blue / 65535.0;

	red = Min(delta * red, 1.0);
	green = Min(delta * green, 1.0);
	blue = Min(delta * blue, 1.0);

	temp.red = red * 65535.0;
	temp.green = green * 65535.0;
	temp.blue = blue * 65535.0;

	GetColor(&temp);
	colors[newColor] = temp.pixel;
	rgbColors[newColor] = GetRGBFromXColor(&temp);

}

/****************************************************************************
 ****************************************************************************/
void DarkenColor(ColorType oldColor, ColorType newColor) {

	XColor temp;
	float red, green, blue;
	float delta = 1.0 - COLOR_DELTA;

	temp = GetXColorFromRGB(rgbColors[oldColor]);

	red = (float)temp.red / 65535.0;
	green = (float)temp.green / 65535.0;
	blue = (float)temp.blue / 65535.0;

	red = delta * red;
	green = delta * green;
	blue = delta * blue;

	temp.red = red * 65535.0;
	temp.green = green * 65535.0;
	temp.blue = blue * 65535.0;

	GetColor(&temp);
	colors[newColor] = temp.pixel;
	rgbColors[newColor] = GetRGBFromXColor(&temp);

}

/***************************************************************************
 ***************************************************************************/
int GetColorByName(const char *str, XColor *c) {

	Assert(str);
	Assert(c);

	if(!JXParseColor(display, rootColormap, str, c)) {
		return 0;
	}

	GetColor(c);

	return 1;

}

/***************************************************************************
 ***************************************************************************/
unsigned long FindClosestColor(const XColor *c) {

	ColorNode *closest;
	ColorNode *np;
	unsigned long closestDistance;
	unsigned long distance;
	int x;

	Assert(c);

	closest = NULL;
	closestDistance = 0;
	for(x = 0; x < COLOR_HASH_SIZE; x++) {
		np = colorHash[x];
		while(np) {

			distance = abs(c->red - np->red)
				+ abs(c->green - np->green)
				+ abs(c->blue - np->blue);

			if(!closest || distance < closestDistance) {
				closest = np;
				closestDistance = distance;
			}

			np = np->next;
		}
	}

	if(closest) {
		return closest->pixel;
	} else {
		return black;
	}

}

/***************************************************************************
 ***************************************************************************/
void GetColorFromPixel(XColor *c) {

	switch(rootDepth) {
	case 32:
	case 24:
		c->red = c->pixel >> 16;
		c->red |= c->red << 8;
		c->green = (c->pixel >> 8) & 0xFF;
		c->green |= c->green << 8;
		c->blue = c->pixel & 0xFF;
		c->blue |= c->blue << 8;
		break;
	case 16:
		c->red = c->pixel & 0xF800;
		c->green = (c->pixel >> 5) & 0x3F;
		c->green <<= 10;
		c->blue = c->pixel & 0x1F;
		c->blue <<= 11;
		break;
	case 15:
		c->red = c->pixel >> 11;
		c->green = (c->pixel >> 6) & 0x1F;
		c->green <<= 11;
		c->blue = c->pixel & 0x1F;
		c->blue <<= 11;
		break;
	case 8:
		c->red = (c->pixel >> 5) << 13;
		c->green = ((c->pixel >> 2) & 0x1C) << 13;
		c->blue = (c->pixel & 0x03)<< 13;
		break;
	default:
		JXQueryColor(display, rootColormap, c);
		break;
	}

}

/***************************************************************************
 ***************************************************************************/
void GetColor(XColor *c) {

	ColorNode *np;
	unsigned long hash;
	unsigned long red;
	unsigned long green;
	unsigned long blue;

	Assert(c);

	switch(rootDepth) {
	case 32:
	case 24:
		red = (c->red << 8) & 0xFF0000;
		green = c->green & 0x00FF00;
		blue = (c->blue >> 8) & 0x0000FF;
		c->pixel = red | green | blue;
		return;
	case 16: /* 5, 6, 5 */
		red = c->red & 0xF800;
		green = (c->green >> 5) & 0x07E0;
		blue = (c->blue >> 11) & 0x001F;
		c->pixel = red | green | blue;
		return;
	case 15: /* 5, 5, 5 */
		red = c->red & 0xF800;
		green = (c->green >> 5) & 0x07C0;
		blue = (c->blue >> 11) & 0x001F;
		return;
	case 8: /* 3, 3, 2 */
		red = (c->red >> 8) & 0xE0;
		green = (c->green >> 11) & 0x1C;
		blue = (c->blue >> 14) & 0x03;
		c->pixel = red | green | blue;
		return;
	default:
		break;
	}

	red = (c->red >> 0) & 0x0F00;
	green = (c->green >> 4) & 0x00F0;
	blue = (c->blue >> 8) & 0x000F;

	hash = red | green | blue;

	Assert(hash >= 0);
	Assert(hash < COLOR_HASH_SIZE);
	Assert(colorHash);

	np = colorHash[hash];
	while(np) {
		if(c->red == np->red && c->green == np->green && c->blue == np->blue) {
			c->pixel = np->pixel;
			return;
		}
		np = np->next;
	}

	np = Allocate(sizeof(ColorNode));

	np->red = c->red;
	np->green = c->green;
	np->blue = c->blue;

	if(!JXAllocColor(display, rootColormap, c)) {
		Warning("could not allocate color: %4X %4X %4X",
			c->red, c->green, c->blue);
		np->pixel = FindClosestColor(c);
	} else {
		np->pixel = c->pixel;
	}

	np->next = colorHash[hash];
	colorHash[hash] = np;

}

/****************************************************************************
 ****************************************************************************/
#ifdef USE_XFT
XftColor *GetXftColor(ColorType type) {

	unsigned long rgb;
	XRenderColor rcolor;

	if(!xftColors[type]) {
		rgb = rgbColors[type];
		xftColors[type] = Allocate(sizeof(XftColor));
		rcolor.alpha = 65535;
		rcolor.red = ((rgb >> 16) & 0xFF) * 257;
		rcolor.green = ((rgb >> 8) & 0xFF) * 257;
		rcolor.blue = (rgb & 0xFF) * 257;
		JXftColorAllocValue(display, rootVisual, rootColormap, &rcolor,
			xftColors[type]);
	}

	return xftColors[type];

}
#endif

