/****************************************************************************
 * Functions to load images.
 * Copyright (C) 2005 Joe Wingbermuehle
 ****************************************************************************/

#ifndef IMAGE_H
#define IMAGE_H

/****************************************************************************
 ****************************************************************************/
typedef struct ImageNode {

#ifdef USE_PNG
	png_uint_32 width;
	png_uint_32 height;
#else
	int width;
	int height;
#endif

	unsigned long *data;

} ImageNode;

ImageNode *LoadImage(const char *fileName);
ImageNode *LoadImageFromData(char **data);

void DestroyImage(ImageNode *image);

#endif

