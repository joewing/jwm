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
#include "button.h"

#define BUTTON_SIZE 4

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
			bp->cp->width = GetStringWidth(FONT_TRAYBUTTON, bp->label) + 4;
			bp->cp->height = GetStringHeight(FONT_TRAYBUTTON);
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
		bp->cp->width += 2 * BUTTON_SIZE;
		bp->cp->height += 2 * BUTTON_SIZE;
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

	JXSetForeground(display, gc, colors[COLOR_TRAYBUTTON_BG]);
	JXFillRectangle(display, cp->pixmap, gc, 0, 0, cp->width, cp->height);

	SetButtonDrawable(cp->pixmap, gc);
	SetButtonSize(cp->width - 3, cp->height - 3);
	DrawButton(1, 1, BUTTON_TASK, NULL);

	/* Compute the offset of the text. */
	if(bp->label) {
		labelx = cp->width - (GetStringWidth(FONT_TRAYBUTTON, bp->label) + 4);
	} else {
		labelx = cp->width;
	}
	labelx -= BUTTON_SIZE;

	if(bp->icon) {
		PutIcon(bp->icon, cp->pixmap, gc, BUTTON_SIZE, BUTTON_SIZE,
			labelx - BUTTON_SIZE, cp->height - BUTTON_SIZE * 2);
	}

	if(bp->label) {
		RenderString(cp->pixmap, gc, FONT_TRAYBUTTON, COLOR_TRAYBUTTON_FG,
			labelx + 2, cp->height / 2 - GetStringHeight(FONT_TRAYBUTTON) / 2,
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



