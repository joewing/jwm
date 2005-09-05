/****************************************************************************
 * Functions to handle moving client windows.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"

typedef struct {
	int valid;
	int left, right;
	int top, bottom;
} RectangleType;

static int shouldStopMove;
static SnapModeType snapMode = SNAP_BORDER;
static int snapDistance = DEFAULT_SNAP_DISTANCE;

static MoveModeType moveMode = MOVE_OPAQUE;

static void StopMove(ClientNode *np, int doMove, int oldx, int oldy);
static void MoveController(int wasDestroyed);

static void DoSnap(ClientNode *np, int north);
static void DoSnapScreen(ClientNode *np, int north);
static void DoSnapBorder(ClientNode *np, int north);
static int ShouldSnap(const ClientNode *np);
static void GetClientRectangle(const ClientNode *np, RectangleType *r);

static int CheckOverlapTopBottom(const RectangleType *a,
	const RectangleType *b);
static int CheckOverlapLeftRight(const RectangleType *a,
	const RectangleType *b);

static int CheckLeftValid(const RectangleType *client,
	const RectangleType *other, const RectangleType *left);
static int CheckRightValid(const RectangleType *client,
	const RectangleType *other, const RectangleType *right);
static int CheckTopValid(const RectangleType *client,
	const RectangleType *other, const RectangleType *top);
static int CheckBottomValid(const RectangleType *client,
	const RectangleType *other, const RectangleType *bottom);

/****************************************************************************
 ****************************************************************************/
void SetSnapMode(SnapModeType mode) {
	snapMode = mode;
}

/****************************************************************************
 ****************************************************************************/
void SetMoveMode(MoveModeType mode) {
	moveMode = mode;
}

/****************************************************************************
 ****************************************************************************/
void SetSnapDistance(const char *value) {
	int temp;

	Assert(value);

	temp = atoi(value);
	if(temp > MAX_SNAP_DISTANCE || temp < MIN_SNAP_DISTANCE) {
		snapDistance = DEFAULT_SNAP_DISTANCE;
		Warning("invalid snap distance specified: %d", temp);
	} else {
		snapDistance = temp;
	}

}

/****************************************************************************
 ****************************************************************************/
void SetDefaultSnapDistance() {
	snapDistance = DEFAULT_SNAP_DISTANCE;
}

/****************************************************************************
 ****************************************************************************/
void MoveController(int wasDestroyed) {
	if(moveMode == MOVE_OUTLINE) {
		ClearOutline();
	}
	JXUngrabPointer(display, CurrentTime);
	JXUngrabKeyboard(display, CurrentTime);
	DestroyMoveWindow();
	shouldStopMove = 1;
}

/****************************************************************************
 ****************************************************************************/
int MoveClient(ClientNode *np, int startx, int starty) {

	XEvent event;
	int oldx, oldy;
	int doMove = 0;
	int north;
	int height;

	Assert(np);

	if(!(np->borderFlags & BORDER_OUTLINE)
		|| !(np->borderFlags & BORDER_MOVE)) {
		return 0;
	}

	if(!GrabMouseForMove()) {
		Debug("MoveClient: could not grab mouse");
		return 0;
	}

	np->controller = MoveController;
	shouldStopMove = 0;

	oldx = np->x;
	oldy = np->y;

	if(!(GetMouseMask() & (Button1Mask | Button2Mask))) {
		StopMove(np, 0, 0, 0);
		return 0;
	}

	if(np->borderFlags & BORDER_TITLE) {
		north = titleSize;
	} else {
		north = borderWidth;
	}
	startx += np->x - borderWidth;
	starty += np->y - north;

	for(;;) {

		WaitForEvent(&event);

		if(shouldStopMove) {
			np->controller = NULL;
			return doMove;
		}

		switch(event.type) {
		case ButtonRelease:
			if(event.xbutton.button == Button1
				|| event.xbutton.button == Button2) {
				StopMove(np, doMove, oldx, oldy);
				return doMove;
			}
			break;
		case MotionNotify:

			while(JXCheckTypedEvent(display, MotionNotify, &event));

			np->x = oldx + event.xmotion.x - startx;
			np->y = oldy + event.xmotion.y - starty;

			DoSnap(np, north);

			if(!doMove && (abs(np->x - oldx) > MOVE_DELTA
				|| abs(np->y - oldy) > MOVE_DELTA)) {
				CreateMoveWindow();
				doMove = 1;
			}

			if(doMove) {
				if(moveMode == MOVE_OUTLINE) {
					ClearOutline();
					height = north + borderWidth;
					if(!(np->statusFlags & STAT_SHADED)) {
						height += np->height;
					}
					DrawOutline(np->x - borderWidth, np->y - north,
						np->width + borderWidth * 2, height);
				} else {
					JXMoveWindow(display, np->parent, np->x - borderWidth,
						np->y - north);
					SendConfigureEvent(np);
				}
				UpdateMoveWindow(np->x, np->y);
				UpdatePager();
			}

			break;
		default:
			break;
		}
	}
}

/****************************************************************************
 ****************************************************************************/
int MoveClientKeyboard(ClientNode *np) {
	XEvent event;
	int oldx, oldy;
	int moved;
	int height;

	Assert(np);

	if(!(np->borderFlags & BORDER_OUTLINE)
		|| !(np->borderFlags & BORDER_MOVE)) {
		return 0;
	}

	if(JXGrabKeyboard(display, np->window, True, GrabModeAsync,
		GrabModeAsync, CurrentTime) != GrabSuccess) {
		Debug("could not grab keyboard for client move");
		return 0;
	}
	GrabMouseForMove();

	oldx = np->x;
	oldy = np->y;

	np->controller = MoveController;
	shouldStopMove = 0;

	CreateMoveWindow();
	UpdateMoveWindow(np->x, np->y);

	JXWarpPointer(display, None, rootWindow, 0, 0, 0, 0,
		np->x, np->y);
	JXCheckTypedEvent(display, MotionNotify, &event);

	if(np->statusFlags & STAT_SHADED) {
		height = 0;
	} else {
		height = np->height;
	}

	for(;;) {

		WaitForEvent(&event);

		if(shouldStopMove) {
			np->controller = NULL;
			return 1;
		}

		moved = 0;

		if(event.type == KeyPress) {

			while(JXCheckTypedEvent(display, KeyPress, &event));

			switch(GetKey(&event.xkey) & 0xFF) {
			case KEY_UP:
				if(np->y + height > 0) {
					np->y -= 10;
				}
				break;
			case KEY_DOWN:
				if(np->y < rootHeight) {
					np->y += 10;
				}
				break;
			case KEY_RIGHT:
				if(np->x < rootWidth) {
					np->x += 10;
				}
				break;
			case KEY_LEFT:
				if(np->x + np->width > 0) {
					np->x -= 10;
				}
				break;
			default:
				StopMove(np, 1, oldx, oldy);
				return 1;
			}

			JXWarpPointer(display, None, rootWindow, 0, 0, 0, 0,
				np->x, np->y);
			JXCheckTypedEvent(display, MotionNotify, &event);

			moved = 1;

		} else if(event.type == MotionNotify) {

			while(JXCheckTypedEvent(display, MotionNotify, &event));

			np->x = event.xmotion.x;
			np->y = event.xmotion.y;

			moved = 1;

		} else if(event.type == ButtonRelease) {

			StopMove(np, 1, oldx, oldy);
			return 1;

		}

		if(moved) {

			if(moveMode == MOVE_OUTLINE) {
				ClearOutline();
				if(np->borderFlags & BORDER_TITLE) {
					DrawOutline(np->x - borderWidth, np->y - titleSize,
						np->width + borderWidth * 2,
						height + titleSize + borderWidth);
				} else {
					DrawOutline(np->x - borderWidth, np->y - borderWidth,
						np->width + borderWidth * 2,
						height + borderWidth * 2);
				}
			} else {
				if(np->borderFlags & BORDER_TITLE) {
					JXMoveWindow(display, np->parent, np->x - borderWidth,
						np->y - titleSize);
				} else {
					JXMoveWindow(display, np->parent, np->x - borderWidth,
						np->y - borderWidth);
				}
				SendConfigureEvent(np);
			}

			UpdateMoveWindow(np->x, np->y);
			UpdatePager();

		}

	}

}

/****************************************************************************
 ****************************************************************************/
void StopMove(ClientNode *np, int doMove, int oldx, int oldy) {

	Assert(np);
	Assert(np->controller);

	(np->controller)(0);

	np->controller = NULL;

	if(!doMove) {
		np->x = oldx;
		np->y = oldy;
		return;
	}

	if(np->borderFlags & BORDER_TITLE) {
		JXMoveWindow(display, np->parent, np->x - borderWidth,
			np->y - titleSize);
	} else {
		JXMoveWindow(display, np->parent, np->x - borderWidth,
			np->y - borderWidth);
	}
	SendConfigureEvent(np);

}

/****************************************************************************
 ****************************************************************************/
void DoSnap(ClientNode *np, int north) {
	switch(snapMode) {
	case SNAP_BORDER:
		DoSnapBorder(np, north);
		DoSnapScreen(np, north);
		break;
	case SNAP_SCREEN:
		DoSnapScreen(np, north);
		break;
	default:
		break;
	}
}

/****************************************************************************
 ****************************************************************************/
void DoSnapScreen(ClientNode *np, int north) {

	RectangleType client;
	int height;
	int screen;
	int screenHeight;
	int screenWidth;
	int screenx;
	int screeny;

	screen = GetCurrentScreen(np->x, np->y);
	screenHeight = GetScreenHeight(screen);
	screenWidth = GetScreenWidth(screen);
	screenx = GetScreenX(screen);
	screeny = GetScreenY(screen);

	if(trayIsHidden) {
		height = screenHeight - 2;
	} else {
 		height = screenHeight - trayHeight;
	}

	GetClientRectangle(np, &client);

	if(abs(client.right - screenWidth) <= snapDistance) {
		np->x = screenWidth - borderWidth - np->width;
	}
	if(abs(client.left) <= snapDistance) {
		np->x = screenx + borderWidth;
	}
	if(abs(client.bottom - height) <= snapDistance) {
		np->y = height - borderWidth;
		if(!(np->statusFlags & STAT_SHADED)) {
			np->y -= np->height;
		}
	}
	if(abs(client.top) <= snapDistance) {
		np->y = north + screeny;
	}

}

/****************************************************************************
 ****************************************************************************/
void DoSnapBorder(ClientNode *np, int north) {
	const ClientNode *tp;
	RectangleType client, other;
	RectangleType left, right, top, bottom;
	int layer;

	left.valid = 0;
	right.valid = 0;
	top.valid = 0;
	bottom.valid = 0;

	GetClientRectangle(np, &client);

	/* Work from the bottom of the window stack to the top. */
	for(layer = 0; layer < LAYER_COUNT; layer++) {
		for(tp = nodeTail[layer]; tp; tp = tp->prev) {

			if(tp == np || !ShouldSnap(tp)) {
				continue;
			}

			GetClientRectangle(tp, &other);

			/* Check if this border invalidates any previous value. */
			left.valid = CheckLeftValid(&client, &other, &left);
			right.valid = CheckRightValid(&client, &other, &right);
			top.valid = CheckTopValid(&client, &other, &top);
			bottom.valid = CheckBottomValid(&client, &other, &bottom);

			/* Compute the new snap values. */
			if(CheckOverlapTopBottom(&client, &other)) {
				if(abs(client.left - other.right) <= snapDistance) {
					left = other;
				}
				if(abs(client.right - other.left) <= snapDistance) {
					right = other;
				}
			}
			if(CheckOverlapLeftRight(&client, &other)) {
				if(abs(client.top - other.bottom) <= snapDistance) {
					top = other;
				}
				if(abs(client.bottom - other.top) <= snapDistance) {
					bottom = other;
				}
			}

		}
	}

	if(right.valid) {
		np->x = right.left - np->width - borderWidth;
	}
	if(left.valid) {
		np->x = left.right + borderWidth;
	}
	if(bottom.valid) {
		np->y = bottom.top - borderWidth;
		if(!(np->statusFlags & STAT_SHADED)) {
			np->y -= np->height;
		}
	}
	if(top.valid) {
		np->y = top.bottom + north;
	}

}

/****************************************************************************
 ****************************************************************************/
int ShouldSnap(const ClientNode *np) {
	if(np->statusFlags & STAT_HIDDEN) {
		return 0;
	} else if(np->statusFlags & STAT_MINIMIZED) {
		return 0;
	} else {
		return 1;
	}
}

/****************************************************************************
 ****************************************************************************/
void GetClientRectangle(const ClientNode *np, RectangleType *r) {
	int border;

	r->left = np->x;
	r->right = np->x + np->width;
	r->top = np->y;
	if(np->statusFlags & STAT_SHADED) {
		r->bottom = np->y;
	} else {
		r->bottom = np->y + np->height;
	}

	if(np->borderFlags & BORDER_OUTLINE) {
		border = borderWidth;
		r->left -= border;
		r->right += border;
		r->bottom += border;
	} else {
		border = 0;
	}

	if(np->borderFlags & BORDER_TITLE) {
		r->top -= titleHeight + border;
	} else {
		r->top -= border;
	}

	r->valid = 1;

}

/****************************************************************************
 ****************************************************************************/
int CheckOverlapTopBottom(const RectangleType *a, const RectangleType *b) {
	if(a->top >= b->bottom) {
		return 0;
	} else if(a->bottom <= b->top) {
		return 0;
	} else {
		return 1;
	}
}

/****************************************************************************
 ****************************************************************************/
int CheckOverlapLeftRight(const RectangleType *a, const RectangleType *b) {
	if(a->left >= b->right) {
		return 0;
	} else if(a->right <= b->left) {
		return 0;
	} else {
		return 1;
	}
}

/****************************************************************************
 * Check if the current left snap position is valid.
 * client - The window being moved.
 * other  - A window higher in stacking order than previously check windows.
 * left   - The top/bottom of the current left snap window.
 * Returns 1 if the current left snap position is still valid, otherwise 0.
 ****************************************************************************/
int CheckLeftValid(const RectangleType *client,
	const RectangleType *other, const RectangleType *left) {

	if(!left->valid) {
		return 0;
	}

	if(left->right > other->right) {
		return 1;
	}

	/* If left and client go higher than other then still valid. */
	if(left->top < other->top && client->top < other->top) {
		return 1;
	}

	/* If left and client go lower than other then still valid. */
	if(left->bottom > other->bottom && client->bottom > other->bottom) {
		return 1;
	}

	if(other->left >= left->right) {
		return 1;
	}

	return 0;

}

/****************************************************************************
 ****************************************************************************/
int CheckRightValid(const RectangleType *client,
	const RectangleType *other, const RectangleType *right) {

	if(!right->valid) {
		return 0;
	}

	if(right->left < other->left) {
		return 1;
	}

	/* If right and client go higher than other then still valid. */
	if(right->top < other->top && client->top < other->top) {
		return 1;
	}

	/* If right and client go lower than other then still valid. */
	if(right->bottom > other->bottom && client->bottom > other->bottom) {
		return 1;
	}

	if(other->right <= right->left) {
		return 1;
	}

	return 0;

}

/****************************************************************************
 ****************************************************************************/
int CheckTopValid(const RectangleType *client,
	const RectangleType *other, const RectangleType *top) {

	if(!top->valid) {
		return 0;
	}

	if(top->bottom > other->bottom) {
		return 1;
	}

	/* If top and client are to the left of other then still valid. */
	if(top->left < other->left && client->left < other->left) {
		return 1;
	}

	/* If top and client are to the right of other then still valid. */
	if(top->right > other->right && client->right > other->right) {
		return 1;
	}

	if(other->top >= top->bottom) {
		return 1;
	}

	return 0;

}

/****************************************************************************
 ****************************************************************************/
int CheckBottomValid(const RectangleType *client,
	const RectangleType *other, const RectangleType *bottom) {

	if(!bottom->valid) {
		return 0;
	}

	if(bottom->top < other->top) {
		return 1;
	}

	/* If bottom and client are to the left of other then still valid. */
	if(bottom->left < other->left && client->left < other->left) {
		return 1;
	}

	/* If bottom and client are to the right of other then still valid. */
	if(bottom->right > other->right && client->right > other->right) {
		return 1;
	}

	if(other->bottom <= bottom->top) {
		return 1;
	}

	return 0;

}

