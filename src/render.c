/****************************************************************************
 * Functions to render icons using the XRender extension.
 * Copyright (C) 2005 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"

#ifdef USE_XRENDER
static int haveRender = 0;
#endif

/****************************************************************************
 ****************************************************************************/
void QueryRenderExtension()
{

#ifdef USE_XRENDER
	int event, error;
	Bool rc;

	rc = JXRenderQueryExtension(display, &event, &error);
	if(rc == True) {
		haveRender = 1;
		Debug("render extension enabled");
	} else {
		haveRender = 0;
		Debug("render extension disabled");
	}
#endif

}

/****************************************************************************
 ****************************************************************************/
int PutScaledRenderIcon(IconNode *icon, ScaledIconNode *node, Drawable d,
	int x, int y)
{

#ifdef USE_XRENDER

	Picture dest;
	Picture source;
	XRenderPictFormat *fp;
	int width, height;

	Assert(icon);

	if(!haveRender) {
		return 0;
	}

	source = node->imagePicture;
	if(source != None) {

		fp = JXRenderFindVisualFormat(display, rootVisual);
		Assert(fp);

		dest = JXRenderCreatePicture(display, d, fp, 0, NULL);

		if(node->size == 0) {
			width = icon->image->width;
			height = icon->image->height;
		} else {
			width = node->size;
			height = node->size;
		}

		JXRenderComposite(display, PictOpOver, source, None, dest,
			0, 0, 0, 0, x, y, width, height);

		JXRenderFreePicture(display, dest);

	}

	return 1;

#else

	return 0;

#endif

}

/****************************************************************************
 ****************************************************************************/
ScaledIconNode *CreateScaledRenderIcon(IconNode *icon, int size)
{

	ScaledIconNode *result = NULL;

#ifdef USE_XRENDER

	XRenderPictureAttributes picAttributes;
	XRenderPictFormat picFormat;
	XRenderPictFormat *fp;
	XColor color;
	GC maskGC;
	GC imageGC;
	unsigned char alpha;
	int index;
	int x, y;
	int width, height;
	double scalex, scaley;
	double srcx, srcy;
	CARD32 *data;

	Assert(icon);

	if(!haveRender) {
		return NULL;
	}

	result = Allocate(sizeof(ScaledIconNode));
	result->next = icon->nodes;
	icon->nodes = result;
	result->size = size;

	if(size == 0) {
		width = icon->image->width;
		height = icon->image->height;
	} else {
		width = size;
		height = size;
	}

	scalex = (double)icon->image->width / width;
	scaley = (double)icon->image->height / height;

	result->mask = JXCreatePixmap(display, rootWindow, width, height, 8);
	maskGC = JXCreateGC(display, result->mask, 0, NULL);
	result->image = JXCreatePixmap(display, rootWindow,
		width, height, rootDepth);
	imageGC = JXCreateGC(display, result->image, 0, NULL);

	data = (CARD32*)icon->image->data;

	srcy = 0.0;
	for(y = 0; y < height; y++) {
		srcx = 0.0;
		for(x = 0; x < width; x++) {

			index = (int)srcy * icon->image->width + (int)srcx;
			alpha = (data[index] >> 24) & 0xFF;
			color.red = ((data[index] >> 16) & 0xFF) * 257;
			color.green = ((data[index] >> 8) & 0xFF) * 257;
			color.blue = (data[index] & 0xFF) * 257;

			GetColor(&color);
			JXSetForeground(display, imageGC, color.pixel);
			JXDrawPoint(display, result->image, imageGC, x, y);

			color.red = alpha * 257;
			color.green = alpha * 257;
			color.blue = alpha * 257;

			GetColor(&color);
			JXSetForeground(display, maskGC, color.pixel);
			JXDrawPoint(display, result->mask, maskGC, x, y);

			srcx += scalex;

		}
		srcy += scaley;
	}

	JXFreeGC(display, maskGC);
	JXFreeGC(display, imageGC);

	picFormat.type = PictTypeDirect;
	picFormat.depth = 8;
	picFormat.direct.alphaMask = 0xFF;
	fp = JXRenderFindFormat(display,
		PictFormatType | PictFormatDepth | PictFormatAlphaMask,
		&picFormat, 0);
	Assert(fp);
	result->maskPicture = JXRenderCreatePicture(display, result->mask,
		fp, 0, NULL);

	picAttributes.alpha_map = result->maskPicture;
	fp = JXRenderFindVisualFormat(display, rootVisual);
	Assert(fp);
	result->imagePicture = JXRenderCreatePicture(display, result->image,
		fp, CPAlphaMap, &picAttributes);

#endif

	return result;

}

