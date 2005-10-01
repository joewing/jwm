/****************************************************************************
 ****************************************************************************/

#ifndef IMAGE_H
#define IMAGE_H

ImageNode *LoadImage(const char *fileName);
ImageNode *LoadImageFromData(char **data);

void ScaleImage(ImageNode *image, int size);

void DestroyImage(ImageNode *image);

#endif

