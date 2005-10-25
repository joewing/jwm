/****************************************************************************
 * Functions to load images.
 * Copyright (C) 2005 Joe Wingbermuehle
 ****************************************************************************/

#ifndef IMAGE_H
#define IMAGE_H

ImageNode *LoadImage(const char *fileName);
ImageNode *LoadImageFromData(char **data);

void DestroyImage(ImageNode *image);

#endif

