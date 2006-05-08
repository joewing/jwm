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

/****************************************************************************
 ****************************************************************************/
typedef struct ThemeImageNode {

	int width;
	int height;

	Pixmap image;
	Pixmap shape;

} ThemeImageNode;

ImageNode *LoadImage(const char *fileName);

ImageNode *LoadImageFromData(char **data);

void DestroyImage(ImageNode *image);

ThemeImageNode *LoadThemeImage(const char *name);

void DestroyThemeImage(ThemeImageNode *image);

#endif

