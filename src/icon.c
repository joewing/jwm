/****************************************************************************
 ****************************************************************************/

#include <X11/xpm.h>

#include "jwm.h"

int iconSize = 0;

#ifdef USE_ICONS

#include "x.xpm"

#define HASH_SIZE 64

typedef struct IconPathNode {
	char *path;
	struct IconPathNode *next;
} IconPathNode;

static const char *iconSuffix = ".xpm";

static IconNode **iconHash;

static IconPathNode *iconPaths;
static IconPathNode *iconPathsTail;

static void SetIconSize();

static IconNode *CreateScaledIconFromXImages(XImage *image, XImage *shape);
static IconNode *CreateIconFromXImages(XImage *image, XImage *shape);
static IconNode *CreateIcon();
static void DoDestroyIcon(int index, IconNode *icon);
static void ReadNetWMIcon(ClientNode *np);
static IconNode *GetDefaultIcon();
static IconNode *CreateIconFromData(const char *name, char **data);
static IconNode *CreateIconFromFile(char *fileName, int scale);
static IconNode *CreateIconFromBinary(const CARD32 *data, int length);

static void InsertIcon(IconNode *icon);
static IconNode *FindIcon(const char *name, int scaled);
static int AreEqual(const char *a, int sa, const char *b, int sb);
static int GetHash(const char *str, int scaled);

/****************************************************************************
 * Must be initialized before parsing the configuration.
 ****************************************************************************/
void InitializeIcons() {

	int x;

	iconPaths = NULL;
	iconPathsTail = NULL;

	iconHash = Allocate(sizeof(IconNode*) * HASH_SIZE);
	for(x = 0; x < HASH_SIZE; x++) {
		iconHash[x] = NULL;
	}

}

/****************************************************************************
 ****************************************************************************/
void StartupIcons() {
}

/****************************************************************************
 ****************************************************************************/
void ShutdownIcons() {
	int x;
	for(x = 0; x < HASH_SIZE; x++) {
		while(iconHash[x]) {
			DoDestroyIcon(x, iconHash[x]);
		}
	}
}

/****************************************************************************
 ****************************************************************************/
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

/****************************************************************************
 ****************************************************************************/
void SetIconSize() {

	XIconSize size;

	if(!iconSize) {

		iconSize = trayHeight - 9;
		if(iconSize > titleHeight - 4) {
			iconSize = titleHeight - 4;
		}

		size.min_width = iconSize;
		size.min_height = iconSize;
		size.max_width = iconSize;
		size.max_height = iconSize;
		size.width_inc = iconSize;
		size.height_inc = iconSize;

		XSetIconSizes(display, rootWindow, &size, 1);

	}

}

/****************************************************************************
 ****************************************************************************/
void AddIconPath(const char *path) {

	IconPathNode *ip;
	int length;
	int addSep;

	Assert(path);

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

/****************************************************************************
 ****************************************************************************/
void PutIcon(const IconNode *icon, Drawable d, GC g, int x, int y) {

	if(icon->image != None) {
		if(icon->mask != None) {
			JXSetClipOrigin(display, g, x, y);
			JXSetClipMask(display, g, icon->mask);
		}

		JXCopyArea(display, icon->image, d, g, 0, 0,
			icon->width, icon->height, x, y);

		if(icon->mask != None) {
			JXSetClipMask(display, g, None);
			JXSetClipOrigin(display, g, 0, 0);
		}
	}

}

/****************************************************************************
 ****************************************************************************/
void LoadIcon(ClientNode *np) {

	IconPathNode *ip;
	char *iconName;

	Assert(np);

	SetIconSize();

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

			iconName = Allocate(strlen(np->instanceName)
				+ strlen(ip->path) + strlen(iconSuffix) + 1);
			strcpy(iconName, ip->path);
			strcat(iconName, np->instanceName);
			strcat(iconName, iconSuffix);

			np->icon = CreateIconFromFile(iconName, 1);

			Release(iconName);
			if(np->icon) {
				return;
			}

		}
	}

	/* Load the default icon */
	np->icon = GetDefaultIcon();

}

/****************************************************************************
 ****************************************************************************/
IconNode *LoadNamedIcon(char *name, int scale) {

	IconPathNode *ip;
	IconNode *icon;
	char *temp;

	Assert(name);

	SetIconSize();

	if(name[0] == '/') {
		return CreateIconFromFile(name, scale);
	} else {
		for(ip = iconPaths; ip; ip = ip->next) {
			temp = Allocate(strlen(name) + strlen(ip->path) + 1);
			strcpy(temp, ip->path);
			strcat(temp, name);
			icon = CreateIconFromFile(temp, scale);
			Release(temp);
			if(icon) {
				return icon;
			}
		}
		return NULL;
	}

}

/****************************************************************************
 ****************************************************************************/
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
		np->icon = CreateIconFromBinary((CARD32*)data, count);
		if(np->icon) {
			InsertIcon(np->icon);
		}
		JXFree(data);
	}

}


/****************************************************************************
 ****************************************************************************/
IconNode *GetDefaultIcon() {
	return CreateIconFromData("default", x_xpm);
}

/****************************************************************************
 ****************************************************************************/
IconNode *CreateIconFromData(const char *name, char **data) {

	XImage *image, *shape;
	IconNode *result;

	Assert(name);

	if(!data) {
		return NULL;
	}

	/* Check if this icon has already been loaded */
	result = FindIcon(name, 1);
	if(result) {
		return result;
	}

	if(XpmCreateImageFromData(display, data, &image,
		&shape, NULL) == XpmSuccess) {

		result = CreateScaledIconFromXImages(image, shape);
		if(result) {
			result->name = Allocate(strlen(name) + 1);
			strcpy(result->name, name);
			InsertIcon(result);
		}

		return result;

	} else {
		Debug("invalid xpm");
		return NULL;
	}


}

/****************************************************************************
 ****************************************************************************/
IconNode *CreateIconFromFile(char *fileName, int scale) {

	XImage *image, *shape;
	IconNode *result;

	if(!fileName) {
		return NULL;
	}

	/* Check if this icon has already been loaded */
	result = FindIcon(fileName, scale);
	if(result) {
		return result;
	}

	/* Load the icon */
	if(XpmReadFileToImage(display, fileName, &image,
		&shape, NULL) == XpmSuccess) {
		if(scale) {
			result = CreateScaledIconFromXImages(image, shape);
		} else {
			result = CreateIconFromXImages(image, shape);
		}
		if(result) {
			result->name = Allocate(strlen(fileName) + 1);
			strcpy(result->name, fileName);
			InsertIcon(result);
		}
		return result;
	} else {
		Debug("error reading xpm: %s", fileName);
		return NULL;
	}

}

/****************************************************************************
 ****************************************************************************/
IconNode *CreateIconFromXImages(XImage *simage, XImage *shape) {

	IconNode *result;
	GC gc;

	result = CreateIcon();
	result->scaled = 0;
	result->width = simage->width;
	result->height = simage->height;

	/* Create the mask pixmap */
	result->mask = JXCreatePixmap(display, rootWindow,
		simage->width, simage->height, 1);
	gc = JXCreateGC(display, result->mask, 0, NULL);
	JXSetForeground(display, gc, 1);
	JXFillRectangle(display, result->mask, gc, 0, 0,
		simage->width, simage->height);
	if(shape) {
		JXPutImage(display, result->mask, gc, shape, 0, 0, 0, 0,
			shape->width, shape->height);
		JXDestroyImage(shape);
	}
	JXFreeGC(display, gc);

	/* Create the image pixmap */
	result->image = JXCreatePixmap(display, rootWindow,
		simage->width, simage->height, rootDepth);
	gc = JXCreateGC(display, result->image, 0, NULL);
	JXPutImage(display, result->image, gc, simage, 0, 0, 0, 0,
		simage->width, simage->height);
	JXFreeGC(display, gc);
	JXDestroyImage(simage);

	return result;

}

/****************************************************************************
 ****************************************************************************/
IconNode *CreateScaledIconFromXImages(XImage *simage, XImage *shape) {

	XImage *dimage;
	double scalex, scaley;
	double srcx, srcy;
	int destx, desty;
	CARD32 pixel, pixelMask;
	GC gc;
	char *data;
	IconNode *result;

	Assert(simage);

	scalex = (double)simage->width / iconSize;
	scaley = (double)simage->height / iconSize;

	data = Allocate(iconSize * iconSize * sizeof(CARD32));

	result = CreateIcon();
	result->scaled = 1;
	result->width = iconSize;
	result->height = iconSize;

	dimage = JXCreateImage(display, rootVisual, rootDepth, ZPixmap, 0,
		data, iconSize, iconSize, 32, 0);

	result->mask = JXCreatePixmap(display, rootWindow, iconSize, iconSize, 1);
	gc = JXCreateGC(display, result->mask, 0, NULL);
	JXSetForeground(display, gc, 0);
	JXFillRectangle(display, result->mask, gc, 0, 0, iconSize, iconSize);
	JXSetForeground(display, gc, 1);

	srcy = 0.0;
	for(desty = 0; desty < iconSize; desty++) {
		srcx = 0.0;
		for(destx = 0; destx < iconSize; destx++) {
			pixel = XGetPixel(simage, (int)srcx, (int)srcy);
			if(shape) {
				pixelMask = XGetPixel(shape, (int)srcx, (int)srcy);
			} else {
				pixelMask = 1;
			}
			XPutPixel(dimage, destx, desty, pixel);
			if(pixelMask) {
				XDrawPoint(display, result->mask, gc, destx, desty);
			}
			srcx += scalex;
		}
		srcy += scaley;
	}

	JXFreeGC(display, gc);
	JXDestroyImage(simage);
	if(shape) {
		JXDestroyImage(shape);
	}

	/* Create the image pixmap */
	result->image = JXCreatePixmap(display, rootWindow, iconSize,
		iconSize, rootDepth);

	gc = JXCreateGC(display, result->image, 0, NULL);
	JXPutImage(display, result->image, gc, dimage, 0, 0, 0, 0,
		iconSize, iconSize);
	JXFreeGC(display, gc);

	Release(dimage->data);
	dimage->data = NULL;
	JXDestroyImage(dimage);

	return result;
}

/****************************************************************************
 ****************************************************************************/
IconNode *CreateIconFromBinary(const CARD32 *input, int length) {

	double scalex, scaley;
	CARD32 height, width;
	CARD32 destx, desty;
	CARD32 index;
	double srcx, srcy;
	CARD32 alpha;
	char *data;
	XColor color;
	IconNode *result;
	XImage *image;
	GC gc;

	if(!input) {
		return NULL;
	}

	width = input[0];
	height = input[1];

	if(width * height + 2 > length) {
		Debug("invalid image size: %d x %d + 2 > %d", width, height, length);
		return NULL;
	}

	scalex = (double)width / iconSize;
	scaley = (double)height / iconSize;

	data = Allocate(iconSize * iconSize * sizeof(CARD32));
	image = XCreateImage(display, rootVisual, rootDepth, ZPixmap, 0,
		data, iconSize, iconSize, 32, 0);

	result = CreateIcon();
	result->scaled = 1;
	result->width = iconSize;
	result->height = iconSize;

	result->mask = JXCreatePixmap(display, rootWindow, iconSize, iconSize, 1);
	gc = JXCreateGC(display, result->mask, 0, NULL);
	JXSetForeground(display, gc, 0);
	JXFillRectangle(display, result->mask, gc, 0, 0, iconSize, iconSize);
	JXSetForeground(display, gc, 1);

	srcy = 0.0;
	for(desty = 0; desty < iconSize; desty++) {
		srcx = 0.0;
		for(destx = 0; destx < iconSize; destx++) {
			index = 2 + (CARD32)srcy * width + (CARD32)srcx;
			alpha = (input[index] >> 24) & 0xFF;
			color.red = ((input[index] >> 16) & 0xFF) * 257;
			color.green = ((input[index] >> 8) & 0xFF) * 257;
			color.blue = (input[index] & 0xFF) * 257;
			if(alpha >= 128) {
				GetColor(&color);
				XPutPixel(image, destx, desty, color.pixel);
				XDrawPoint(display, result->mask, gc, destx, desty);
			}
			srcx += scalex;
		}
		srcy += scaley;
	}

	JXFreeGC(display, gc);

	result->image = JXCreatePixmap(display, rootWindow,
		iconSize, iconSize, rootDepth);
	gc = JXCreateGC(display, result->image, 0, NULL);
	JXPutImage(display, result->image, gc, image, 0, 0, 0, 0,
		iconSize, iconSize);
	JXFreeGC(display, gc);

	Release(image->data);
	image->data = NULL;
	JXDestroyImage(image);

	return result;

}

/****************************************************************************
 ****************************************************************************/
IconNode *CreateIcon() {

	IconNode *icon;

	icon = Allocate(sizeof(IconNode));
	icon->name = NULL;
	icon->image = None;
	icon->mask = None;
	icon->scaled = 0;
	icon->width = 0;
	icon->height = 0;
	icon->next = NULL;
	icon->prev = NULL;

	return icon;

}

/****************************************************************************
 ****************************************************************************/
void DoDestroyIcon(int index, IconNode *icon) {

	if(icon) {
		if(icon->image) {
			JXFreePixmap(display, icon->image);
		}
		if(icon->mask != None) {
			JXFreePixmap(display, icon->mask);
		}
		if(icon->name) {
			Release(icon->name);
		}
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

/****************************************************************************
 ****************************************************************************/
void DestroyIcon(IconNode *icon) {

	int index;

	if(icon && !icon->name) {
		index = GetHash(icon->name, icon->scaled);
		DoDestroyIcon(index, icon);
	}

}

/****************************************************************************
 ****************************************************************************/
void InsertIcon(IconNode *icon) {

	int index;

	Assert(icon);

	index = GetHash(icon->name, icon->scaled);

	icon->prev = NULL;
	if(iconHash[index]) {
		iconHash[index]->prev = icon;
	}
	icon->next = iconHash[index];
	iconHash[index] = icon;

}

/****************************************************************************
 ****************************************************************************/
IconNode *FindIcon(const char *name, int scaled) {

	IconNode *icon;
	int index;

	index = GetHash(name, scaled);

	icon = iconHash[index];
	while(icon) {
		if(AreEqual(icon->name, icon->scaled, name, scaled)) {
			return icon;
		}
		icon = icon->next;
	}

	return NULL;

}

/****************************************************************************
 ****************************************************************************/
int AreEqual(const char *a, int sa, const char *b, int sb) {

	if(sa != sb) {
		return 0;
	} else if(!a && !b) {
		return 1;
	} else if(!a || !b) {
		return 0;
	} else if(!strcmp(a, b)) {
		return 1;
	} else {
		return 0;
	}

}

/****************************************************************************
 ****************************************************************************/
int GetHash(const char *str, int scaled) {

	int x;
	int hash = scaled;

	if(!str || !str[0]) {
		return hash % HASH_SIZE;
	}

	for(x = 0; str[x]; x++) {
		hash = (hash + str[x]) % HASH_SIZE;
	}

	return hash;

}

#endif /* USE_ICONS */

