/****************************************************************************
 * Screen functions.
 * Copyright (C) 2005 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"
#include "screen.h"
#include "main.h"
#include "cursor.h"

static ScreenType *screens;
static int screenCount;

/****************************************************************************
 ****************************************************************************/
void InitializeScreens() {
	screens = NULL;
}

/****************************************************************************
 ****************************************************************************/
void StartupScreens() {
#ifdef USE_XINERAMA

	XineramaScreenInfo *info;
	int x;

	if(XineramaIsActive(display)) {

		info = XineramaQueryScreens(display, &screenCount);

		screens = Allocate(sizeof(ScreenType) * screenCount);
		for(x = 0; x < screenCount; x++) {
			screens[x].index = x;
			screens[x].x = info[x].x_org;
			screens[x].y = info[x].y_org;
			screens[x].width = info[x].width;
			screens[x].height = info[x].height;
		}

		JXFree(info);

	} else {

		screenCount = 1;
		screens = Allocate(sizeof(ScreenType));
		screens->index = 0;
		screens->x = 0;
		screens->y = 0;
		screens->width = rootWidth;
		screens->height = rootHeight;

	}

#else

	screenCount = 1;
	screens = Allocate(sizeof(ScreenType));
	screens->index = 0;
	screens->x = 0;
	screens->y = 0;
	screens->width = rootWidth;
	screens->height = rootHeight;

#endif /* USE_XINERAMA */
}

/****************************************************************************
 ****************************************************************************/
void ShutdownScreens() {
	if(screens) {
		Release(screens);
		screens = NULL;
	}
}

/****************************************************************************
 ****************************************************************************/
void DestroyScreens() {
}

/****************************************************************************
 ****************************************************************************/
const ScreenType *GetCurrentScreen(int x, int y) {

	ScreenType *sp;
	int index;

	for(index = 1; index < screenCount; index++) {
		sp = &screens[index];
		if(x >= sp->x && x < sp->x + sp->width) {
			if(y >= sp->y && y <= sp->y + sp->height) {
				return sp;
			}
		}
	}

	return &screens[0];

}

/****************************************************************************
 ****************************************************************************/
const ScreenType *GetMouseScreen() {
#ifdef USE_XINERAMA

	int x, y;

	GetMousePosition(&x, &y);
	return GetCurrentScreen(x, y);

#else

	return &screens[0];

#endif
}

/****************************************************************************
 ****************************************************************************/
const ScreenType *GetScreen(int index) {

	Assert(index >= 0);
	Assert(index < screenCount);

	return &screens[index];

}

/****************************************************************************
 ****************************************************************************/
int GetScreenCount() {

	return screenCount;

}


