/***************************************************************************
 ***************************************************************************/

#include "jwm.h"

typedef struct TrayButtonType {

	TrayComponentType *cp;

	char *iconName;
	IconNode *icon;

	struct TrayButtonType *next;

} TrayButtonType;

static TrayButtonType *buttons;

static void Create(TrayComponentType *cp);
static void Destroy(TrayComponentType *cp);

static void ProcessButtonEvent(TrayComponentType *cp,
	int x, int y, int mask);

/***************************************************************************
 ***************************************************************************/
void InitializeTrayButtons() {
	buttons = NULL;
}

/***************************************************************************
 ***************************************************************************/
void StartupTrayButtons() {

	TrayButtonType *bp;

	for(bp = buttons; bp; bp = bp->next) {
		bp->icon = LoadNamedIcon(bp->iconName);
		if(bp->icon) {
			bp->cp->width = bp->icon->image->width;
			bp->cp->height = bp->icon->image->height;
		} else {
			Warning("could not load tray icon: \"%s\"", bp->iconName);
			bp->cp->width = 1;
			bp->cp->height = 1;
		}
	}

}

/***************************************************************************
 ***************************************************************************/
void ShutdownTrayButtons() {
}

/***************************************************************************
 ***************************************************************************/
void DestroyTrayButtons() {
}

/***************************************************************************
 ***************************************************************************/
TrayComponentType *CreateTrayButton(const char *iconName) {

	TrayButtonType *bp;
	TrayComponentType *cp;

	if(iconName == NULL || strlen(iconName) == 0) {
		Warning("no icon for TrayButton");
		return NULL;
	}

	bp = Allocate(sizeof(TrayButtonType));
	bp->next = buttons;
	buttons = bp;

	bp->icon = NULL;
	bp->iconName = Allocate(strlen(iconName) + 1);
	strcpy(bp->iconName, iconName);

	cp = CreateTrayComponent();
	cp->object = bp;
	bp->cp = cp;

	cp->Create = Create;
	cp->Destroy = Destroy;

	cp->ProcessButtonEvent = ProcessButtonEvent;

	return cp;

}

/***************************************************************************
 ***************************************************************************/
void Create(TrayComponentType *cp) {

	TrayButtonType *bp;
	GC gc;

	bp = (TrayButtonType*)cp->object;

	if(bp->icon) {

		cp->pixmap = JXCreatePixmap(display, rootWindow,
			cp->width, cp->height, rootDepth);
		gc = JXCreateGC(display, cp->pixmap, 0, NULL);

		JXSetForeground(display, gc, colors[COLOR_TRAY_BG]);
		JXFillRectangle(display, cp->pixmap, gc, 0, 0, cp->width, cp->height);

		PutIcon(bp->icon, cp->pixmap, gc, 0, 0, cp->width);

		JXFreeGC(display, gc);

	}

}

/***************************************************************************
 ***************************************************************************/
void Destroy(TrayComponentType *cp) {

	TrayButtonType *bp = (TrayButtonType*)cp->object;

	Assert(bp);

	Release(bp->iconName);

	if(cp->pixmap != None) {
		JXFreePixmap(display, cp->pixmap);
	}

	Release(bp);

}

/***************************************************************************
 ***************************************************************************/
void ProcessButtonEvent(TrayComponentType *cp, int x, int y, int mask) {
	GetMousePosition(&x, &y);
	ShowRootMenu(x, y);
}



