/****************************************************************************
 ****************************************************************************/

#include "jwm.h"

/****************************************************************************
 ****************************************************************************/
int PutRenderIcon(const IconNode *icon, Drawable d, int x, int y) {

#ifdef USE_XRENDER

	Picture dest;
	Picture source;
	XRenderPictFormat *fp;

	source = icon->imagePicture;
	if(source != None) {

		fp = JXRenderFindVisualFormat(display, rootVisual);
		dest = JXRenderCreatePicture(display, d, fp, 0, NULL);

		JXRenderComposite(display, PictOpOver, source, None, dest,
			0, 0, 0, 0, x, y, icon->width, icon->height);

		JXRenderFreePicture(display, dest);

	}

	return 1;

#else

	return 0;

#endif

}

/****************************************************************************
 ****************************************************************************/
IconNode *CreateRenderIconFromImage(ImageNode *image) {

	IconNode *result = NULL;

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

	result = CreateIcon();
	result->width = image->width;
	result->height = image->height;

	result->mask = JXCreatePixmap(display, rootWindow,
		image->width, image->height, 8);
	maskGC = JXCreateGC(display, result->mask, 0, NULL);
	result->image = JXCreatePixmap(display, rootWindow,
		image->width, image->height, rootDepth);
	imageGC = JXCreateGC(display, result->image, 0, NULL);

	index = 0;
	for(y = 0; y < image->height; y++) {
		for(x = 0; x < image->width; x++) {

			alpha = image->data[index++];
			color.red = image->data[index++] * 257;
			color.green = image->data[index++] * 257;
			color.blue = image->data[index++] * 257;

			GetColor(&color);
			JXSetForeground(display, imageGC, color.pixel);
			XDrawPoint(display, result->image, imageGC, x, y);

			color.red = alpha * 257;
			color.green = alpha * 257;
			color.blue = alpha * 257;

			GetColor(&color);
			JXSetForeground(display, maskGC, color.pixel);
			XDrawPoint(display, result->mask, maskGC, x, y);

		}
	}

	JXFreeGC(display, maskGC);
	JXFreeGC(display, imageGC);

	picFormat.type = PictTypeDirect;
	picFormat.depth = 8;
	picFormat.direct.alphaMask = 0xFF;
	fp = JXRenderFindFormat(display,
		PictFormatType | PictFormatDepth | PictFormatAlphaMask,
		&picFormat, 0);
	result->maskPicture = JXRenderCreatePicture(display, result->mask,
		fp, 0, NULL);

	picAttributes.alpha_map = result->maskPicture;
	fp = JXRenderFindVisualFormat(display, rootVisual);
	result->imagePicture = JXRenderCreatePicture(display, result->image,
		fp, CPAlphaMap, &picAttributes);

#endif

	return result;

}

