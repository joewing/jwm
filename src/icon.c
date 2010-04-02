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

static int iconSize = 0;

#ifdef USE_ICONS

#include "x.xpm"

/* Must be a power of two. */
#define HASH_SIZE 128

/** Linked list of icon paths. */
typedef struct IconPathNode {
   char *path;
   struct IconPathNode *next;
} IconPathNode;

static IconNode **iconHash;

static IconPathNode *iconPaths;
static IconPathNode *iconPathsTail;

static GC iconGC;

static void SetIconSize();

static void DoDestroyIcon(int index, IconNode *icon);
static void ReadNetWMIcon(ClientNode *np);
static IconNode *GetDefaultIcon();
static IconNode *CreateIconFromData(const char *name, char **data);
static IconNode *CreateIconFromFile(const char *fileName);
static IconNode *CreateIconFromBinary(const unsigned long *data,
   unsigned int length);
static IconNode *LoadNamedIconHelper(const char *name, const char *path);

static IconNode *LoadSuffixedIcon(const char *path, const char *name,
   const char *suffix);

static ScaledIconNode *GetScaledIcon(IconNode *icon, int width, int height);

static void InsertIcon(IconNode *icon);
static IconNode *FindIcon(const char *name);
static int GetHash(const char *str);

/** Initialize icon data.
 * This must be initialized before parsing the configuration.
 */
void InitializeIcons() {

   int x;

   iconPaths = NULL;
   iconPathsTail = NULL;

   iconHash = Allocate(sizeof(IconNode*) * HASH_SIZE);
   for(x = 0; x < HASH_SIZE; x++) {
      iconHash[x] = NULL;
   }

}

/** Startup icon support. */
void StartupIcons() {

   XGCValues gcValues;
   unsigned long gcMask;

   gcMask = GCGraphicsExposures;
   gcValues.graphics_exposures = False;
   iconGC = JXCreateGC(display, rootWindow, gcMask, &gcValues);

}

/** Shutdown icon support. */
void ShutdownIcons() {

   int x;

   for(x = 0; x < HASH_SIZE; x++) {
      while(iconHash[x]) {
         DoDestroyIcon(x, iconHash[x]);
      }
   }

   JXFreeGC(display, iconGC);

}

/** Destroy icon data. */
void DestroyIcons() {

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
void SetIconSize() {

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
void AddIconPath(char *path) {

   IconPathNode *ip;
   int length;
   int addSep;

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
void PutIcon(IconNode *icon, Drawable d, int x, int y,
             int width, int height) {

   ScaledIconNode *node;
   int ix, iy;

   Assert(icon);

   /* Scale the icon. */
   node = GetScaledIcon(icon, width, height);

   if(node) {

      ix = x + width / 2 - node->width / 2;
      iy = y + height / 2 - node->height / 2;

      /* If we support xrender, use it. */
      if(PutScaledRenderIcon(icon, node, d, ix, iy)) {
         return;
      }

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
void LoadIcon(ClientNode *np) {

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

      }
   }

   /* Load the default icon */
   np->icon = GetDefaultIcon();

}

/** Load an icon given a name, path, and suffix. */
IconNode *LoadSuffixedIcon(const char *path, const char *name,
   const char *suffix) {

   IconNode *result;
   ImageNode *image;
   char *iconName;

   Assert(path);
   Assert(name);
   Assert(suffix);

   iconName = Allocate(strlen(name)
      + strlen(path) + strlen(suffix) + 1);
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
      result->image = image;
      InsertIcon(result);
      return result;
   } else {
      Release(iconName);
      return NULL;
   }

}

/** Load an icon from a file. */
IconNode *LoadNamedIcon(const char *name) {

   IconPathNode *ip;
   IconNode *icon;

   Assert(name);

   SetIconSize();

   if(name[0] == '/') {
      return CreateIconFromFile(name);
   } else {
      for(ip = iconPaths; ip; ip = ip->next) {
         icon = LoadNamedIconHelper(name, ip->path);
         if(icon) {
            return icon;
         }
      }
      return NULL;
   }

}

/** Helper for loading icons by name. */
IconNode *LoadNamedIconHelper(const char *name, const char *path) {

   IconNode *result;
   char *temp;

   temp = AllocateStack(strlen(name) + strlen(path) + 1);
   strcpy(temp, path);
   strcat(temp, name);
   result = CreateIconFromFile(temp);
   ReleaseStack(temp);

   return result;

}

/** Read the icon property from a client. */
void ReadNetWMIcon(ClientNode *np) {

   unsigned long count;
   int status;
   unsigned long extra;
   Atom realType;
   int realFormat;
   unsigned char *data;

   status = JXGetWindowProperty(display, np->window, atoms[ATOM_NET_WM_ICON],
      0, 256 * 256 * 4, False, XA_CARDINAL, &realType, &realFormat, &count,
      &extra, &data);

   if(status == Success && data) {
      np->icon = CreateIconFromBinary((unsigned long*)data, count);
      JXFree(data);
   }

}


/** Create the default icon. */
IconNode *GetDefaultIcon() {
   return CreateIconFromData("default", x_xpm);
}

/** Create an icon from XPM image data. */
IconNode *CreateIconFromData(const char *name, char **data) {

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
      result->image = image;
      InsertIcon(result);
      return result;
   } else {
      return NULL;
   }

}

/** Create an icon from the specified file. */
IconNode *CreateIconFromFile(const char *fileName) {

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
      result->name = CopyString(fileName);
      result->image = image;
      InsertIcon(result);
      return result;
   } else {
      return NULL;
   }

}

/** Get a scaled icon. */
ScaledIconNode *GetScaledIcon(IconNode *icon, int rwidth, int rheight) {

   XColor color;
   XImage *image;
   ScaledIconNode *np;
   GC maskGC;
   int x, y;
   int index, yindex;
   double scalex, scaley;
   double srcx, srcy;
   double ratio;
   int nwidth, nheight;
   int usesMask;
   unsigned char *data;

   Assert(icon);
   Assert(icon->image);

   if(rwidth == 0) {
      rwidth = icon->image->width;
   }
   if(rheight == 0) {
      rheight = icon->image->height;
   }

   ratio = (double)icon->image->width / icon->image->height;
   nwidth = Min(rwidth, rheight * ratio);
   nheight = Min(rheight, nwidth / ratio);
   nwidth = nheight * ratio;
   if(nwidth < 1) {
      nwidth = 1;
   }
   if(nheight < 1) {
      nheight = 1;
   }

   /* Check if this size already exists. */
   for(np = icon->nodes; np; np = np->next) {
      if(np->width == nwidth && np->height == nheight) {
         return np;
      }
   }

   /* See if we can use XRender to create the icon. */
   np = CreateScaledRenderIcon(icon, nwidth, nheight);
   if(np) {
      return np;
   }

   /* Create a new ScaledIconNode the old-fashioned way. */
   np = Allocate(sizeof(ScaledIconNode));
   np->width = nwidth;
   np->height = nheight;
   np->next = icon->nodes;
#ifdef USE_XRENDER
   np->imagePicture = None;
   np->maskPicture = None;
#endif
   icon->nodes = np;

   /* Determine if we need a mask. */
   usesMask = 0;
   x = 4 * icon->image->height * icon->image->width;
   for(index = 0; index < x; index++) {
      if(icon->image->data[index] >= 128) {
         usesMask = 1;
         break;
      }
   }

   /* Create a mask if needed. */
   if(usesMask) {
      np->mask = JXCreatePixmap(display, rootWindow, nwidth, nheight, 1);
      maskGC = JXCreateGC(display, np->mask, 0, NULL);
      JXSetForeground(display, maskGC, 0);
      JXFillRectangle(display, np->mask, maskGC, 0, 0, nwidth, nheight);
      JXSetForeground(display, maskGC, 1);
   } else {
      np->mask = None;
      maskGC = None;
   }

   /* Create a temporary XImage for scaling. */
   image = JXCreateImage(display, rootVisual, rootDepth, ZPixmap, 0,
      NULL, nwidth, nheight, 8, 0);
   image->data = Allocate(sizeof(unsigned long) * nwidth * nheight);

   /* Determine the scale factor. */
   scalex = (double)icon->image->width / nwidth;
   scaley = (double)icon->image->height / nheight;

   data = icon->image->data;
   srcy = 0.0;
   for(y = 0; y < nheight; y++) {
      srcx = 0.0;
      yindex = (int)srcy * icon->image->width;
      for(x = 0; x < nwidth; x++) {
         index = 4 * (yindex + (int)srcx);

         color.red = data[index + 1];
         color.red |= color.red << 8;
         color.green = data[index + 2];
         color.green |= color.green << 8;
         color.blue = data[index + 3];
         color.blue |= color.blue << 8;
         GetColor(&color);

         XPutPixel(image, x, y, color.pixel);

         if(usesMask && data[index] >= 128) {
            JXDrawPoint(display, np->mask, maskGC, x, y);
         }

         srcx += scalex;

      }

      srcy += scaley;
   }

   /* Release the mask GC. */
   if(usesMask) {
      JXFreeGC(display, maskGC);
   }

   /* Create the color data pixmap. */
   np->image = JXCreatePixmap(display, rootWindow, nwidth, nheight, rootDepth);

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
   unsigned int length) {

   unsigned long height, width;
   IconNode *result;
   unsigned char *data;
   unsigned int x, index;

   if(!input) {
      return NULL;
   }

   width = input[0];
   height = input[1];

   if(width * height + 2 > length) {
      Debug("invalid image size: %d x %d + 2 > %d", width, height, length);
      return NULL;
   } else if(width == 0 || height == 0) {
      Debug("invalid image size: %d x %d", width, height);
      return NULL;
   }

   result = CreateIcon();

   result->image = Allocate(sizeof(ImageNode));
   result->image->width = width;
   result->image->height = height;

   result->image->data = Allocate(4 * width * height);
   data = result->image->data;

   /* Note: the data types here might be of different sizes. */
   index = 0;
   for(x = 0; x < width * height; x++) {
      data[index++] = input[x + 2] >> 24;
      data[index++] = (input[x + 2] >> 16) & 0xFF;
      data[index++] = (input[x + 2] >> 8) & 0xFF;
      data[index++] = input[x + 2] & 0xFF;
   }

   /* Don't insert this icon since it is transient. */

   return result;

}

/** Create an empty icon node. */
IconNode *CreateIcon() {

   IconNode *icon;

   icon = Allocate(sizeof(IconNode));
   icon->name = NULL;
   icon->image = NULL;
   icon->nodes = NULL;
   icon->useRender = 1;
   icon->next = NULL;
   icon->prev = NULL;

   return icon;

}

/** Helper method for destroy icons. */
void DoDestroyIcon(int index, IconNode *icon) {

   ScaledIconNode *np;

   if(icon) {
      while(icon->nodes) {
         np = icon->nodes->next;

#ifdef USE_XRENDER
         if(icon->nodes->imagePicture != None) {
            JXRenderFreePicture(display, icon->nodes->imagePicture);
         }
         if(icon->nodes->maskPicture != None) {
            JXRenderFreePicture(display, icon->nodes->maskPicture);
         }
#endif

         if(icon->nodes->image != None) {
            JXFreePixmap(display, icon->nodes->image);
         }
         if(icon->nodes->mask != None) {
            JXFreePixmap(display, icon->nodes->mask);
         }

         Release(icon->nodes);
         icon->nodes = np;
      }

      if(icon->name) {
         Release(icon->name);
      }
      DestroyImage(icon->image);

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
void DestroyIcon(IconNode *icon) {

   int index;

   if(icon && !icon->name) {
      index = GetHash(icon->name);
      DoDestroyIcon(index, icon);
   }

}

/** Insert an icon to the icon hash table. */
void InsertIcon(IconNode *icon) {

   int index;

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
IconNode *FindIcon(const char *name) {

   IconNode *icon;
   int index;

   index = GetHash(name);

   icon = iconHash[index];
   while(icon) {
      if(!strcmp(icon->name, name)) {
         return icon;
      }
      icon = icon->next;
   }

   return NULL;

}

/** Get the hash for a string. */
int GetHash(const char *str) {

   int x;
   unsigned int hash = 0;

   if(str) {
      for(x = 0; str[x]; x++) {
         hash = (hash + (hash << 5)) ^ (unsigned int)str[x];
      }
      hash &= (HASH_SIZE - 1);
   }

   return hash;

}

#endif /* USE_ICONS */

