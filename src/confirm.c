/***************************************************************************
 * The confirm dialog functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ***************************************************************************/

#include "jwm.h"
#include "confirm.h"
#include "client.h"
#include "main.h"
#include "font.h"
#include "button.h"
#include "screen.h"
#include "color.h"

#ifndef DISABLE_CONFIRM

typedef struct DialogType {

	int x, y;
	int width, height;
	int lineHeight;

	int okx;
	int cancelx;
	int buttony;
	int buttonWidth, buttonHeight;

	int lineCount;
	char **message;

	ClientNode *node;
	GC gc;

	void (*action)(ClientNode*);
	ClientNode *client;

	struct DialogType *prev;
	struct DialogType *next;

} DialogType;

static const char *OK_STRING = "Ok";
static const char *CANCEL_STRING = "Cancel";

static DialogType *dialogList = NULL;

static int minWidth = 0;

static void DrawConfirmDialog(DialogType *d);
static void DestroyConfirmDialog(DialogType *d);
static void ComputeDimensions(DialogType *d);
static void DrawMessage(DialogType *d);
static void DrawButtons(DialogType *d);
static DialogType *FindDialogByWindow(Window w);
static int HandleDialogExpose(const XExposeEvent *event); 
static int HandleDialogButtonRelease(const XButtonEvent *event);

/***************************************************************************
 ***************************************************************************/
void InitializeDialogs() {
}

/***************************************************************************
 ***************************************************************************/
void StartupDialogs() {
}

/***************************************************************************
 ***************************************************************************/
void ShutdownDialogs() {
}

/***************************************************************************
 ***************************************************************************/
void DestroyDialogs() {
}

/***************************************************************************
 ***************************************************************************/
int ProcessDialogEvent(const XEvent *event) {
	int handled = 0;

	switch(event->type) {
	case Expose:
		return HandleDialogExpose(&event->xexpose);
	case ButtonRelease:
		return HandleDialogButtonRelease(&event->xbutton);
	default:
		break;
	}

	return handled;
}

/***************************************************************************
 ***************************************************************************/
int HandleDialogExpose(const XExposeEvent *event) {
	DialogType *dp;

	dp = FindDialogByWindow(event->window);
	if(dp) {
		DrawConfirmDialog(dp);
		return 1;
	} else {
		return 0;
	}
}

/***************************************************************************
 ***************************************************************************/
int HandleDialogButtonRelease(const XButtonEvent *event) {
	DialogType *dp;
	int x, y;
	int cancelPressed, okPressed;

	dp = FindDialogByWindow(event->window);
	if(dp) {
		cancelPressed = 0;
		okPressed = 0;
		y = event->y;
		if(y >= dp->buttony && y < dp->buttony + dp->buttonHeight) {
			x = event->x;
			if(x >= dp->okx && x < dp->okx + dp->buttonWidth) {
				okPressed = 1;
			} else if(x >= dp->cancelx && x < dp->cancelx + dp->buttonWidth) {
				cancelPressed = 1;
			}
		}

		if(okPressed) {
			(dp->action)(dp->client);
		}

		if(cancelPressed || okPressed) {
			DestroyConfirmDialog(dp);
		}

		return 1;
	} else {
		return 0;
	}
}

/***************************************************************************
 ***************************************************************************/
DialogType *FindDialogByWindow(Window w) {
	DialogType *dp;

	for(dp = dialogList; dp; dp = dp->next) {
		if(dp->node->window == w || dp->node->parent == w) {
			return dp;
		}
	}

	return NULL;

}

/***************************************************************************
 ***************************************************************************/
void ShowConfirmDialog(ClientNode *np, void (*action)(ClientNode*), ...) {
	va_list ap;
	DialogType *dp;
	XSetWindowAttributes attrs;
	XSizeHints shints;
	Window window;
	char *str;
	int x;

	dp = Allocate(sizeof(DialogType));
	dp->client = np;
	dp->action = action;

	dp->prev = NULL;
	dp->next = dialogList;
	if(dialogList) {
		dialogList->prev = dp;
	}
	dialogList = dp;

	/* Get the number of lines. */
	va_start(ap, action);
	for(dp->lineCount = 0; va_arg(ap, char*); dp->lineCount++);
	va_end(ap);

	dp->message = Allocate(dp->lineCount * sizeof(char*));
	va_start(ap, action);
	for(x = 0; x < dp->lineCount; x++) {
		str = va_arg(ap, char*);
		Assert(str);
		dp->message[x] = Allocate(strlen(str) + 1);
		strcpy(dp->message[x], str);
	}
	va_end(ap);

	ComputeDimensions(dp);

	attrs.background_pixel = colors[COLOR_MENU_BG];
	attrs.event_mask = ButtonReleaseMask | ExposureMask;

	window = JXCreateWindow(display, rootWindow,
		dp->x, dp->y, dp->width, dp->height, 0,
		CopyFromParent, InputOutput, CopyFromParent,
		CWBackPixel | CWEventMask, &attrs);

	shints.x = dp->x;
	shints.y = dp->y;
	shints.flags = PPosition;
	JXSetWMNormalHints(display, window, &shints);

	JXStoreName(display, window, "Confirm");

	dp->node = AddClientWindow(window, 0, 0);
	Assert(dp->node);
	if(np) {
		dp->node->owner = np->window;
	}
	dp->node->state.status |= STAT_WMDIALOG;
	FocusClient(dp->node);

	dp->gc = JXCreateGC(display, window, 0, NULL);

	DrawConfirmDialog(dp);

	JXGrabButton(display, AnyButton, AnyModifier, window,
		True, ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);

}

/***************************************************************************
 ***************************************************************************/
void DrawConfirmDialog(DialogType *dp) {

	DrawMessage(dp);
	DrawButtons(dp);

}

/***************************************************************************
 ***************************************************************************/
/*
void DestroyAllDialogs() {

	while(dialogList) {
		DestroyConfirmDialog(dialogList);
	}

}
*/

/***************************************************************************
 ***************************************************************************/
void DestroyConfirmDialog(DialogType *dp) {
	int x;

	/* This will take care of destroying the dialog window since
	 * its parent will be destroyed. */
	RemoveClient(dp->node);

	for(x = 0; x < dp->lineCount; x++) {
		Release(dp->message[x]);
	}
	Release(dp->message);

	if(dp->next) {
		dp->next->prev = dp->prev;
	}
	if(dp->prev) {
		dp->prev->next = dp->next;
	} else {
		dialogList = dp->next;
	}
	Release(dp);

}

/***************************************************************************
 ***************************************************************************/
void ComputeDimensions(DialogType *dp) {
	int width;
	int x;

	if(!minWidth) {
		minWidth = GetStringWidth(FONT_MENU, CANCEL_STRING) * 3;
		width = GetStringWidth(FONT_MENU, OK_STRING) * 3;
		if(width > minWidth) {
			minWidth = width;
		}
		minWidth += 30;
	}
	dp->width = minWidth;

	for(x = 0; x < dp->lineCount; x++) {
		width = GetStringWidth(FONT_MENU, dp->message[x]);
		if(width > dp->width) {
			dp->width = width;
		}
	}
	dp->lineHeight = GetStringHeight(FONT_MENU);
	dp->width += 8;
	dp->height = (dp->lineCount + 2) * dp->lineHeight;

	if(dp->client) {

		dp->x = dp->client->x + dp->client->width / 2 - dp->width / 2;
		dp->y = dp->client->y + dp->client->height / 2 - dp->height / 2;

		if(dp->x < 0) {
			dp->x = 0;
		}
		if(dp->y < 0) {
			dp->y = 0;
		}
		if(dp->x + dp->width >= rootWidth) {
			dp->x = rootWidth - dp->width - (borderWidth * 2);
		}
		if(dp->y + dp->height >= rootHeight) {
			dp->y = rootHeight - dp->height - (borderWidth * 2 + titleHeight);
		}

	} else {

		x = GetMouseScreen();

		dp->x = GetScreenWidth(x) / 2 - dp->width / 2 + GetScreenX(x);
		dp->y = GetScreenHeight(x) / 2 - dp->height / 2 + GetScreenY(x);

	}

}

/***************************************************************************
 ***************************************************************************/
void DrawMessage(DialogType *dp) {
	int yoffset;
	int x;

	yoffset = 4;
	for(x = 0; x < dp->lineCount; x++) {
		RenderString(dp->node->window, dp->gc, FONT_MENU, COLOR_MENU_FG,
			4, yoffset, dp->width, dp->message[x]);
		yoffset += dp->lineHeight;
	}

}

/***************************************************************************
 ***************************************************************************/
void DrawButtons(DialogType *dp) {
	int temp;

	dp->buttonWidth = GetStringWidth(FONT_MENU, CANCEL_STRING);
	temp = GetStringWidth(FONT_MENU, OK_STRING);
	if(temp > dp->buttonWidth) {
		dp->buttonWidth = temp;
	}
	dp->buttonWidth += 8;
	dp->buttonHeight = dp->lineHeight + 4;

	SetButtonDrawable(dp->node->window, dp->gc);
	SetButtonFont(FONT_MENU);
	SetButtonSize(dp->buttonWidth, dp->buttonHeight);
	SetButtonAlignment(ALIGN_CENTER);

	dp->okx = dp->width / 3 - dp->buttonWidth / 2;
	dp->cancelx = 2 * dp->width / 3 - dp->buttonWidth / 2;
	dp->buttony = dp->height - dp->lineHeight - dp->lineHeight / 2;

	DrawButton(dp->okx, dp->buttony, BUTTON_MENU, OK_STRING);
	DrawButton(dp->cancelx, dp->buttony, BUTTON_MENU, CANCEL_STRING);

}

#else /* DISABLE_CONFIRM */

/***************************************************************************
 ***************************************************************************/
int ProcessDialogEvent(const XEvent *event) {
	return 0;
}

/***************************************************************************
 ***************************************************************************/
void ShowConfirmDialog(ClientNode *np, void (*action)(ClientNode*), ...) {
	(action)(np);
}

#endif /* DISABLE_CONFIRM */



