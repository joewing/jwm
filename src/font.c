/****************************************************************************
 * Functions to load fonts.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"
#include "font.h"
#include "main.h"
#include "error.h"
#include "color.h"
#include "misc.h"

static const char *DEFAULT_FONT = "-*-courier-*-r-*-*-14-*-*-*-*-*-*-*";

static char *fontNames[FONT_COUNT];

#ifdef USE_XFT
static XftFont *fonts[FONT_COUNT];
#else
static XFontStruct *fonts[FONT_COUNT];
#endif

/****************************************************************************
 ****************************************************************************/
void InitializeFonts() {

	int x;

	for(x = 0; x < FONT_COUNT; x++) {
		fonts[x] = NULL;
		fontNames[x] = NULL;
	}

}

/****************************************************************************
 ****************************************************************************/
void StartupFonts() {

	int x;

	/* Inherit unset fonts from the tray for tray items. */
	if(!fontNames[FONT_TASK]) {
		fontNames[FONT_TASK] = CopyString(fontNames[FONT_TRAY]);
	}
	if(!fontNames[FONT_TRAYBUTTON]) {
		fontNames[FONT_TRAYBUTTON] = CopyString(fontNames[FONT_TRAY]);
	}
	if(!fontNames[FONT_CLOCK]) {
		fontNames[FONT_CLOCK] = CopyString(fontNames[FONT_TRAY]);
	}

#ifdef USE_XFT

	for(x = 0; x < FONT_COUNT; x++) {
		if(fontNames[x]) {
			fonts[x] = JXftFontOpenName(display, rootScreen, fontNames[x]);
			if(!fonts[x]) {
				fonts[x] = JXftFontOpenXlfd(display, rootScreen, fontNames[x]);
			}
			if(!fonts[x]) {
				Warning("could not load font: %s", fontNames[x]);
			}
		}
		if(!fonts[x]) {
			fonts[x] = JXftFontOpenXlfd(display, rootScreen, DEFAULT_FONT);
		}
		if(!fonts[x]) {
			FatalError("could not load the default font: %s", DEFAULT_FONT);
		}
	}

#else

	for(x = 0; x < FONT_COUNT; x++) {
		if(fontNames[x]) {
			fonts[x] = JXLoadQueryFont(display, fontNames[x]);
			if(!fonts[x] && fontNames[x]) {
				Warning("could not load font: %s", fontNames[x]);
			}
		}
		if(!fonts[x]) {
			fonts[x] = JXLoadQueryFont(display, DEFAULT_FONT);
		}
		if(!fonts[x]) {
			FatalError("could not load the default font: %s", DEFAULT_FONT);
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

	Assert(str);
	Assert(fonts[type]);

	length = strlen(str);

	JXftTextExtentsUtf8(display, fonts[type], (const unsigned char*)str,
		length, &extents);

	return extents.width;

#else

	Assert(str);
	Assert(fonts[type]);

	return XTextWidth(fonts[type], str, strlen(str));

#endif
}

/****************************************************************************
 ****************************************************************************/
int GetStringHeight(FontType type) {
	Assert(fonts[type]);
	return fonts[type]->ascent + fonts[type]->descent;
}

/****************************************************************************
 ****************************************************************************/
void SetFont(FontType type, const char *value) {

	if(!value) {
		Warning("empty Font tag");
		return;
	}

	if(fontNames[type]) {
		Release(fontNames[type]);
	}

	fontNames[type] = CopyString(value);

}

/****************************************************************************
 ****************************************************************************/
void RenderString(Drawable d, GC g, FontType font, ColorType color,
	int x, int y, int width, Region region, const char *str) {

	XRectangle rect;
	int len, h, w;

	char *output;

#ifdef USE_FRIBIDI

	FriBidiChar *temp;
	FriBidiCharType type = FRIBIDI_TYPE_ON;
	int unicodeLength;

#endif

#ifdef USE_XFT
	XftDraw *xd;
#endif

	Assert(str);
	Assert(fonts[font]);

	len = strlen(str);

	h = Min(GetStringHeight(font), rootHeight) + 2;
	w = Min(GetStringWidth(font, str), width) + 2;

	rect.x = x;
	rect.y = y;
	rect.width = w;
	rect.height = h;

	/* Apply the bidi algorithm if requested. */

#ifdef USE_FRIBIDI

	temp = AllocateStack((len + 1) * sizeof(FriBidiChar));
	unicodeLength = fribidi_utf8_to_unicode((char*)str, len, temp);

	fribidi_log2vis(temp, unicodeLength, &type, temp, NULL, NULL, NULL);

	fribidi_unicode_to_utf8(temp, len, (char*)temp);
	output = (char*)temp;

#else

	output = (char*)str;

#endif

	/* Display the string. */

#ifdef USE_XFT

	xd = JXftDrawCreate(display, d, rootVisual, rootColormap);
	Assert(xd);

	if(region) {
		XftDrawSetClip(xd, region);
	} else {
		JXftDrawSetClipRectangles(xd, 0, 0, &rect, 1);
	}
	JXftDrawStringUtf8(xd, GetXftColor(color), fonts[font],
		x, y + fonts[font]->ascent, (const unsigned char*)output, len);
	JXftDrawDestroy(xd);

#else

	JXSetForeground(display, g, colors[color]);
	if(region) {
		XSetRegion(display, g, region);
	} else {
		JXSetClipRectangles(display, g, 0, 0, &rect, 1, Unsorted);
	}
	JXSetFont(display, g, fonts[font]->fid);
	JXDrawString(display, d, g, x, y + fonts[font]->ascent, output, len);
	if(width > 0 || region) {
		JXSetClipMask(display, g, None);
	}

#endif

	/* Free any memory used for UTF conversion. */

#ifdef USE_FRIBIDI

	ReleaseStack(output);

#endif

}

