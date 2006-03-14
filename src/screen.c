/****************************************************************************
 * Screen functions.
 * Copyright (C) 2005 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"
#include "screen.h"
#include "main.h"
#include "cursor.h"

#ifdef USE_XINERAMA
static XineramaScreenInfo *screens = NULL;
static int screenCount = 0;
#endif

/****************************************************************************
 ****************************************************************************/
void InitializeScreens() {
}

/****************************************************************************
 ****************************************************************************/
void StartupScreens() {
#ifdef USE_XINERAMA

	if(XineramaIsActive(display)) {
		screens = XineramaQueryScreens(display, &screenCount);
	} else {
		screenCount = 1;
	}

#endif /* USE_XINERAMA */
}

/****************************************************************************
 ****************************************************************************/
void ShutdownScreens() {
#ifdef USE_XINERAMA
	if(screens) {
		JXFree(screens);
		screens = NULL;
	}
#endif
}

/****************************************************************************
 ****************************************************************************/
void DestroyScreens() {
}

/****************************************************************************
 ****************************************************************************/
int GetCurrentScreen(int x, int y) {
#ifdef USE_XINERAMA

	int s;
	XineramaScreenInfo *info;

	for(s = 1; s < screenCount; s++) {
		info = &screens[s];
		if(x >= info->x_org && x < info->x_org + info->width) {
			if(y >= info->y_org && y < info->y_org + info->height) {
				return s;
			}
		}
	}

	return 0;

#else

	return 0;

#endif
}

/****************************************************************************
 ****************************************************************************/
int GetMouseScreen() {
#ifdef USE_XINERAMA

	int x, y;

	GetMousePosition(&x, &y);
	return GetCurrentScreen(x, y);

#else
	return 0;
#endif
}

/****************************************************************************
 ****************************************************************************/
int GetScreenWidth(int index) {
#ifdef USE_XINERAMA
	if(screens) {
		return screens[index].width;
	} else {
		return rootWidth;
	}
#else
	return rootWidth;
#endif
}

/****************************************************************************
 ****************************************************************************/
int GetScreenHeight(int index) {
#ifdef USE_XINERAMA
	if(screens) {
		return screens[index].height;
	} else {
		return rootHeight;
	}
#else
	return rootHeight;
#endif
}

/****************************************************************************
 ****************************************************************************/
int GetScreenX(int index) {
#ifdef USE_XINERAMA
	if(screens) {
		return screens[index].x_org;
	} else {
		return 0;
	}
#else
	return 0;
#endif
}

/****************************************************************************
 ****************************************************************************/
int GetScreenY(int index) {
#ifdef USE_XINERAMA
	if(screens) {
		return screens[index].y_org;
	} else {
		return 0;
	}
#else
	return 0;
#endif
}

/****************************************************************************
 ****************************************************************************/
int GetScreenCount() {
#ifdef USE_XINERAMA
	return screenCount;
#else
	return 1;
#endif
}


