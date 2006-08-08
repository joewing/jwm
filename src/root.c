/***************************************************************************
 * Functions to handle the root menu.
 * Copyright (C) 2004 Joe Wingbermuehle
 ***************************************************************************/

#include "jwm.h"
#include "root.h"
#include "menu.h"
#include "client.h"
#include "main.h"
#include "error.h"
#include "confirm.h"
#include "desktop.h"
#include "misc.h"

/* Allow for menus 0 to 9. */
#define ROOT_MENU_COUNT 10

static MenuType *rootMenu[ROOT_MENU_COUNT];
static int showExitConfirmation = 1;

static void ExitHandler(ClientNode *np);
static void PatchRootMenu(MenuType *menu);
static void UnpatchRootMenu(MenuType *menu);

/***************************************************************************
 ***************************************************************************/
void InitializeRootMenu() {

	int x;

	for(x = 0; x < ROOT_MENU_COUNT; x++) {
		rootMenu[x] = NULL;
	}

}

/***************************************************************************
 ***************************************************************************/
void StartupRootMenu() {

	int x, y;
	int found;

	for(x = 0; x < ROOT_MENU_COUNT; x++) {
		if(rootMenu[x]) {
			found = 0;
			for(y = 0; y < x; y++) {
				if(rootMenu[y] == rootMenu[x]) {
					found = 1;
					break;
				}
			}
			if(!found) {
				InitializeMenu(rootMenu[x]);
			}
		}
	}

}

/***************************************************************************
 ***************************************************************************/
void ShutdownRootMenu() {
}

/***************************************************************************
 ***************************************************************************/
void DestroyRootMenu() {

	int x, y;

	for(x = 0; x < ROOT_MENU_COUNT; x++) {
		if(rootMenu[x]) {
			DestroyMenu(rootMenu[x]);
			for(y = x + 1; y < ROOT_MENU_COUNT; y++) {
				if(rootMenu[x] == rootMenu[y]) {
					rootMenu[y] = NULL;
				}
			}
			rootMenu[x] = NULL;
		}
	}

}

/***************************************************************************
 ***************************************************************************/
void SetRootMenu(const char *indexes, MenuType *m) {

	int x, y;
	int index;
	int found;

	/* Loop over each index to consider. */
	for(x = 0; indexes[x]; x++) {

		/* Get the index and make sure it's in range. */
		index = indexes[x] - '0';
		if(index < 0 || index >= ROOT_MENU_COUNT) {
			Warning("invalid root menu specified: \"%c\"", indexes[x]);
			continue;
		}

		if(rootMenu[index] && rootMenu[index] != m) {

			/* See if replacing this value will cause an orphan. */
			found = 0;
			for(y = 0; y < ROOT_MENU_COUNT; y++) {
				if(x != y && rootMenu[y] == rootMenu[x]) {
					found = 1;
					break;
				}
			}

			/* If we have an orphan, destroy it. */
			if(!found) {
				DestroyMenu(rootMenu[index]);
			}

		}

		rootMenu[index] = m;

	}

}

/***************************************************************************
 ***************************************************************************/
void SetShowExitConfirmation(int v) {
	showExitConfirmation = v;
}

/***************************************************************************
 ***************************************************************************/
void GetRootMenuSize(int index, int *width, int *height) {

	if(!rootMenu[index]) {
		*width = 0;
		*height = 0;
	}

	PatchRootMenu(rootMenu[index]);
	*width = rootMenu[index]->width;
	*height = rootMenu[index]->height;
	UnpatchRootMenu(rootMenu[index]);

}

/***************************************************************************
 ***************************************************************************/
int ShowRootMenu(int index, int x, int y) {

	if(!rootMenu[index]) {
		return 0;
	}

	PatchRootMenu(rootMenu[index]);
	ShowMenu(rootMenu[index], RunCommand, x, y);
	UnpatchRootMenu(rootMenu[index]);

	return 1;

}

/***************************************************************************
 ***************************************************************************/
void PatchRootMenu(MenuType *menu) {

	MenuItemType *item;

	for(item = menu->items; item; item = item->next) {
		if(item->submenu) {
			PatchRootMenu(item->submenu);
		}
		if(item->flags & MENU_ITEM_DESKTOPS) {
			item->submenu = CreateDesktopMenu(1 << currentDesktop);
			InitializeMenu(item->submenu);
		}
	}

}

/***************************************************************************
 ***************************************************************************/
void UnpatchRootMenu(MenuType *menu) {

	MenuItemType *item;

	for(item = menu->items; item; item = item->next) {
		if(item->flags & MENU_ITEM_DESKTOPS) {
			DestroyMenu(item->submenu);
			item->submenu = NULL;
		} else if(item->submenu) {
			UnpatchRootMenu(item->submenu);
		}
	}

}

/***************************************************************************
 ***************************************************************************/
void ExitHandler(ClientNode *np) {
	shouldExit = 1;
}

/***************************************************************************
 ***************************************************************************/
void Restart() {
	shouldRestart = 1;
	shouldExit = 1;
}

/***************************************************************************
 ***************************************************************************/
void Exit() {
	if(showExitConfirmation) {
		ShowConfirmDialog(NULL, ExitHandler,
			"Exit JWM",
			"Are you sure?",
			NULL);
	} else {
		ExitHandler(NULL);
	}
}

/***************************************************************************
 ***************************************************************************/
void RunCommand(const char *command) {
	char *displayString;
	char *str;

	if(!command) {
		return;
	}

	if(!strncmp(command, "#exit", 5)) {
		if(exitCommand) {
			Release(exitCommand);
			exitCommand = NULL;
		}
		if(strlen(command) > 6) {
			exitCommand = CopyString(command + 6);
		}
		Exit();
		return;
	} else if(!strcmp(command, "#restart")) {
		Restart();
		return;
	} else if(!strncmp(command, "#desk", 5)) {
		ChangeDesktop(command[5] - '0');
		return;
	}

	displayString = DisplayString(display);

	if(!fork()) {
		if(!fork()) {
			close(ConnectionNumber(display));
			if(displayString && displayString[0]) {
				str = malloc(strlen(displayString) + 9);
				sprintf(str, "DISPLAY=%s", displayString);
				putenv(str);
			}
			execl(SHELL_NAME, SHELL_NAME, "-c", command, NULL);
			Warning("exec failed: (%s) %s", SHELL_NAME, command);
			exit(1);
		}
		exit(0);
	}

	wait(NULL);

}

