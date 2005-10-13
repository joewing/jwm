/***************************************************************************
 ***************************************************************************/

#include "jwm.h"

typedef struct TrayButtonType {

	char *iconName;
	char *label;

} TrayButtonType;

static void Create(TrayComponentType *cp);
static void Destroy(TrayComponentType *cp);

static void ProcessButtonEvent(TrayComponentType *cp,
	int x, int y, int mask);

/***************************************************************************
 ***************************************************************************/
TrayComponentType *CreateTrayButton(const char *icon, const char *label) {

	TrayButtonType *bp;
	TrayComponentType *cp;

	bp = Allocate(sizeof(TrayButtonType));

	if(icon) {
		bp->iconName = Allocate(strlen(icon) + 1);
		strcpy(bp->iconName, icon);
	} else {
		bp->iconName = NULL;
	}

	if(label) {
		bp->label = Allocate(strlen(label) + 1);
		strcpy(bp->label, label);
	} else {
		bp->label = NULL;
	}

	cp = CreateTrayComponent();
	cp->object = bp;

	cp->Create = Create;
	cp->Destroy = Destroy;

	cp->ProcessButtonEvent = ProcessButtonEvent;

	return cp;

}

/***************************************************************************
 ***************************************************************************/
void Create(TrayComponentType *cp) {

}

/***************************************************************************
 ***************************************************************************/
void Destroy(TrayComponentType *cp) {

	TrayButtonType *bp = (TrayButtonType*)cp->object;

	Assert(bp);

	if(bp->iconName) {
		Release(bp->iconName);
	}
	if(bp->label) {
		Release(bp->label);
	}
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



