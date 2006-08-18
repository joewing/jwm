/****************************************************************************
 * Theme functions.
 * Copyright (C) 2006 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"
#include "theme.h"
#include "error.h"
#include "misc.h"

typedef struct ThemePathNode {
	char *path;
	struct ThemePathNode *next;
} ThemePathNode;

static ThemePathNode *themePaths;
static char *themeName;

/****************************************************************************
 ****************************************************************************/
void InitializeThemes() {

	themeName = NULL;
	themePaths = NULL;

}

/****************************************************************************
 ****************************************************************************/
void StartupThemes() {
}

/****************************************************************************
 ****************************************************************************/
void ShutdownThemes() {
}

/****************************************************************************
 ****************************************************************************/
void DestroyThemes() {

	ThemePathNode *tp;

	if(themeName) {
		Release(themeName);
	}

	while(themePaths) {
		tp = themePaths->next;
		Release(themePaths->path);
		Release(themePaths);
		themePaths = tp;
	}

}

/****************************************************************************
 ****************************************************************************/
void AddThemePath(const char *path) {

	ThemePathNode *tp;

	if(!path) {
		return;
	}

	Trim(path);

	tp = Allocate(sizeof(ThemePathNode));
	tp->path = CopyString(path);

	tp->next = themePaths;
	themePaths = tp;

}

/****************************************************************************
 ****************************************************************************/
void SetTheme(const char *name) {

	if(themeName) {
		Warning("theme set multiple times");
	}

	themeName = CopyString(name);
	Trim(themeName);

}

