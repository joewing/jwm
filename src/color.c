/****************************************************************************
 * Functions to handle loading colors.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"

/* Note: COLOR_HASH_SIZE = 2 ** (3 * COLOR_HASH_BITS) */
static const int COLOR_HASH_BITS = 3;
static const int COLOR_HASH_SIZE = 512;

typedef struct ColorNode {
	unsigned short red, green, blue;
	unsigned long pixel;
	struct ColorNode *prev;
	struct ColorNode *next;
} ColorNode;

typedef struct {
	ColorType type;
	const char *value;
} DefaultColorNode;

static ColorNode **colorHash = NULL;

static const float COLOR_DELTA = 0.45;

unsigned long colors[COLOR_COUNT];
unsigned int rgbColors[COLOR_COUNT];
unsigned long white;
unsigned long black;

static DefaultColorNode DEFAULT_COLORS[] = {
	{ COLOR_BORDER_BG,          "gray"    },
	{ COLOR_BORDER_FG,          "black"   },
	{ COLOR_BORDER_ACTIVE_BG,   "red"     },
	{ COLOR_BORDER_ACTIVE_FG,   "white"   },
	{ COLOR_TRAY_BG,            "gray"    },
	{ COLOR_TRAY_FG,            "black"   },
	{ COLOR_TRAY_ACTIVE_BG,     "red"     },
	{ COLOR_TRAY_ACTIVE_FG,     "white"   },
	{ COLOR_PAGER_BG,           "black"   },
	{ COLOR_PAGER_FG,           "gray"    },
	{ COLOR_PAGER_ACTIVE_BG,    "red"     },
	{ COLOR_PAGER_ACTIVE_FG,    "red"     },
	{ COLOR_PAGER_OUTLINE,      "black"   },
	{ COLOR_LOAD_BG,            "gray"    },
	{ COLOR_LOAD_FG,            "red"     },
	{ COLOR_LOAD_OUTLINE,       "black"   },
	{ COLOR_MENU_BG,            "gray"    },
	{ COLOR_MENU_FG,            "black"   },
	{ COLOR_MENU_ACTIVE_BG,     "red"     },
	{ COLOR_MENU_ACTIVE_FG,     "white"   },
	{ COLOR_POPUP_BG,           "yellow"  },
	{ COLOR_POPUP_FG,           "black"   },
	{ COLOR_POPUP_OUTLINE,      "black"   },
	{ COLOR_COUNT,              NULL      }
};

static int ParseColor(ColorType type, const char *value);
static void SetDefaultColor(ColorType type); 

static unsigned int ReadHex(const char *hex);

static unsigned int GetRGBFromXColor(const XColor *c);
static XColor GetXColorFromRGB(unsigned int rgb);

static int GetColorByName(const char *str, XColor *c);
static unsigned long FindClosestColor(const XColor *c);
static void InitializeNames();

static void LightenColor(ColorType oldColor, ColorType newColor);
static void DarkenColor(ColorType oldColor, ColorType newColor);

static void CreateColorRamp(unsigned int a, unsigned int b,
	unsigned long *ramp);

static char **names = NULL;
unsigned long *ramps[RAMP_COUNT];

/****************************************************************************
 ****************************************************************************/
unsigned int GetRGBFromXColor(const XColor *c) {
	float red, green, blue;
	unsigned int rgb;

	red = (float)c->red / 65535.0;
	green = (float)c->green / 65535.0;
	blue = (float)c->blue / 65535.0;

	rgb = (unsigned int)(red * 255.0) << 16;
	rgb |= (unsigned int)(green * 255.0) << 8;
	rgb |= (unsigned int)(blue * 255.0);

	return rgb;
}

/****************************************************************************
 ****************************************************************************/
XColor GetXColorFromRGB(unsigned int rgb) {
	XColor ret;

	ret.flags = DoRed | DoGreen | DoBlue;
	ret.red = ((rgb >> 16) & 0xFF) * 257;
	ret.green = ((rgb >> 8) & 0xFF) * 257;
	ret.blue = (rgb & 0xFF) * 257;

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

	for(x = 0; x < RAMP_COUNT; x++) {
		ramps[x] = Allocate(8 * sizeof(unsigned long));
	}

}

/****************************************************************************
 ****************************************************************************/
void StartupColors() {
	int x;

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

	LightenColor(COLOR_TRAY_ACTIVE_BG, COLOR_TRAY_ACTIVE_UP);
	DarkenColor(COLOR_TRAY_ACTIVE_BG, COLOR_TRAY_ACTIVE_DOWN);

	LightenColor(COLOR_MENU_BG, COLOR_MENU_UP);
	DarkenColor(COLOR_MENU_BG, COLOR_MENU_DOWN);

	LightenColor(COLOR_MENU_ACTIVE_BG, COLOR_MENU_ACTIVE_UP);
	DarkenColor(COLOR_MENU_ACTIVE_BG, COLOR_MENU_ACTIVE_DOWN);

	CreateColorRamp(rgbColors[COLOR_BORDER_BG], rgbColors[COLOR_BORDER_FG],
		ramps[RAMP_BORDER]);

	CreateColorRamp(rgbColors[COLOR_BORDER_ACTIVE_BG],
		rgbColors[COLOR_BORDER_ACTIVE_FG], ramps[RAMP_BORDER_ACTIVE]);

	CreateColorRamp(rgbColors[COLOR_TRAY_BG], rgbColors[COLOR_TRAY_FG],
		ramps[RAMP_TRAY]);

	CreateColorRamp(rgbColors[COLOR_TRAY_ACTIVE_BG],
		rgbColors[COLOR_TRAY_ACTIVE_FG], ramps[RAMP_TRAY_ACTIVE]);

	CreateColorRamp(rgbColors[COLOR_MENU_BG], rgbColors[COLOR_MENU_FG],
		ramps[RAMP_MENU]);

	CreateColorRamp(rgbColors[COLOR_MENU_ACTIVE_BG],
		rgbColors[COLOR_MENU_ACTIVE_FG], ramps[RAMP_MENU_ACTIVE]);

	CreateColorRamp(rgbColors[COLOR_POPUP_BG],
		rgbColors[COLOR_POPUP_FG], ramps[RAMP_POPUP]);

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

	for(x = 0; x < RAMP_COUNT; x++) {
		if(ramps[x]) {
			Release(ramps[x]);
		}
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
	unsigned int rgb;

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
unsigned int ReadHex(const char *hex) {
	unsigned int value = 0;
	unsigned int x;

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
void CreateColorRamp(unsigned int a, unsigned int b, unsigned long *ramp) {

	XColor color;
	unsigned int x;
	float value, inv;

	float reda = (float)((a >> 16) & 0xFF) / 255.0;
	float greena = (float)((a >> 8) & 0xFF) / 255.0;
	float bluea = (float)(a & 0xFF) / 255.0;

	float redb = (float)((b >> 16) & 0xFF) / 255.0;
	float greenb = (float)((b >> 8) & 0xFF) / 255.0;
	float blueb = (float)(b & 0xFF) / 255.0;

	Assert(ramp);

	for(x = 0; x < 8; x++) {

		value = (float)x / 7.0;
		inv = 1.0 - value;

		color.red = (unsigned int)((reda * inv + redb * value)
			* 65535.0 + 0.5);
		color.green = (unsigned int)((greena * inv + greenb * value)
			* 65535.0 + 0.5);
		color.blue = (unsigned int)((bluea * inv + blueb * value)
			* 65535.0 + 0.5);

		GetColor(&color);
		ramp[x] = color.pixel;

	}

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
	long closestDistance;
	long distance;
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
void GetColor(XColor *c) {

	ColorNode *np;
	unsigned int hash;

	Assert(c);

	hash = c->red >> (8 + 5 - COLOR_HASH_BITS * 2);
	hash |= c->green >> (8 + 5 - COLOR_HASH_BITS);
	hash |= c->blue >> (8 + 5);

	Assert(hash >= 0);
	Assert(hash < COLOR_HASH_SIZE);

	Assert(colorHash);

	np = colorHash[hash];
	while(np) {
		if(c->red == np->red && c->green == np->green && c->blue == np->blue) {
			if(np->prev) {
				np->prev->next = np->next;
				if(np->next) {
					np->next->prev = np->prev;
				}
				np->prev = NULL;
				colorHash[hash]->prev = np;
				np->next = colorHash[hash];
				colorHash[hash] = np;
			}
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

	if(colorHash[hash]) {
		colorHash[hash]->prev = np;
	}
	np->prev = NULL;
	np->next = colorHash[hash];
	colorHash[hash] = np;

}

