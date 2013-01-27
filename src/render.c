/**
 * @file render.c
 * @author Joe Wingbermuehle
 * @date 2005-2006
 *
 * @brief Functions to render icons using the XRender extension.
 *
 */

#include "jwm.h"
#include "render.h"
#include "icon.h"
#include "image.h"
#include "main.h"
#include "color.h"
#include "error.h"

/** Draw a scaled icon. */
int PutScaledRenderIcon(IconNode *icon, ScaledIconNode *node, Drawable d,
   int x, int y)
{

#ifdef USE_XRENDER

   Picture dest;
   Picture source;
   Picture alpha;
   XRenderPictFormat *fp;
   XRenderPictureAttributes pa;
   XTransform xf;
   int width, height;
   int xscale, yscale;

   Assert(icon);

   if(!haveRender) {
      return 0;
   }

   source = node->imagePicture;
   alpha = node->alphaPicture;
   if(source != None) {

      fp = JXRenderFindVisualFormat(display, rootVisual);
      Assert(fp);

      pa.subwindow_mode = IncludeInferiors;
      dest = JXRenderCreatePicture(display, d, fp, CPSubwindowMode, &pa);

      if(node->width == 0) {
         width = icon->image->width;
         xscale = 65536;
      } else {
         width = node->width;
         xscale = (icon->image->width << 16) / width;
      }
      if(node->height == 0) {
         height = icon->image->height;
         yscale = 65536;
      } else {
         height = node->height;
         yscale = (icon->image->height << 16) / height;
      }

      memset(&xf, 0, sizeof(xf));
      xf.matrix[0][0] = xscale;
      xf.matrix[1][1] = yscale;
      xf.matrix[2][2] = 65536;
      XRenderSetPictureTransform(display, source, &xf);
      XRenderSetPictureFilter(display, source, FilterBest, NULL, 0);
      XRenderSetPictureTransform(display, alpha, &xf);
      XRenderSetPictureFilter(display, alpha, FilterBest, NULL, 0);

      JXRenderComposite(display, PictOpOver, source, alpha, dest,
                        0, 0, 0, 0, x, y, width, height);

      JXRenderFreePicture(display, dest);

   }

   return 1;

#else

   return 0;

#endif

}

/** Create a scaled icon. */
ScaledIconNode *CreateScaledRenderIcon(IconNode *icon,
                                       int width, int height) {

   ScaledIconNode *result = NULL;

#ifdef USE_XRENDER

   XRenderPictFormat *fp;
   XColor color;
   GC maskGC;
   XImage *destImage;
   XImage *destMask;
   unsigned long alpha;
   int index, yindex;
   int x, y;
   int imageLine;
   int maskLine;

   Assert(icon);

   if(!haveRender) {
      return NULL;
   }

   result = Allocate(sizeof(ScaledIconNode));
   result->next = icon->nodes;
   icon->nodes = result;

   result->width = width;
   result->height = height;
   width = icon->image->width;
   height = icon->image->height;

   result->mask = JXCreatePixmap(display, rootWindow, width, height, 8);
   maskGC = JXCreateGC(display, result->mask, 0, NULL);
   result->image = JXCreatePixmap(display, rootWindow, width, height,
                                  rootDepth);

   destImage = JXCreateImage(display, rootVisual, rootDepth, ZPixmap, 0,
                             NULL, width, height, 8, 0);
   destImage->data = Allocate(sizeof(unsigned long) * width * height);

   destMask = JXCreateImage(display, rootVisual, 8, ZPixmap, 0,
                            NULL, width, height, 8, 0);
   destMask->data = Allocate(width * height);

   imageLine = 0;
   maskLine = 0;
   for(y = 0; y < height; y++) {
      yindex = y * icon->image->width;
      for(x = 0; x < width; x++) {

         index = 4 * (yindex + x);
         alpha = icon->image->data[index];
         color.red = icon->image->data[index + 1];
         color.red |= color.red << 8;
         color.green = icon->image->data[index + 2];
         color.green |= color.green << 8;
         color.blue = icon->image->data[index + 3];
         color.blue |= color.blue << 8;

         color.red = (color.red * alpha) >> 8;
         color.green = (color.green * alpha) >> 8;
         color.blue = (color.blue * alpha) >> 8;

         GetColor(&color);
         XPutPixel(destImage, x, y, color.pixel);
         destMask->data[maskLine + x] = alpha;

      }
      imageLine += destImage->bytes_per_line;
      maskLine += destMask->bytes_per_line;
   }

   /* Render the image data to the image pixmap. */
   JXPutImage(display, result->image, rootGC, destImage, 0, 0, 0, 0,
              width, height);
   Release(destImage->data);
   destImage->data = NULL;
   JXDestroyImage(destImage);

   /* Render the alpha data to the mask pixmap. */
   JXPutImage(display, result->mask, maskGC, destMask, 0, 0, 0, 0,
              width, height);
   Release(destMask->data);
   destMask->data = NULL;
   JXDestroyImage(destMask);
   JXFreeGC(display, maskGC);

   /* Create the alpha picture. */
   fp = JXRenderFindStandardFormat(display, PictStandardA8);
   Assert(fp);
   result->alphaPicture = JXRenderCreatePicture(display, result->mask, fp,
                                                0, NULL);
   
   /* Create the render picture. */
   fp = JXRenderFindVisualFormat(display, rootVisual);
   Assert(fp);
   result->imagePicture = JXRenderCreatePicture(display, result->image, fp,
                                                0, NULL);

   /* Free unneeded pixmaps. */
   JXFreePixmap(display, result->image);
   result->image = None;
   JXFreePixmap(display, result->mask);
   result->mask = None;

#endif

   return result;

}

