/****************************************************************************
 * Functions to load images.
 * Copyright (C) 2005 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"
#include "image.h"
#include "main.h"
#include "error.h"

static ImageNode *LoadPNGImage(const char *fileName);
static ImageNode *LoadXPMImage(const char *fileName);
static ImageNode *CreateImageFromXImages(XImage *image, XImage *shape);

/****************************************************************************
 ****************************************************************************/
ImageNode *LoadImage(const char *fileName) {

	ImageNode *result;

	if(!fileName) {
		return NULL;
	}

	/* Attempt to load the file as a PNG image. */
	result = LoadPNGImage(fileName);
	if(result) {
		return result;
	}

	/* Attempt to load the file as an XPM image. */
	result = LoadXPMImage(fileName);
	if(result) {
		return result;
	}

	return NULL;

}

/****************************************************************************
 ****************************************************************************/
ImageNode *LoadImageFromData(char **data) {

	ImageNode *result = NULL;

#ifdef USE_XPM

	XImage *image;
	XImage *shape;
	int rc;

	Assert(data);

	rc = XpmCreateImageFromData(display, data, &image, &shape, NULL);
	if(rc == XpmSuccess) {
		result = CreateImageFromXImages(image, shape);
		JXDestroyImage(image);
		if(shape) {
			JXDestroyImage(shape);
		}
	}

#endif

	return result;

}

/****************************************************************************
 ****************************************************************************/
ImageNode *LoadPNGImage(const char *fileName) {

	ImageNode *result = NULL;

#ifdef USE_PNG

	FILE *fd = NULL;
	unsigned char header[8];
	unsigned long rowBytes;
	int bitDepth, colorType;
	png_structp pngData = NULL;
	png_infop pngInfo = NULL;
	png_infop pngEndInfo = NULL;
	unsigned char *data;
	unsigned char **rows;
	int x, y;
	unsigned long temp;

	Assert(fileName);

	fd = fopen(fileName, "rb");
	if(!fd) {
		return NULL;
	}

	x = fread(header, 1, sizeof(header), fd);
	if(x != sizeof(header) || png_sig_cmp(header, 0, sizeof(header))) {
		fclose(fd);
		return NULL;
	}

	pngData = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!pngData) {
		fclose(fd);
		Warning("could not create read struct for PNG image: %s", fileName);
		return NULL;
	}

	if(setjmp(png_jmpbuf(pngData))) {
		png_destroy_read_struct(&pngData, &pngInfo, &pngEndInfo);
		if(fd) {
			fclose(fd);
		}
		if(rows) {
			Release(rows);
		}
		if(result) {
			if(result->data) {
				Release(result->data);
			}
			Release(result);
		}
		Warning("error reading PNG image: %s", fileName);
	}

	pngInfo = png_create_info_struct(pngData);
	if(!pngInfo) {
		png_destroy_read_struct(&pngData, NULL, NULL);
		fclose(fd);
		Warning("could not create info struct for PNG image: %s", fileName);
		return NULL;
	}

	pngEndInfo = png_create_info_struct(pngData);
	if(!pngEndInfo) {
		png_destroy_read_struct(&pngData, &pngInfo, NULL);
		fclose(fd);
		Warning("could not create end info struct for PNG image: %s", fileName);
		return NULL;
	}

	png_init_io(pngData, fd);
	png_set_sig_bytes(pngData, sizeof(header));

	png_read_info(pngData, pngInfo);

	result = Allocate(sizeof(ImageNode));

	png_get_IHDR(pngData, pngInfo, &result->width, &result->height,
		&bitDepth, &colorType, NULL, NULL, NULL);

	png_set_expand(pngData);

	if(bitDepth == 16) {
		png_set_strip_16(pngData);
	} else if(bitDepth < 8) {
		png_set_packing(pngData);
	}

	png_set_swap_alpha(pngData);
	png_set_filler(pngData, 0xFF, PNG_FILLER_BEFORE);

	if(colorType == PNG_COLOR_TYPE_GRAY
		|| colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
		png_set_gray_to_rgb(pngData);
	}

	png_read_update_info(pngData, pngInfo);

	rowBytes = png_get_rowbytes(pngData, pngInfo);
	data = Allocate(rowBytes * result->height);

	rows = Allocate(result->height * sizeof(result->data));

	y = 0;
	for(x = 0; x < result->height; x++) {
		rows[x] = &data[y];
		y += result->width * 4;
	}

	png_read_image(pngData, rows);

	png_read_end(pngData, pngInfo);
	png_destroy_read_struct(&pngData, &pngInfo, &pngEndInfo);

	fclose(fd);

	/* Convert the row data to ARGB format. */
	/* Source is stored ARGB bytes. */
	/* Destination is stored in unsigned longs with A most significant. */
	result->data = Allocate(sizeof(unsigned long)
		* result->width * result->height);
	for(y = 0; y < result->height; y++) {
		for(x = 0; x < result->width; x++) {
			temp  = (unsigned long)rows[y][4 * x + 0] << 24;
			temp |= (unsigned long)rows[y][4 * x + 1] << 16;
			temp |= (unsigned long)rows[y][4 * x + 2] << 8;
			temp |= (unsigned long)rows[y][4 * x + 3] << 0;
			result->data[y * result->width + x] = temp;
		}
	}

	Release(rows);
	Release(data);

#endif

	return result;

}

/****************************************************************************
 ****************************************************************************/
ImageNode *LoadXPMImage(const char *fileName) {

	ImageNode *result = NULL;

#ifdef USE_XPM

	XImage *image;
	XImage *shape;
	int rc;

	Assert(fileName);

	rc = XpmReadFileToImage(display, (char*)fileName, &image, &shape, NULL);
	if(rc == XpmSuccess) {
		result = CreateImageFromXImages(image, shape);

		JXDestroyImage(image);
		if(shape) {
			JXDestroyImage(shape);
		}
	}

#endif

	return result;

}

/****************************************************************************
 ****************************************************************************/
ImageNode *CreateImageFromXImages(XImage *image, XImage *shape) {

	ImageNode *result;
	XColor color;
	unsigned char red, green, blue, alpha;
	int index;
	int x, y;

	result = Allocate(sizeof(ImageNode));
	result->data = Allocate(sizeof(unsigned long)
		* image->width * image->height);
	result->width = image->width;
	result->height = image->height;

	index = 0;
	for(y = 0; y < image->height; y++) {
		for(x = 0; x < image->width; x++) {

			color.pixel = XGetPixel(image, x, y);
			XQueryColor(display, rootColormap, &color);
			red = color.red >> 8;
			green = color.green >> 8;
			blue = color.blue >> 8;
			alpha = 0;
			if(!shape || XGetPixel(shape, x, y)) {
				alpha = 255;
			}

			result->data[index] = alpha << 24;
			result->data[index] |= red << 16;
			result->data[index] |= green << 8;
			result->data[index] |= blue;
			++index;

		}
	}

	return result;

}

/****************************************************************************
 ****************************************************************************/
void DestroyImage(ImageNode *image) {
	if(image) {
		Release(image->data);
		Release(image);
	}
}

