/***************************************************************************
 ***************************************************************************/

#include "jwm.h"
#include "traybutton.h"
#include "tray.h"
#include "icon.h"
#include "image.h"
#include "error.h"
#include "root.h"
#include "cursor.h"
#include "main.h"
#include "color.h"
#include "font.h"

typedef struct TrayButtonType {

	TrayComponentType *cp;

	char *label;
	char *iconName;
	IconNode *icon;

	char *action;

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
		if(bp->label) {
			bp->cp->width = GetStringWidth(FONT_TASK, bp->label) + 4;
			bp->cp->height = GetStringHeight(FONT_TASK);
		} else {
			bp->cp->width = 0;
			bp->cp->height = 0;
		}
		if(bp->iconName) {
			bp->icon = LoadNamedIcon(bp->iconName);
			if(bp->icon) {
				bp->cp->width += bp->icon->image->width;
				bp->cp->height += bp->icon->image->height;
			} else {
				Warning("could not load tray icon: \"%s\"", bp->iconName);
			}
		}
		if(bp->cp->width == 0) {
			bp->cp->width = 1;
		}
		if(bp->cp->height == 0) {
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
TrayComponentType *CreateTrayButton(const char *iconName,
	const char *label, const char *action) {

	TrayButtonType *bp;
	TrayComponentType *cp;

	if((label == NULL || strlen(label) == 0)
		&& (iconName == NULL || strlen(iconName) == 0)) {
		Warning("no icon or label for TrayButton");
		return NULL;
	}

	bp = Allocate(sizeof(TrayButtonType));
	bp->next = buttons;
	buttons = bp;

	bp->icon = NULL;
	if(iconName) {
		bp->iconName = Allocate(strlen(iconName) + 1);
		strcpy(bp->iconName, iconName);
	} else {
		bp->iconName = NULL;
	}

	if(label) {
		bp->label = Allocate(strlen(label) + 1);
		strcpy(bp->label, label);
	} else {
		bp->label = NULL;
	}

	if(action) {
		bp->action = Allocate(strlen(action) + 1);
		strcpy(bp->action, action);
	} else {
		bp->action = NULL;
	}

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
	int labelx;

	bp = (TrayButtonType*)cp->object;

	cp->pixmap = JXCreatePixmap(display, rootWindow,
		cp->width, cp->height, rootDepth);
	gc = JXCreateGC(display, cp->pixmap, 0, NULL);

	JXSetForeground(display, gc, colors[COLOR_TRAY_BG]);
	JXFillRectangle(display, cp->pixmap, gc, 0, 0, cp->width, cp->height);

	if(bp->label) {
		labelx = cp->width - (GetStringWidth(FONT_TASK, bp->label) + 4);
	} else {
		labelx = cp->width;
	}

	if(bp->icon) {
		PutIcon(bp->icon, cp->pixmap, gc, 0, 0, labelx, cp->height);
	}

	if(bp->label) {
		RenderString(cp->pixmap, gc, FONT_TASK, COLOR_TASK_FG,
			labelx + 2, cp->height / 2 - GetStringHeight(FONT_TASK) / 2,
			cp->width - labelx, bp->label);
	}

	JXFreeGC(display, gc);

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

	if(bp->action) {
		Release(bp->action);
	}

	if(cp->pixmap != None) {
		JXFreePixmap(display, cp->pixmap);
	}

	Release(bp);

}

/***************************************************************************
 ***************************************************************************/
void ProcessButtonEvent(TrayComponentType *cp, int x, int y, int mask) {

	TrayButtonType *bp = (TrayButtonType*)cp->object;

	Assert(bp);

	if(bp->action && strlen(bp->action) > 0) {
		RunCommand(bp->action);
	} else {
		GetMousePosition(&x, &y);
		ShowRootMenu(x, y);
	}

}



