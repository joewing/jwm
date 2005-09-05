/****************************************************************************
 * Functions to load fonts.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"

static const char *DEFAULT_FONT = "5x7";

static char *fontNames[FONT_COUNT] = { NULL };
static int antialias[FONT_COUNT] = { 0 };
XFontStruct *fonts[FONT_COUNT] = { NULL };

/* Data for antialiasing */
static char *inputData = NULL;
static char *outputData = NULL;
static int bitDepth, byteDepth;
static XImage *inputImage = NULL;
static XImage *outputImage = NULL;
static Pixmap buffer = None;
static GC bufferGC;

static void Antialias8(int width, int height, RampType ramp);
static void Antialias16(int width, int height, RampType ramp);
static void Antialias32(int width, int height, RampType ramp);

/****************************************************************************
 ****************************************************************************/
void InitializeFonts() {
}

/****************************************************************************
 ****************************************************************************/
void StartupFonts() {

	int x;

	if(rootDepth <= 8) {
		bitDepth = 8;
		byteDepth = 1;
	} else if(rootDepth <= 16) {
		bitDepth = 16;
		byteDepth = 2;
	} else {
		bitDepth = 32;
		byteDepth = 4;
	}

	for(x = 0; x < FONT_COUNT; x++) {
		fonts[x] = JXLoadQueryFont(display, fontNames[x]);
		if(!fonts[x]) {
			fonts[x] = JXLoadQueryFont(display, DEFAULT_FONT);
			if(!fonts[x]) {
				FatalError("Could not load the default font: %s", DEFAULT_FONT);
			}
		}
	}

}

/****************************************************************************
 ****************************************************************************/
void ShutdownFonts() {
	int x;

	for(x = 0; x < FONT_COUNT; x++) {
		if(fonts[x]) {
			JXFreeFont(display, fonts[x]);
			fonts[x] = NULL;
		}
		if(fontNames[x]) {
			Release(fontNames[x]);
			fontNames[x] = NULL;
		}
	}

	if(inputData) {
		inputImage->data = NULL;
		JXDestroyImage(inputImage);

		outputImage->data = NULL;
		JXDestroyImage(outputImage);

		JXFreeGC(display, bufferGC);
		JXFreePixmap(display, buffer);
		buffer = None;
	}

}

/****************************************************************************
 ****************************************************************************/
void DestroyFonts() {

	int x;

	if(inputData) {
		Release(inputData);
		inputData = NULL;
		Release(outputData);
		outputData = NULL;
	}

	for(x = 0; x < FONT_COUNT; x++) {
		if(fontNames[x]) {
			Release(fontNames[x]);
			fontNames[x] = NULL;
		}
	}

}

/****************************************************************************
 ****************************************************************************/
void SetFont(FontType type, const char *value, int aa) {

	Assert(value);

	if(fontNames[type]) {
		Release(fontNames[type]);
	}

	antialias[type] = aa;
	fontNames[type] = Allocate(strlen(value) + 1);
	strcpy(fontNames[type], value);

}

/****************************************************************************
 ****************************************************************************/
void Antialias8(int width, int height, RampType ramp) {

	int x, y;
	int offset;
	int value;
	int line = inputImage->width;

	char *in = inputData;
	char *out = outputData;

	for(y = 1; y < height - 1; y++) {
		for(x = 1; x < width - 1; x++) {

			offset = y * line + x;

			if(   in[offset + 1] != in[offset - 1]
				&& in[offset + line + 1] != in[offset - line - 1]
				&& in[offset + line - 1] != in[offset - line + 1]
				&& in[offset + line] != in[offset - line]) {

				if(in[offset] == black) {
					value = 2;
				} else {
					value = 0;
				}
				if(in[offset + 1] == black) {
					++value;
				}
				if(in[offset - 1] == black) {
					++value;
				}
				if(in[offset + line] == black) {
					++value;
				}
				if(in[offset - line] == black) {
					++value;
				}

				out[offset] = ramps[ramp][value];

			} else {

				if(in[offset] == black) {
					out[offset] = ramps[ramp][7];
				} else {
					out[offset] = ramps[ramp][0];
				}

			}

		}
	}

}

/****************************************************************************
 ****************************************************************************/
void Antialias16(int width, int height, RampType ramp) {

	int x, y;
	int offset;
	int value;
	int line = inputImage->width;

	unsigned short *in = (unsigned short*)inputData;
	unsigned short *out = (unsigned short*)outputData;

	for(y = 1; y < height - 1; y++) {
		for(x = 1; x < width - 1; x++) {

			offset = y * line + x;

			if(   in[offset + 1] != in[offset - 1]
				&& in[offset + line + 1] != in[offset - line - 1]
				&& in[offset + line - 1] != in[offset - line + 1]
				&& in[offset + line] != in[offset - line]) {

				if(in[offset] == black) {
					value = 2;
				} else {
					value = 0;
				}
				if(in[offset + 1] == black) {
					++value;
				}
				if(in[offset - 1] == black) {
					++value;
				}
				if(in[offset + line] == black) {
					++value;
				}
				if(in[offset - line] == black) {
					++value;
				}

				out[offset] = ramps[ramp][value];

			} else {

				if(in[offset] == black) {
					out[offset] = ramps[ramp][7];
				} else {
					out[offset] = ramps[ramp][0];
				}

			}

		}
	}

}

/****************************************************************************
 ****************************************************************************/
void Antialias32(int width, int height, RampType ramp) {

	int x, y;
	int offset;
	int value;
	int line = inputImage->width;

	unsigned long *in = (unsigned long*)inputData;
	unsigned long *out = (unsigned long*)outputData;

	for(y = 1; y < height - 1; y++) {
		for(x = 1; x < width - 1; x++) {

			offset = y * line + x;

			if(   in[offset + 1] != in[offset - 1]
				&& in[offset + line + 1] != in[offset - line - 1]
				&& in[offset + line - 1] != in[offset - line + 1]
				&& in[offset + line] != in[offset - line]) {

				if(in[offset] == black) {
					value = 2;
				} else {
					value = 0;
				}
				if(in[offset + 1] == black) {
					++value;
				}
				if(in[offset - 1] == black) {
					++value;
				}
				if(in[offset + line] == black) {
					++value;
				}
				if(in[offset - line] == black) {
					++value;
				}

				out[offset] = ramps[ramp][value];

			} else {

				if(in[offset] == black) {
					out[offset] = ramps[ramp][7];
				} else {
					out[offset] = ramps[ramp][0];
				}

			}

		}
	}

}

/****************************************************************************
 ****************************************************************************/
void RenderString(Drawable d, GC g, FontType font, RampType ramp,
	int x, int y, int width, const char *str) {

	XRectangle rect;
	int len, h, w;

	Assert(str);

	len = strlen(str);

	h = Min(fonts[font]->ascent + fonts[font]->descent, rootHeight) + 2;
	w = Min(JXTextWidth(fonts[font], str, len), width) + 2;

	if(!antialias[font]) {
		switch(ramp) {
		case RAMP_BORDER:
			JXSetForeground(display, g, colors[COLOR_BORDER_FG]);
			break;
		case RAMP_BORDER_ACTIVE:
			JXSetForeground(display, g, colors[COLOR_BORDER_ACTIVE_FG]);
			break;
		case RAMP_TRAY:
			JXSetForeground(display, g, colors[COLOR_TRAY_FG]);
			break;
		case RAMP_TRAY_ACTIVE:
			JXSetForeground(display, g, colors[COLOR_TRAY_ACTIVE_FG]);
			break;
		case RAMP_MENU:
			JXSetForeground(display, g, colors[COLOR_MENU_FG]);
			break;
		case RAMP_MENU_ACTIVE:
			JXSetForeground(display, g, colors[COLOR_MENU_ACTIVE_FG]);
			break;
		default:
			break;
		}

		rect.x = x;
		rect.y = y;
		rect.width = w;
		rect.height = h;

		JXSetFont(display, g, fonts[font]->fid);
		JXSetClipRectangles(display, g, 0, 0, &rect, 1, Unsorted);
		JXDrawString(display, d, g, x, y + fonts[font]->ascent, str, len);
		JXSetClipMask(display, g, None);

		return;
	}

	if(!inputData || h > inputImage->height || w > inputImage->width) {

		if(inputData) {

			Release(inputData);
			inputImage->data = NULL;
			JXDestroyImage(inputImage);

			Release(outputData);
			outputImage->data = NULL;
			JXDestroyImage(outputImage);

			JXFreeGC(display, bufferGC);
			JXFreePixmap(display, buffer);

		}

		buffer = JXCreatePixmap(display, rootWindow, w, h, rootDepth);
		bufferGC = JXCreateGC(display, buffer, 0, NULL);

		inputData = Allocate(w * h * byteDepth);
		inputImage = JXCreateImage(display, rootVisual, rootDepth,
			ZPixmap, 0, inputData, w, h, bitDepth, 0);

		outputData = Allocate(w * h * byteDepth);
		outputImage = JXCreateImage(display, rootVisual, rootDepth,
			ZPixmap, 0, outputData, w, h, bitDepth, 0);

	}

	JXSetForeground(display, bufferGC, white);
	JXFillRectangle(display, buffer, bufferGC, 0, 0, w, h);

	JXSetForeground(display, bufferGC, black);
	JXSetFont(display, bufferGC, fonts[font]->fid);
	JXDrawString(display, buffer, bufferGC, 1,
		1 + fonts[font]->ascent, str, len);

	JXGetSubImage(display, buffer, 0, 0, w, h, AllPlanes, ZPixmap,
		inputImage, 0, 0);

	switch(byteDepth) {
	case 1:
		Antialias8(w, h, ramp);
		break;
	case 2:
		Antialias16(w, h, ramp);
		break;
	default:
		Antialias32(w, h, ramp);
		break;
	}

	JXPutImage(display, d, g, outputImage, 1, 1, x, y, w - 2, h - 2);

}


