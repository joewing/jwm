/****************************************************************************
 * Functions to load fonts.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"

static const char *DEFAULT_FONT = "5x7";

static char *fontNames[FONT_COUNT] = { NULL };

#ifdef USE_XFT
static XftFont *fonts[FONT_COUNT] = { NULL };
#else
static XFontStruct *fonts[FONT_COUNT] = { NULL };
#endif

/****************************************************************************
 ****************************************************************************/
void InitializeFonts() {
}

/****************************************************************************
 ****************************************************************************/
void StartupFonts() {

	int x;

#ifdef USE_XFT

	for(x = 0; x < FONT_COUNT; x++) {
		fonts[x] = JXftFontOpenXlfd(display, rootScreen, fontNames[x]);
		if(!fonts[x]) {
			fonts[x] = JXftFontOpenXlfd(display, rootScreen, DEFAULT_FONT);
			if(!fonts[x]) {
				FatalError("Could not load the default font: %s", DEFAULT_FONT);
			}
		}
	}

#else

	for(x = 0; x < FONT_COUNT; x++) {
		fonts[x] = JXLoadQueryFont(display, fontNames[x]);
		if(!fonts[x]) {
			fonts[x] = JXLoadQueryFont(display, DEFAULT_FONT);
			if(!fonts[x]) {
				FatalError("Could not load the default font: %s", DEFAULT_FONT);
			}
		}
	}

#endif

}

/****************************************************************************
 ****************************************************************************/
void ShutdownFonts() {
	int x;

	for(x = 0; x < FONT_COUNT; x++) {
		if(fonts[x]) {
#ifdef USE_XFT
			JXftFontClose(display, fonts[x]);
#else
			JXFreeFont(display, fonts[x]);
#endif
			fonts[x] = NULL;
		}
		if(fontNames[x]) {
			Release(fontNames[x]);
			fontNames[x] = NULL;
		}
	}

}

/****************************************************************************
 ****************************************************************************/
void DestroyFonts() {

	int x;

	for(x = 0; x < FONT_COUNT; x++) {
		if(fontNames[x]) {
			Release(fontNames[x]);
			fontNames[x] = NULL;
		}
	}

}

/****************************************************************************
 ****************************************************************************/
int GetStringWidth(FontType type, const char *str) {
#ifdef USE_XFT

	XGlyphInfo extents;
	unsigned int length;

	length = strlen(str);

	JXftTextExtents8(display, fonts[type], (const unsigned char*)str,
		length, &extents);

	return extents.width;

#else

	return XTextWidth(fonts[type], str, strlen(str));

#endif
}

/****************************************************************************
 ****************************************************************************/
int GetStringHeight(FontType type) {
	return fonts[type]->ascent + fonts[type]->descent;
}

/****************************************************************************
 ****************************************************************************/
void SetFont(FontType type, const char *value) {

	Assert(value);

	if(fontNames[type]) {
		Release(fontNames[type]);
	}

	fontNames[type] = Allocate(strlen(value) + 1);
	strcpy(fontNames[type], value);

}

/****************************************************************************
 ****************************************************************************/
void RenderString(Drawable d, GC g, FontType font, ColorType color,
	int x, int y, int width, const char *str) {

	XRectangle rect;
	int len, h, w;

#ifdef USE_XFT
	XftDraw *xd;
#endif

	Assert(str);

	len = strlen(str);

	h = Min(GetStringHeight(font), rootHeight) + 2;
	w = Min(GetStringWidth(font, str), width) + 2;

	rect.x = x;
	rect.y = y;
	rect.width = w;
	rect.height = h;

#ifdef USE_XFT

	xd = JXftDrawCreate(display, d, rootVisual, rootColormap);
	JXftDrawSetClipRectangles(xd, 0, 0, &rect, 1);
	JXftDrawString8(xd, GetXftColor(color), fonts[font],
		x, y + fonts[font]->ascent, (const unsigned char*)str, len);
	JXftDrawDestroy(xd);

#else

	JXSetForeground(display, g, colors[color]);
	JXSetClipRectangles(display, g, 0, 0, &rect, 1, Unsorted);
	JXSetFont(display, g, fonts[font]->fid);
	JXDrawString(display, d, g, x, y + fonts[font]->ascent, str, len);
	JXSetClipMask(display, g, None);

#endif

}


