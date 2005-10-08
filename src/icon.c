/****************************************************************************
 ****************************************************************************/

#include "jwm.h"

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
static IconNode *GetDefaultIcon(int size);
static IconNode *CreateIconFromData(const char *name, char **data, int size);
static IconNode *CreateIconFromFile(const char *fileName, int size);
static IconNode *CreateIconFromBinary(const CARD32 *data, int length,
	int size);
static IconNode *CreateIconFromImage(ImageNode *image);
static IconNode *LoadSuffixedIcon(const char *path, const char *name,
	const char *suffix, int size);

static void InsertIcon(IconNode *icon);
static IconNode *FindIcon(const char *name, int size);
static int AreEqual(const char *a, int sa, const char *b, int sb);
static int GetHash(const char *str, int size);

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

		iconSize = trayHeight - 9;
		if(iconSize < titleHeight - 4) {
			iconSize = titleHeight - 4;
		}

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

	Assert(icon);

	if(PutRenderIcon(icon, d, x, y)) {
		return;
	}

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

	Assert(np);

	SetIconSize();

	DestroyIcon(np->titleIcon);
	np->titleIcon = NULL;
	DestroyIcon(np->trayIcon);
	np->trayIcon = NULL;

	/* Attempt to read _NET_WM_ICON for an icon */
	ReadNetWMIcon(np);
	if(np->titleIcon) {
		return;
	}

	/* Attempt to find an icon for this program in the icon directory */
	if(np->instanceName) {
		for(ip = iconPaths; ip; ip = ip->next) {

#ifdef USE_PNG
			np->titleIcon = LoadSuffixedIcon(ip->path, np->instanceName,
				".png", GetBorderIconSize());
			if(np->titleIcon) {
				np->trayIcon = LoadSuffixedIcon(ip->path, np->instanceName,
					".png", GetTrayIconSize());
				return;
			}
#endif

#ifdef USE_XPM
			np->titleIcon = LoadSuffixedIcon(ip->path, np->instanceName,
				".xpm", GetBorderIconSize());
			if(np->titleIcon) {
				np->trayIcon = LoadSuffixedIcon(ip->path, np->instanceName,
					".xpm", GetTrayIconSize());
				return;
			}
#endif

		}
	}

	/* Load the default icon */
	np->titleIcon = GetDefaultIcon(GetBorderIconSize());
	np->trayIcon = GetDefaultIcon(GetTrayIconSize());

}

/****************************************************************************
 ****************************************************************************/
IconNode *LoadSuffixedIcon(const char *path, const char *name,
	const char *suffix, int size) {

	IconNode *result = NULL;
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

	result = FindIcon(iconName, size);
	if(result) {
		Release(iconName);
		return result;
	}

	image = LoadImage(iconName);
	if(image) {
		ScaleImage(image, size);
		result = CreateIconFromImage(image);
		result->size = size;
		result->name = iconName;
		InsertIcon(result);
		DestroyImage(image);
	} else {
		Release(iconName);
	}

	return result;

}

/****************************************************************************
 ****************************************************************************/
IconNode *LoadNamedIcon(const char *name, int size) {

	IconPathNode *ip;
	IconNode *icon;
	char *temp;

	Assert(name);

	SetIconSize();

	if(name[0] == '/') {
		return CreateIconFromFile(name, size);
	} else {
		for(ip = iconPaths; ip; ip = ip->next) {
			temp = Allocate(strlen(name) + strlen(ip->path) + 1);
			strcpy(temp, ip->path);
			strcat(temp, name);
			icon = CreateIconFromFile(temp, size);
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
		np->titleIcon = CreateIconFromBinary((CARD32*)data, count,
			GetBorderIconSize());
		if(np->titleIcon) {
			InsertIcon(np->titleIcon);
		}
		np->trayIcon = CreateIconFromBinary((CARD32*)data, count,
			GetTrayIconSize());
		if(np->trayIcon) {
			InsertIcon(np->trayIcon);
		}
		JXFree(data);
	}

}


/****************************************************************************
 ****************************************************************************/
IconNode *GetDefaultIcon(int size) {
	return CreateIconFromData("default", x_xpm, size);
}

/****************************************************************************
 ****************************************************************************/
IconNode *CreateIconFromData(const char *name, char **data, int size) {

	ImageNode *image;
	IconNode *result;

	Assert(name);

	if(!data) {
		return NULL;
	}

	/* Check if this icon has already been loaded */
	result = FindIcon(name, size);
	if(result) {
		return result;
	}

	image = LoadImageFromData(data);
	if(image) {
		ScaleImage(image, size);
		result = CreateIconFromImage(image);
		DestroyImage(image);
		result->name = Allocate(strlen(name) + 1);
		strcpy(result->name, name);
		result->size = size;
		InsertIcon(result);
		return result;
	} else {
		return NULL;
	}


}

/****************************************************************************
 ****************************************************************************/
IconNode *CreateIconFromFile(const char *fileName, int size) {

	IconNode *result = NULL;
	ImageNode *image;

	if(!fileName) {
		return NULL;
	}

	/* Check if this icon has already been loaded */
	result = FindIcon(fileName, size);
	if(result) {
		return result;
	}

	image = LoadImage(fileName);
	if(image) {
		ScaleImage(image, size);
		result = CreateIconFromImage(image);
		if(result) {
			result->name = Allocate(strlen(fileName) + 1);
			strcpy(result->name, fileName);
			result->size = size;
			InsertIcon(result);
		}
		DestroyImage(image);
	}

	return result;

}

/****************************************************************************
 ****************************************************************************/
IconNode *CreateIconFromImage(ImageNode *image) {

	IconNode *result;
	XColor color;
	unsigned char alpha;
	int x, y;
	int index;
	GC imageGC, maskGC;
	CARD32 *data;

	Assert(image);

	result = CreateRenderIconFromImage(image);
	if(result) {
		return result;
	}

	result = CreateIcon();
	result->width = image->width;
	result->height = image->height;

	result->mask = JXCreatePixmap(display, rootWindow,
		image->width, image->height, 1);
	maskGC = JXCreateGC(display, result->mask, 0, NULL);
	result->image = JXCreatePixmap(display, rootWindow,
		image->width, image->height, rootDepth);
	imageGC = JXCreateGC(display, result->image, 0, NULL);

	data = (CARD32*)image->data;

	for(y = 0; y < image->height; y++) {
		for(x = 0; x < image->width; x++) {
			index = y * image->width + x;

			alpha = (data[index] >> 24) & 0xFF;
			color.red = ((data[index] >> 16) & 0xFF) * 257;
			color.green = ((data[index] >> 8) & 0xFF) * 257;
			color.blue = (data[index] & 0xFF) * 257;

			GetColor(&color);
			JXSetForeground(display, imageGC, color.pixel);
			JXDrawPoint(display, result->image, imageGC, x, y);
			if(alpha >= 128) {
				JXSetForeground(display, maskGC, 1);
			} else {
				JXSetForeground(display, maskGC, 0);
			}
			JXDrawPoint(display, result->mask, maskGC, x, y);
		}
	}

	JXFreeGC(display, maskGC);
	JXFreeGC(display, imageGC);

	return result;

}

/****************************************************************************
 ****************************************************************************/
IconNode *CreateIconFromBinary(const CARD32 *input, int length, int size) {

	CARD32 height, width;
	IconNode *result;
	ImageNode *image;

	if(!input) {
		return NULL;
	}

	width = input[0];
	height = input[1];

	if(width * height + 2 > length) {
		Debug("invalid image size: %d x %d + 2 > %d", width, height, length);
		return NULL;
	}

	image = Allocate(sizeof(ImageNode));
	image->width = width;
	image->height = height;

	image->data = Allocate(width * height * sizeof(CARD32));
	memcpy(image->data, input + 2, width * height * sizeof(CARD32));

	ScaleImage(image, size);
	result = CreateIconFromImage(image);
	result->size = size;
	/* Don't insert this icon since it is transient. */
	DestroyImage(image);

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
#ifdef USE_XRENDER
	icon->imagePicture = None;
	icon->maskPicture = None;
#endif
	icon->size = 0;
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
#ifdef USE_XRENDER
		if(icon->imagePicture != None) {
			JXRenderFreePicture(display, icon->imagePicture);
		}
		if(icon->maskPicture != None) {
			JXRenderFreePicture(display, icon->maskPicture);
		}
#endif
		if(icon->image != None) {
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
		index = GetHash(icon->name, icon->size);
		DoDestroyIcon(index, icon);
	}

}

/****************************************************************************
 ****************************************************************************/
void InsertIcon(IconNode *icon) {

	int index;

	Assert(icon);

	index = GetHash(icon->name, icon->size);

	icon->prev = NULL;
	if(iconHash[index]) {
		iconHash[index]->prev = icon;
	}
	icon->next = iconHash[index];
	iconHash[index] = icon;

}

/****************************************************************************
 ****************************************************************************/
IconNode *FindIcon(const char *name, int size) {

	IconNode *icon;
	int index;

	index = GetHash(name, size);

	icon = iconHash[index];
	while(icon) {
		if(AreEqual(icon->name, icon->size, name, size)) {
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
int GetHash(const char *str, int size) {

	int x;
	int hash = size;

	if(!str || !str[0]) {
		return hash % HASH_SIZE;
	}

	for(x = 0; str[x]; x++) {
		hash = (hash + str[x]) % HASH_SIZE;
	}

	return hash;

}

#endif /* USE_ICONS */

