/***************************************************************************
 ***************************************************************************/

#include "jwm.h"
#include "traybutton.h"
#include "tray.h"
#include "icon.h"
#include "image.h"
#include "error.h"
#include "root.h"
#include "main.h"
#include "color.h"
#include "font.h"
#include "button.h"
#include "misc.h"
#include "screen.h"
#include "desktop.h"
#include "popup.h"
#include "timing.h"

typedef struct TrayButtonType {

	TrayComponentType *cp;

	char *label;
	char *popup;
	char *iconName;
	IconNode *icon;

	char *action;

	int mousex;
	int mousey;
	TimeType mouseTime;

	struct TrayButtonType *next;

} TrayButtonType;

static TrayButtonType *buttons;

static void Create(TrayComponentType *cp);
static void Destroy(TrayComponentType *cp);
static void SetSize(TrayComponentType *cp, int width, int height);
static void Resize(TrayComponentType *cp);

static void ProcessButtonEvent(TrayComponentType *cp,
	int x, int y, int mask);
static void ProcessMotionEvent(TrayComponentType *cp,
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
	int north, south, east, west;

	GetButtonOffsets(&north, &south, &east, &west);

	for(bp = buttons; bp; bp = bp->next) {
		if(bp->label) {
			bp->cp->requestedWidth
				= GetStringWidth(FONT_TRAYBUTTON, bp->label) + 4;
			bp->cp->requestedHeight
				= GetStringHeight(FONT_TRAYBUTTON);
		} else {
			bp->cp->requestedWidth = 0;
			bp->cp->requestedHeight = 0;
		}
		if(bp->iconName) {
			bp->icon = LoadNamedIcon(bp->iconName);
			if(bp->icon) {
				bp->cp->requestedWidth += bp->icon->image->width;
				bp->cp->requestedHeight += bp->icon->image->height;
			} else {
				Warning("could not load tray icon: \"%s\"", bp->iconName);
			}
		}
		bp->cp->requestedWidth += east + west;
		bp->cp->requestedHeight += north + south;
	}

}

/***************************************************************************
 ***************************************************************************/
void ShutdownTrayButtons() {
}

/***************************************************************************
 ***************************************************************************/
void DestroyTrayButtons() {

	TrayButtonType *bp;

	while(buttons) {
		bp = buttons->next;
		if(buttons->label) {
			Release(buttons->label);
		}
		if(buttons->iconName) {
			Release(buttons->iconName);
		}
		if(buttons->action) {
			Release(buttons->action);
		}
		Release(buttons);
		buttons = bp;
	}

}

/***************************************************************************
 ***************************************************************************/
TrayComponentType *CreateTrayButton(const char *iconName,
	const char *label, const char *action,
	const char *popup, int width, int height) {

	TrayButtonType *bp;
	TrayComponentType *cp;

	if((label == NULL || strlen(label) == 0)
		&& (iconName == NULL || strlen(iconName) == 0)) {
		Warning("no icon or label for TrayButton");
		return NULL;
	}

	if(width < 0) {
		Warning("invalid TrayButton width: %d", width);
		width = 0;
	}
	if(height < 0) {
		Warning("invalid TrayButton height: %d", height);
		height = 0;
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

	if(popup) {
		bp->popup = Allocate(strlen(popup) + 1);
		strcpy(bp->popup, popup);
	} else {
		bp->popup = NULL;
	}

	cp = CreateTrayComponent();
	cp->object = bp;
	bp->cp = cp;
	cp->requestedWidth = width;
	cp->requestedHeight = height;

	cp->Create = Create;
	cp->Destroy = Destroy;
	cp->SetSize = SetSize;
	cp->Resize = Resize;

	cp->ProcessButtonEvent = ProcessButtonEvent;
	if(popup || label) {
		cp->ProcessMotionEvent = ProcessMotionEvent;
	}

	return cp;

}

/***************************************************************************
 ***************************************************************************/
void SetSize(TrayComponentType *cp, int width, int height) {

	TrayButtonType *bp;
	int labelWidth, labelHeight;
	int iconWidth, iconHeight;
	double ratio;
	int north, south, east, west;

	bp = (TrayButtonType*)cp->object;

	if(bp->icon) {

		if(bp->label) {
			labelWidth = GetStringWidth(FONT_TRAYBUTTON, bp->label);
			labelHeight = GetStringHeight(FONT_TRAYBUTTON);
		} else {
			labelWidth = 0;
			labelHeight = 0;
		}

		iconWidth = bp->icon->image->width;
		iconHeight = bp->icon->image->height;
		ratio = (double)iconWidth / iconHeight;

		GetButtonOffsets(&north, &south, &east, &west);

		if(width > 0) {

			/* Compute height from width. */
			iconWidth = width - labelWidth - east - west;
			iconHeight = iconWidth / ratio;
			height = Max(iconHeight, labelHeight) + north + south;

		} else if(height > 0) {

			/* Compute width from height. */
			iconHeight = height - north - south;
			iconWidth = iconHeight * ratio;
			width = iconWidth + labelWidth + east + west;

		}

		cp->width = width;
		cp->height = height;

	}

}

/***************************************************************************
 ***************************************************************************/
void Create(TrayComponentType *cp) {

	ButtonData button;
	TrayButtonType *bp;
	GC gc;

	bp = (TrayButtonType*)cp->object;

	cp->pixmap = JXCreatePixmap(display, rootWindow,
		cp->width, cp->height, rootDepth);
	gc = JXCreateGC(display, cp->pixmap, 0, NULL);

	JXSetForeground(display, gc, colors[COLOR_TRAYBUTTON_BG]);
	JXFillRectangle(display, cp->pixmap, gc, 0, 0, cp->width, cp->height);

	ResetButton(&button, cp->pixmap, gc);
	button.type = BUTTON_NORMAL;
	button.width = cp->width;
	button.height = cp->height;
	button.alignment = ALIGN_CENTER;
	button.x = 0;
	button.y = 0;
	button.icon = bp->icon;
	button.text = bp->label;

	DrawButton(&button);

	JXFreeGC(display, gc);

}

/***************************************************************************
 ***************************************************************************/
void Resize(TrayComponentType *cp) {
	Destroy(cp);
	Create(cp);
}

/***************************************************************************
 ***************************************************************************/
void Destroy(TrayComponentType *cp) {
	if(cp->pixmap != None) {
		JXFreePixmap(display, cp->pixmap);
	}
}

/***************************************************************************
 ***************************************************************************/
void ProcessButtonEvent(TrayComponentType *cp, int x, int y, int mask) {

	const ScreenType *sp;
	int mwidth, mheight;

	TrayButtonType *bp = (TrayButtonType*)cp->object;

	Assert(bp);

	if(bp->action && strlen(bp->action) > 0 && strcmp(bp->action, "root")) {
		if(!strncmp(bp->action, "exec:", 5)) {
			RunCommand(bp->action + 5);
		} else if(!strcmp(bp->action, "showdesktop")) {
			ShowDesktop();
		} else {
			Warning("invalid TrayButton action: \"%s\"", bp->action);
		}
	} else {

		GetRootMenuSize(&mwidth, &mheight);

		sp = GetCurrentScreen(cp->screenx, cp->screeny);

		if(cp->tray->layout == LAYOUT_HORIZONTAL) {
			x = cp->screenx;
			if(cp->screeny + cp->height / 2 < sp->y + sp->height / 2) {
				y = cp->screeny + cp->height;
			} else {
				y = cp->screeny - mheight;
			}
		} else {
			y = cp->screeny;
			if(cp->screenx + cp->width / 2 < sp->x + sp->width / 2) {
				x = cp->screenx + cp->width;
			} else {
				x = cp->screenx - mwidth;
			}
		}

		ShowRootMenu(x, y);

	}

}

/***************************************************************************
 ***************************************************************************/
void ProcessMotionEvent(TrayComponentType *cp, int x, int y, int mask) {

	TrayButtonType *bp = (TrayButtonType*)cp->object;

	bp->mousex = cp->screenx + x;
	bp->mousey = cp->screeny + y;
	GetCurrentTime(&bp->mouseTime);

}

/***************************************************************************
 ***************************************************************************/
void SignalTrayButton(TimeType *now, int x, int y) {

	TrayButtonType *bp;
	const char *popup;

	for(bp = buttons; bp; bp = bp->next) {
		if(bp->popup) {
			popup = bp->popup;
		} else if(bp->label) {
			popup = bp->label;
		} else {
			continue;
		}
		if(abs(bp->mousex - x) < POPUP_DELTA
			&& abs(bp->mousey - y) < POPUP_DELTA) {
			if(GetTimeDifference(now, &bp->mouseTime) >= popupDelay) {
				ShowPopup(x, y, popup);
			}
		}
	}

}


