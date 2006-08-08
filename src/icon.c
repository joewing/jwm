/****************************************************************************
 ****************************************************************************/

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

#define HASH_SIZE 64

typedef struct IconPathNode {
	char *path;
	struct IconPathNode *next;
} IconPathNode;

static IconNode **iconHash;

static IconPathNode *iconPaths;
static IconPathNode *iconPathsTail;

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
	QueryRenderExtension();
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

/****************************************************************************
 ****************************************************************************/
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

/****************************************************************************
 ****************************************************************************/
void PutIcon(IconNode *icon, Drawable d, GC g, int x, int y,
	int width, int height) {

	ScaledIconNode *node;

	Assert(icon);

	node = GetScaledIcon(icon, width, height);

	if(node) {

		if(PutScaledRenderIcon(icon, node, d, x, y)) {
			return;
		}

		if(node->image != None) {

			if(node->mask != None) {
				JXSetClipOrigin(display, g, x, y);
				JXSetClipMask(display, g, node->mask);
			}

			JXCopyArea(display, node->image, d, g, 0, 0,
				node->width, node->height, x, y);

			if(node->mask != None) {
				JXSetClipMask(display, g, None);
				JXSetClipOrigin(display, g, 0, 0);
			}

		}

	}

}

/****************************************************************************
 ****************************************************************************/
void LoadIcon(ClientNode *np) {

	IconPathNode *ip;

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

		}
	}

	/* Load the default icon */
	np->icon = GetDefaultIcon();

}

/****************************************************************************
 ****************************************************************************/
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

/****************************************************************************
 ****************************************************************************/
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

/****************************************************************************
 ****************************************************************************/
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
		np->icon = CreateIconFromBinary((unsigned long*)data, count);
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

/****************************************************************************
 ****************************************************************************/
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

/****************************************************************************
 ****************************************************************************/
ScaledIconNode *GetScaledIcon(IconNode *icon, int rwidth, int rheight) {

	XColor color;
	ScaledIconNode *np;
	GC maskGC;
	int x, y;
	int index;
	int alpha;
	double scalex, scaley;
	double srcx, srcy;
	double ratio;
	int nwidth, nheight;

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

	np->mask = JXCreatePixmap(display, rootWindow, nwidth, nheight, 1);
	maskGC = JXCreateGC(display, np->mask, 0, NULL);
	np->image = JXCreatePixmap(display, rootWindow, nwidth, nheight, rootDepth);

	scalex = (double)icon->image->width / nwidth;
	scaley = (double)icon->image->height / nheight;

	srcy = 0.0;
	for(y = 0; y < nheight; y++) {
		srcx = 0.0;
		for(x = 0; x < nwidth; x++) {

			index = (int)srcy * icon->image->width + (int)srcx;

			alpha = (icon->image->data[index] >> 24) & 0xFFUL;
			if(alpha >= 128) {

				color.red = ((icon->image->data[index] >> 16) & 0xFFUL) * 257;
				color.green = ((icon->image->data[index] >> 8) & 0xFFUL) * 257;
				color.blue = (icon->image->data[index] & 0xFFUL) * 257;
				GetColor(&color);

				JXSetForeground(display, rootGC, color.pixel);
				JXDrawPoint(display, np->image, rootGC, x, y);

				JXSetForeground(display, maskGC, 1);

			} else {
				JXSetForeground(display, maskGC, 0);
			}
			JXDrawPoint(display, np->mask, maskGC, x, y);

			srcx += scalex;

		}

		srcy += scaley;
	}

	JXFreeGC(display, maskGC);

	return np;

}

/****************************************************************************
 ****************************************************************************/
IconNode *CreateIconFromBinary(const unsigned long *input,
	unsigned int length) {

	unsigned long height, width;
	IconNode *result;
	unsigned long *data;
	unsigned int x;

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

	result->image->data = Allocate(width * height * sizeof(unsigned long));
	data = (unsigned long*)result->image->data;

	/* Note: the data types here might be of different sizes. */
	for(x = 0; x < width * height; x++) {
		data[x] = input[x + 2];
	}

	/* Don't insert this icon since it is transient. */

	return result;

}

/****************************************************************************
 ****************************************************************************/
IconNode *CreateIcon() {

	IconNode *icon;

	icon = Allocate(sizeof(IconNode));
	icon->name = NULL;
	icon->image = NULL;
	icon->nodes = NULL;
	icon->next = NULL;
	icon->prev = NULL;

	return icon;

}

/****************************************************************************
 ****************************************************************************/
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

/****************************************************************************
 ****************************************************************************/
void DestroyIcon(IconNode *icon) {

	int index;

	if(icon && !icon->name) {
		index = GetHash(icon->name);
		DoDestroyIcon(index, icon);
	}

}

/****************************************************************************
 ****************************************************************************/
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

/****************************************************************************
 ****************************************************************************/
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

/****************************************************************************
 ****************************************************************************/
int GetHash(const char *str) {

	int x;
	int hash = 0;

	if(!str || !str[0]) {
		return hash % HASH_SIZE;
	}

	for(x = 0; str[x]; x++) {
		hash = (hash + str[x]) % HASH_SIZE;
	}

	return hash;

}

#endif /* USE_ICONS */

