/**
 * @file icon.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Icon functions.
 *
 */

#include "jwm.h"
#include "icon.h"
#include "client.h"
#include "render.h"
#include "main.h"
#include "image.h"
#include "misc.h"
#include "hint.h"
#include "color.h"

IconNode emptyIcon;

#ifdef USE_ICONS

#include "x.xpm"

/* Must be a power of two. */
#define HASH_SIZE 128

/** Linked list of icon paths. */
typedef struct IconPathNode {
   char *path;
   struct IconPathNode *next;
} IconPathNode;

static int iconSize = 0;
static IconNode **iconHash;
static IconPathNode *iconPaths;
static IconPathNode *iconPathsTail;
static GC iconGC;

static void SetIconSize(void);
static void DoDestroyIcon(int index, IconNode *icon);
static void ReadNetWMIcon(ClientNode *np);
static IconNode *CreateIcon(void);
static IconNode *GetDefaultIcon(void);
static IconNode *CreateIconFromData(const char *name, char **data);
static IconNode *CreateIconFromFile(const char *fileName,
                                    char save, char preserveAspect);
static IconNode *CreateIconFromBinary(const unsigned long *data,
                                      unsigned int length);
static IconNode *LoadNamedIconHelper(const char *name, const char *path,
                                     char save, char preserveAspect);

#if defined(USE_ICONS)
static IconNode *LoadSuffixedIcon(const char *path, const char *name,
                                  const char *suffix);
#endif

static ImageNode *GetBestImage(IconNode *icon, int rwidth, int rheight);
static ScaledIconNode *GetScaledIcon(IconNode *icon, ImageNode *iconImage,
                                     long fg, int rwidth, int rheight);

static void InsertIcon(IconNode *icon);
static IconNode *FindIcon(const char *name);
static unsigned int GetHash(const char *str);

/** Initialize icon data.
 * This must be initialized before parsing the configuration.
 */
void InitializeIcons(void)
{
   unsigned int x;
   iconPaths = NULL;
   iconPathsTail = NULL;
   iconHash = Allocate(sizeof(IconNode*) * HASH_SIZE);
   for(x = 0; x < HASH_SIZE; x++) {
      iconHash[x] = NULL;
   }
   memset(&emptyIcon, 0, sizeof(emptyIcon));
}

/** Startup icon support. */
void StartupIcons(void)
{
   XGCValues gcValues;
   unsigned long gcMask;
   gcMask = GCGraphicsExposures;
   gcValues.graphics_exposures = False;
   iconGC = JXCreateGC(display, rootWindow, gcMask, &gcValues);
}

/** Shutdown icon support. */
void ShutdownIcons(void)
{
   unsigned int x;
   for(x = 0; x < HASH_SIZE; x++) {
      while(iconHash[x]) {
         DoDestroyIcon(x, iconHash[x]);
      }
   }
   JXFreeGC(display, iconGC);
}

/** Destroy icon data. */
void DestroyIcons(void)
{
   IconPathNode *pn;
   while(iconPaths) {
      pn = iconPaths->next;
      Release(iconPaths->path);
      Release(iconPaths);
      iconPaths = pn;
   }
   iconPathsTail = NULL;
   if(iconHash) {
      Release(iconHash);
      iconHash = NULL;
   }
}

/** Set the preferred icon sizes on the root window. */
void SetIconSize(void)
{

   XIconSize size;

   if(!iconSize) {

      /* FIXME: compute values based on the sizes we can actually use. */
      iconSize = 32;

      size.min_width = iconSize;
      size.min_height = iconSize;
      size.max_width = iconSize;
      size.max_height = iconSize;
      size.width_inc = iconSize;
      size.height_inc = iconSize;

      JXSetIconSizes(display, rootWindow, &size, 1);

   }

}

/** Add an icon search path. */
void AddIconPath(char *path)
{

   IconPathNode *ip;
   int length;
   char addSep;

   if(!path) {
      return;
   }

   Trim(path);

   length = strlen(path);
   if(path[length - 1] != '/') {
      addSep = 1;
   } else {
      addSep = 0;
   }

   ip = Allocate(sizeof(IconPathNode));
   ip->path = Allocate(length + addSep + 1);
   strcpy(ip->path, path);
   if(addSep) {
      ip->path[length] = '/';
      ip->path[length + 1] = 0;
   }
   ExpandPath(&ip->path);
   ip->next = NULL;

   if(iconPathsTail) {
      iconPathsTail->next = ip;
   } else {
      iconPaths = ip;
   }
   iconPathsTail = ip;

}

/** Draw an icon. */
void PutIcon(const VisualData *visual, IconNode *icon, Drawable d, long fg,
             int x, int y, int width, int height)
{
   ImageNode *imageNode;
   ScaledIconNode *node;

   Assert(icon);

   if(icon == &emptyIcon) {
      return;
   }

   /* Scale the icon. */
   imageNode = GetBestImage(icon, width, height);
   node = GetScaledIcon(icon, imageNode, fg, width, height);
   if(node) {

      const int ix = x + (width - node->width) / 2;
      const int iy = y + (height - node->height) / 2;

      /* If we support xrender, use it. */
#ifdef USE_XRENDER
      if(haveRender) {
         PutScaledRenderIcon(visual, imageNode, d, ix, iy);
         return;
      }
#endif

      /* Draw the icon the old way. */
      if(node->image != None) {

         /* Set the clip mask. */
         if(node->mask != None) {
            JXSetClipOrigin(display, iconGC, ix, iy);
            JXSetClipMask(display, iconGC, node->mask);
         }

         /* Draw the icon. */
         JXCopyArea(display, node->image, d, iconGC, 0, 0,
                    node->width, node->height, ix, iy);

         /* Reset the clip mask. */
         if(node->mask != None) {
            JXSetClipMask(display, iconGC, None);
            JXSetClipOrigin(display, iconGC, 0, 0);
         }

      }

   }

}

/** Load the icon for a client. */
void LoadIcon(ClientNode *np)
{

   IconPathNode *ip;

   Assert(np);

   SetIconSize();

   /* If client already has an icon, destroy it first. */
   DestroyIcon(np->icon);
   np->icon = NULL;

   /* Attempt to read _NET_WM_ICON for an icon */
   ReadNetWMIcon(np);
   if(np->icon) {
      return;
   }

   /* Attempt to find an icon for this program in the icon directory */
   if(np->instanceName) {
      for(ip = iconPaths; ip; ip = ip->next) {

#ifdef USE_PNG
         np->icon = LoadSuffixedIcon(ip->path, np->instanceName, ".png");
         if(np->icon) {
            return;
         }
#endif

#ifdef USE_XPM
         np->icon = LoadSuffixedIcon(ip->path, np->instanceName, ".xpm");
         if(np->icon) {
            return;
         }
#endif

#ifdef USE_JPEG
         np->icon = LoadSuffixedIcon(ip->path, np->instanceName, ".jpg");
         if(np->icon) {
            return;
         }
#endif

#ifdef USE_ICONS
         np->icon = LoadSuffixedIcon(ip->path, np->instanceName, ".xbm");
         if(np->icon) {
            return;
         }
#endif

      }
   }

   /* Load the default icon */
   np->icon = GetDefaultIcon();

}

/** Load an icon given a name, path, and suffix. */
#if defined(USE_ICONS)
IconNode *LoadSuffixedIcon(const char *path, const char *name,
                           const char *suffix)
{

   IconNode *result;
   ImageNode *image;
   char *iconName;
   unsigned int len;

   Assert(path);
   Assert(name);
   Assert(suffix);

   len = strlen(name) + strlen(path) + strlen(suffix);
   iconName = Allocate(len + 1);
   strcpy(iconName, path);
   strcat(iconName, name);
   strcat(iconName, suffix);

   result = FindIcon(iconName);
   if(result) {
      Release(iconName);
      return result;
   }

   image = LoadImage(iconName);
   if(image) {
      result = CreateIcon();
      result->name = iconName;
      result->images = image;
      InsertIcon(result);
      return result;
   } else {
      Release(iconName);
      return NULL;
   }

}
#endif /* defined(USE_PNG) || defined(USE_XPM) || defined(USE_JPEG) */

/** Load an icon from a file. */
IconNode *LoadNamedIcon(const char *name, char save, char preserveAspect)
{

   IconPathNode *ip;
   IconNode *icon;

   Assert(name);

   SetIconSize();

   if(name[0] == '/') {
      return CreateIconFromFile(name, save, preserveAspect);
   } else {
      for(ip = iconPaths; ip; ip = ip->next) {
         icon = LoadNamedIconHelper(name, ip->path, save, preserveAspect);
         if(icon) {
            return icon;
         }
      }
      return NULL;
   }

}

/** Helper for loading icons by name. */
IconNode *LoadNamedIconHelper(const char *name, const char *path,
                              char save, char preserveAspect)
{

   IconNode *result;
   char *temp;

   temp = AllocateStack(strlen(name) + strlen(path) + 1);
   strcpy(temp, path);
   strcat(temp, name);
   result = CreateIconFromFile(temp, save, preserveAspect);
   ReleaseStack(temp);

   return result;

}

/** Read the icon property from a client. */
void ReadNetWMIcon(ClientNode *np)
{
   static const long MAX_LENGTH = 1 << 20;
   unsigned long count;
   int status;
   unsigned long extra;
   Atom realType;
   int realFormat;
   unsigned char *data;
   status = JXGetWindowProperty(display, np->window, atoms[ATOM_NET_WM_ICON],
                                0, MAX_LENGTH, False, XA_CARDINAL,
                                &realType, &realFormat, &count, &extra, &data);
   if(status == Success && realFormat != 0 && data) {
      np->icon = CreateIconFromBinary((unsigned long*)data, count);
      JXFree(data);
   }
}


/** Create the default icon. */
IconNode *GetDefaultIcon(void)
{
   return CreateIconFromData("default", x_xpm);
}

/** Create an icon from XPM image data. */
IconNode *CreateIconFromData(const char *name, char **data)
{

   ImageNode *image;
   IconNode *result;

   Assert(name);
   Assert(data);

   /* Check if this icon has already been loaded */
   result = FindIcon(name);
   if(result) {
      return result;
   }

   image = LoadImageFromData(data);
   if(image) {
      result = CreateIcon();
      result->name = CopyString(name);
      result->images = image;
      InsertIcon(result);
      return result;
   } else {
      return NULL;
   }

}

/** Create an icon from the specified file. */
IconNode *CreateIconFromFile(const char *fileName,
                             char save, char preserveAspect)
{

   ImageNode *image;
   IconNode *result;

   if(!fileName) {
      return NULL;
   }

   /* Check if this icon has already been loaded */
   result = FindIcon(fileName);
   if(result) {
      return result;
   }

   image = LoadImage(fileName);
   if(image) {
      result = CreateIcon();
      result->preserveAspect = preserveAspect;
      result->images = image;
      if(save) {
         result->name = CopyString(fileName);
         InsertIcon(result);
      }
      return result;
   } else {
      return NULL;
   }

}

/** Get the best image for the requested size. */
ImageNode *GetBestImage(IconNode *icon, int rwidth, int rheight)
{
   /* Find the best image to use.
    * Select the smallest image to completely cover the
    * requested size.  If no image completely covers the
    * requested size, select the one that overlaps the most area.
    * If no size is specified, use the largest. */
   ImageNode *best = icon->images;
   ImageNode *ip = icon->images->next;
   while(ip) {
      const int best_area = best->width * best->height;
      const int other_area = ip->width * ip->height;
      int best_overlap;
      int other_overlap;
      if(rwidth == 0 && rheight == 0) {
         best_overlap = 0;
         other_overlap = 0;
      } else if(rwidth == 0) {
         best_overlap = Min(best->height, rheight);
         other_overlap = Min(ip->height, rheight);
      } else if(rheight == 0) {
         best_overlap = Min(best->width, rwidth);
         other_overlap = Min(ip->width, rwidth);
      } else {
         best_overlap = Min(best->width, rwidth)
                      * Min(best->height, rheight);
         other_overlap = Min(ip->width, rwidth)
                       * Min(ip->height, rheight);
      }
      if(other_overlap > best_overlap) {
         best = ip;
      } else if(other_overlap == best_overlap) {
         if(other_area < best_area) {
            best = ip;
         }
      }
      ip = ip->next;
   }
   return best;
}

/** Get a scaled icon. */
ScaledIconNode *GetScaledIcon(IconNode *icon, ImageNode *iconImage,
                              long fg, int rwidth, int rheight)
{

   XColor color;
   XImage *image;
   XPoint *points;
   ScaledIconNode *np;
   GC maskGC;
   int x, y;
   int scalex, scaley;     /* Fixed point. */
   int srcx, srcy;         /* Fixed point. */
   int ratio;              /* Fixed point. */
   int nwidth, nheight;
   unsigned char *data;

   if(rwidth == 0) {
      rwidth = iconImage->width;
   }
   if(rheight == 0) {
      rheight = iconImage->height;
   }

   if(icon->preserveAspect) {
      ratio = (iconImage->width << 16) / iconImage->height;
      nwidth = Min(rwidth, (rheight * ratio) >> 16);
      nheight = Min(rheight, (nwidth << 16) / ratio);
      nwidth = (nheight * ratio) >> 16;
   } else {
      nheight = rheight;
      nwidth = rwidth;
   }
   if(nwidth < 1) {
      nwidth = 1;
   }
   if(nheight < 1) {
      nheight = 1;
   }

   /* Check if this size already exists.
    * Note that XRender scales on the fly.
    */
   for(np = iconImage->nodes; np; np = np->next) {
#ifdef USE_XRENDER
      if(np->imagePicture != None) {
         np->width = nwidth;
         np->height = nheight;
         return np;
      }
#endif
      if(np->width == nwidth && np->height == nheight) {
         if(!iconImage->bitmap || np->fg == fg) {
            return np;
         }
      }
   }

   /* See if we can use XRender to create the icon. */
#ifdef USE_XRENDER
   if(haveRender) {
      np = CreateScaledRenderIcon(iconImage, fg);

      /* Don't keep the image data around after creating the icon. */
      Release(iconImage->data);
      iconImage->data = NULL;

      np->width = nwidth;
      np->height = nheight;

      return np;
   }
#endif

   /* Create a new ScaledIconNode the old-fashioned way. */
   np = Allocate(sizeof(ScaledIconNode));
   np->fg = fg;
   np->width = nwidth;
   np->height = nheight;
   np->next = iconImage->nodes;
#ifdef USE_XRENDER
   np->imagePicture = None;
#endif
   iconImage->nodes = np;

   /* Create a mask. */
   np->mask = JXCreatePixmap(display, rootWindow, nwidth, nheight, 1);
   maskGC = JXCreateGC(display, np->mask, 0, NULL);
   JXSetForeground(display, maskGC, 0);
   JXFillRectangle(display, np->mask, maskGC, 0, 0, nwidth, nheight);
   JXSetForeground(display, maskGC, 1);

   /* Create a temporary XImage for scaling. */
   image = JXCreateImage(display, rootVisual.visual, rootVisual.depth,
                         ZPixmap, 0, NULL, nwidth, nheight, 8, 0);
   image->data = Allocate(sizeof(unsigned long) * nwidth * nheight);

   /* Determine the scale factor. */
   scalex = (iconImage->width << 16) / nwidth;
   scaley = (iconImage->height << 16) / nheight;

   points = Allocate(sizeof(XPoint) * nwidth);
   data = iconImage->data;
   srcy = 0;
   for(y = 0; y < nheight; y++) {
      const int yindex = (srcy >> 16) * iconImage->width;
      int pindex = 0;
      srcx = 0;
      for(x = 0; x < nwidth; x++) {
         if(iconImage->bitmap) {
            const int index = yindex + (srcx >> 16);
            const int offset = index >> 3;
            const int mask = 1 << (index & 7);
            if(data[offset] & mask) {
               points[pindex].x = x;
               points[pindex].y = y;
               XPutPixel(image, x, y, fg);
               pindex += 1;
            }
         } else {
            const int yindex = (srcy >> 16) * iconImage->width;
            const int index = 4 * (yindex + (srcx >> 16));
            color.red = data[index + 1];
            color.red |= color.red << 8;
            color.green = data[index + 2];
            color.green |= color.green << 8;
            color.blue = data[index + 3];
            color.blue |= color.blue << 8;
            GetColor(&color);
            XPutPixel(image, x, y, color.pixel);
            if(data[index] >= 128) {
               points[pindex].x = x;
               points[pindex].y = y;
               pindex += 1;
            }
         }
         srcx += scalex;
      }
      JXDrawPoints(display, np->mask, maskGC, points, pindex, CoordModeOrigin);
      srcy += scaley;
   }
   Release(points);

   /* Release the mask GC. */
   JXFreeGC(display, maskGC);
 
   /* Create the color data pixmap. */
   np->image = JXCreatePixmap(display, rootWindow, nwidth, nheight,
                              rootVisual.depth);

   /* Render the image to the color data pixmap. */
   JXPutImage(display, np->image, rootGC, image, 0, 0, 0, 0, nwidth, nheight);   
   /* Release the XImage. */
   Release(image->data);
   image->data = NULL;
   JXDestroyImage(image);

   return np;

}

/** Create an icon from binary data (as specified via window properties). */
IconNode *CreateIconFromBinary(const unsigned long *input,
                               unsigned int length)
{
   IconNode *result = NULL;
   unsigned int offset = 0;

   if(!input) {
      return NULL;
   }

   while(offset < length) {

      const unsigned int width = input[offset + 0];
      const unsigned int height = input[offset + 1];
      unsigned char *data;
      ImageNode *image;
      unsigned int x, index;

      if(JUNLIKELY(width * height + 2 > length - offset)) {
         Debug("invalid image size: %d x %d + 2 > %d",
               width, height, length - offset);
         return result;
      } else if(JUNLIKELY(width == 0 || height == 0)) {
         Debug("invalid image size: %d x %d", width, height);
         return result;
      }

      if(result == NULL) {
         result = CreateIcon();
      }

      image = CreateImage(width, height, 0);
      image->next = result->images;
      result->images = image;
      data = image->data;

      /* Note: the data types here might be of different sizes. */
      index = 0;
      for(x = 0; x < width * height; x++) {
         data[index++] = (input[offset + x + 2] >> 24) & 0xFF;
         data[index++] = (input[offset + x + 2] >> 16) & 0xFF;
         data[index++] = (input[offset + x + 2] >>  8) & 0xFF;
         data[index++] = (input[offset + x + 2] >>  0) & 0xFF;
      }

      /* Move to the next size. */
      offset += 2 + width * height;

      /* Don't insert this icon into the hash since it is transient. */

   }

   return result;
}

/** Create an empty icon node. */
IconNode *CreateIcon(void)
{
   IconNode *icon;
   icon = Allocate(sizeof(IconNode));
   icon->name = NULL;
   icon->images = NULL;
   icon->next = NULL;
   icon->prev = NULL;
   icon->preserveAspect = 1;
   return icon;
}

/** Helper method for destroy icons. */
void DoDestroyIcon(int index, IconNode *icon)
{
   if(icon) {
      ImageNode *image = icon->images;
      while(image) {
         ScaledIconNode *np = image->nodes;
         while(np) {
            ScaledIconNode *next_node = np->next;

#ifdef USE_XRENDER
            if(np->imagePicture != None) {
               JXRenderFreePicture(display, np->imagePicture);
            }
            if(np->alphaPicture != None) {
               JXRenderFreePicture(display, np->alphaPicture);
            }
#endif
            if(np->image != None) {
               JXFreePixmap(display, np->image);
            }
            if(np->mask != None) {
               JXFreePixmap(display, np->mask);
            }

            Release(np);
            np = next_node;
         }
         image = image->next;
      }
      if(icon->name) {
         Release(icon->name);
      }
      DestroyImage(icon->images);

      if(icon->prev) {
         icon->prev->next = icon->next;
      } else {
         iconHash[index] = icon->next;
      }
      if(icon->next) {
         icon->next->prev = icon->prev;
      }
      Release(icon);
   }
}

/** Destroy an icon. */
void DestroyIcon(IconNode *icon)
{
   if(icon && !icon->name) {
      const unsigned int index = GetHash(icon->name);
      DoDestroyIcon(index, icon);
   }
}

/** Insert an icon to the icon hash table. */
void InsertIcon(IconNode *icon)
{
   unsigned int index;
   Assert(icon);
   Assert(icon->name);
   index = GetHash(icon->name);
   icon->prev = NULL;
   if(iconHash[index]) {
      iconHash[index]->prev = icon;
   }
   icon->next = iconHash[index];
   iconHash[index] = icon;
}

/** Find a icon in the icon hash table. */
IconNode *FindIcon(const char *name)
{
   const unsigned int index = GetHash(name);
   IconNode *icon = iconHash[index];
   while(icon) {
      if(!strcmp(icon->name, name)) {
         return icon;
      }
      icon = icon->next;
   }

   return NULL;
}

/** Get the hash for a string. */
unsigned int GetHash(const char *str)
{
   unsigned int hash = 0;
   if(str) {
      unsigned int x;
      for(x = 0; str[x]; x++) {
         hash = (hash + (hash << 5)) ^ (unsigned int)str[x];
      }
      hash &= (HASH_SIZE - 1);
   }
   return hash;
}

#endif /* USE_ICONS */

