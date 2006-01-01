/****************************************************************************
 * Client placement functions.
 * Copyright (C) 2005 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"
#include "place.h"
#include "client.h"
#include "screen.h"
#include "border.h"
#include "tray.h"
#include "main.h"

typedef struct BoundingBox {
	int x, y;
	int width, height;
} BoundingBox;

/* desktopCount x screenCount */
/* Note that we assume x and y are 0 based for all screens here. */
static int *cascadeOffsets = NULL;

static void GetScreenBounds(int index, BoundingBox *box);
static void UpdateTrayBounds(BoundingBox *box, unsigned int layer);
static void SubtractBounds(const BoundingBox *src, BoundingBox *dest);

/****************************************************************************
 ****************************************************************************/
void InitializePlacement() {
}

/****************************************************************************
 ****************************************************************************/
void StartupPlacement() {

	int count;
	int x;

	count = desktopCount * GetScreenCount();
	cascadeOffsets = Allocate(count * sizeof(int));

	for(x = 0; x < count; x++) {
		cascadeOffsets[x] = borderWidth + titleHeight;
	}

}

/****************************************************************************
 ****************************************************************************/
void ShutdownPlacement() {
	Release(cascadeOffsets);
}

/****************************************************************************
 ****************************************************************************/
void DestroyPlacement() {
}

/****************************************************************************
 ****************************************************************************/
void GetScreenBounds(int index, BoundingBox *box) {

	box->x = GetScreenX(index);
	box->y = GetScreenY(index);
	box->width = GetScreenWidth(index);
	box->height = GetScreenHeight(index);

}

/****************************************************************************
 * Shrink dest such that it does not intersect with src.
 ****************************************************************************/
void SubtractBounds(const BoundingBox *src, BoundingBox *dest) {

	BoundingBox boxes[4];

	if(src->x + src->width <= dest->x) {
		return;
	}
	if(src->y + src->height <= dest->y) {
		return;
	}
	if(dest->x + dest->width <= src->x) {
		return;
	}
	if(dest->y + dest->height <= src->y) {
		return;
	}

	/* There are four ways to do this:
	 *  1. Increase the x-coordinate and decrease the width of dest.
	 *  2. Increase the y-coordinate and decrease the height of dest.
	 *  3. Decrease the width of dest.
	 *  4. Decrease the height of dest.
	 * We will chose the option which leaves the greatest area.
	 * Note that negative areas are possible.
	 */

	/* 1 */
	boxes[0] = *dest;
	boxes[0].x = src->x + src->width;
	boxes[0].width = dest->x + dest->width - boxes[0].x;

	/* 2 */
	boxes[1] = *dest;
	boxes[1].y = src->y + src->height;
	boxes[1].height = dest->y + dest->height - boxes[1].y;

	/* 3 */
	boxes[2] = *dest;
	boxes[2].width = src->x - dest->x;

	/* 4 */
	boxes[3] = *dest;
	boxes[3].height = src->y - dest->y;

	/* 1 and 2, winner in 1. */
	if(boxes[0].width * boxes[0].height < boxes[1].width * boxes[1].height) {
		boxes[0] = boxes[1];
	}

	/* 3 and 4, winner in 3. */
	if(boxes[2].width * boxes[2].height < boxes[3].width * boxes[3].height) {
		boxes[2] = boxes[3];
	}

	/* 1 and 3, winner in dest. */
	if(boxes[0].width * boxes[0].height < boxes[2].width * boxes[2].height) {
		*dest = boxes[2];
	} else {
		*dest = boxes[0];
	}

}

/****************************************************************************
 ****************************************************************************/
void UpdateTrayBounds(BoundingBox *box, unsigned int layer) {

	TrayType *tp;
	BoundingBox src;
	BoundingBox last;

	for(tp = GetTrays(); tp; tp = tp->next) {

		if(tp->layer > layer) {

			src.x = tp->x;
			src.y = tp->y;
			src.width = tp->width;
			src.height = tp->height;

			last = *box;
			SubtractBounds(&src, box);
			if(box->width * box->height <= 0) {
				*box = last;
				break;
			}

		}

	}

}

/****************************************************************************
 ****************************************************************************/
void PlaceClient(ClientNode *np, int alreadyMapped) {

	BoundingBox box;
	int north, west;
	int screenIndex;
	int cascadeMultiplier;
	int cascadeIndex;
	int overflow;

	Assert(np);

	north = 0;
	west = 0;
	if(np->state.border & BORDER_OUTLINE) {
		north = borderWidth;
		west = borderWidth;
	}
	if(np->state.border & BORDER_TITLE) {
		north += titleHeight;
	}

	screenIndex = GetMouseScreen();

	GetScreenBounds(screenIndex, &box);

	if(alreadyMapped || (np->sizeFlags & (PPosition | USPosition))) {

		if(np->x + np->width - box.x > box.width) {
			np->x = box.x;
		}
		if(np->y + np->height - box.y > box.height) {
			np->y = box.y;
		}

		GravitateClient(np, 0);

	} else {

		UpdateTrayBounds(&box, np->state.layer);

		cascadeMultiplier = GetScreenCount() * desktopCount;
		cascadeIndex = screenIndex * cascadeMultiplier + currentDesktop;

		/* Set the cascaded location. */
		np->x = box.x + west + cascadeOffsets[cascadeIndex];
		np->y = box.y + north + cascadeOffsets[cascadeIndex];
		cascadeOffsets[cascadeIndex] += borderWidth + titleHeight;

		/* Check for cascade overflow. */
		overflow = 0;
		if(np->x + np->width - box.x > box.width) {
			overflow = 1;
		} else if(np->y + np->height - box.y > box.height) {
			overflow = 1;
		}

		if(overflow) {

			cascadeOffsets[cascadeIndex] = borderWidth + titleHeight;
			np->x = box.x + west + cascadeOffsets[cascadeIndex];
			np->y = box.y + north + cascadeOffsets[cascadeIndex];

			/* Check for client overflow. */
			overflow = 0;
			if(np->x + np->width - box.x > box.width) {
				overflow = 1;
			} else if(np->y + np->height - box.y > box.height) {
				overflow = 1;
			}

			/* Update cascade position or position client. */
			if(overflow) {
				np->x = box.x + west;
				np->y = box.y + north;
			} else {
				cascadeOffsets[cascadeIndex] += borderWidth + titleHeight;
			}

		}

	}

	JXMoveWindow(display, np->parent, np->x - west, np->y - north);

}

/****************************************************************************
 ****************************************************************************/
void PlaceMaximizedClient(ClientNode *np) {

	BoundingBox box;
	int screenIndex;
	int north, west;

	np->oldx = np->x;
	np->oldy = np->y;
	np->oldWidth = np->width;
	np->oldHeight = np->height;

	north = 0;
	west = 0;
	if(np->state.border & BORDER_OUTLINE) {
		north = borderWidth;
		west = borderWidth;
	}
	if(np->state.border & BORDER_TITLE) {
		north += titleHeight;
	}

	screenIndex = GetCurrentScreen(np->x, np->y);
	GetScreenBounds(screenIndex, &box);
	UpdateTrayBounds(&box, np->state.layer);

	box.x += west;
	box.y += north;
	box.width -= west + west;
	box.height -= north + west;

	if(box.width > np->maxWidth) {
		box.width = np->maxWidth;
	}
	if(box.height > np->maxHeight) {
		box.height = np->maxHeight;
	}

	if(np->sizeFlags & PAspect) {
		if((float)box.width / box.height
			< (float)np->aspect.minx / np->aspect.miny) {
			box.height = box.width * np->aspect.miny / np->aspect.minx;
		}
		if((float)box.width / box.height
			> (float)np->aspect.maxx / np->aspect.maxy) {
			box.width = box.height * np->aspect.maxx / np->aspect.maxy;
		}
	}

	np->x = box.x;
	np->y = box.y;
	np->width = box.width - (box.width % np->xinc);
	np->height = box.height - (box.height % np->yinc);

	np->state.status |= STAT_MAXIMIZED;

}

/****************************************************************************
 * Move the window in the specified direction for reparenting.
 ****************************************************************************/
void GravitateClient(ClientNode *np, int negate) {

	int north, south, west;
	int northDelta, westDelta;

	Assert(np);

	north = 0;
	west = 0;
	if(np->state.border & BORDER_OUTLINE) {
		north = borderWidth;
		west = borderWidth;
	}
	if(np->state.border & BORDER_TITLE) {
		north += titleHeight;
	}

	if(north) {
		south = borderWidth;
	} else {
		south = 0;
	}

	northDelta = 0;
	westDelta = 0;
	switch(np->gravity) {
	case NorthWestGravity:
		northDelta = -north;
		westDelta = -west;
		break;
	case NorthGravity:
		northDelta = -north;
		break;
	case NorthEastGravity:
		northDelta = -north;
		westDelta = west;
		break;
	case WestGravity:
		westDelta = -west;
		break;
	case CenterGravity:
		northDelta = (north + south) / 2;
		westDelta = west;
		break;
	case EastGravity:
		westDelta = west;
		break;
	case SouthWestGravity:
		northDelta = south;
		westDelta = -west;
		break;
	case SouthGravity:
		northDelta = south;
		break;
	case SouthEastGravity:
		northDelta = south;
		westDelta = west;
		break;
	default: /* Static */
		break;
	}

	if(negate) {
		np->x += westDelta;
		np->y += northDelta;
	} else {
		np->x -= westDelta;
		np->y -= northDelta;
	}

}

