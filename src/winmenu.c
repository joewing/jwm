/****************************************************************************
 * Functions for handling window menus.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"
#include "winmenu.h"
#include "client.h"
#include "menu.h"
#include "main.h"
#include "desktop.h"
#include "move.h"
#include "resize.h"

static const char *SENDTO_TEXT = "Send To";
static const char *LAYER_TEXT = "Layer";

static MenuType *CreateWindowMenu();
static void RunWindowCommand(const char *command);

static void CreateWindowSendToMenu(MenuType *menu);
static void CreateWindowLayerMenu(MenuType *menu);
static void AddWindowMenuItem(MenuType *menu, const char *name,
	const char *command);

static ClientNode *client = NULL;

/****************************************************************************
 ****************************************************************************/
void ShowWindowMenu(ClientNode *np, int x, int y) {
	MenuType *menu;

	client = np;
	menu = CreateWindowMenu();

	InitializeMenu(menu);

	ShowMenu(menu, RunWindowCommand, x, y);

	DestroyMenu(menu);

}

/****************************************************************************
 ****************************************************************************/
MenuType *CreateWindowMenu() {
	MenuType *menu = Allocate(sizeof(MenuType));
	menu->itemHeight = 0;
	menu->items = NULL;
	menu->label = NULL;

	/* Note that items are added in reverse order of display. */

	if(!(client->state.status & STAT_WMDIALOG)) {
		AddWindowMenuItem(menu, "Close", "close");
		AddWindowMenuItem(menu, "Kill", "kill");
	}

	AddWindowMenuItem(menu, NULL, NULL);

	if(client->state.status & (STAT_MAPPED | STAT_SHADED)) {
		if(client->state.border & BORDER_RESIZE) {
			AddWindowMenuItem(menu, "Resize", "resize");
		}
		if(client->state.border & BORDER_MOVE) {
			AddWindowMenuItem(menu, "Move", "move");
		}
	}

	if(client->state.border & BORDER_MIN) {

		if(client->state.status & STAT_MINIMIZED) {
			AddWindowMenuItem(menu, "Restore", "restore");
		} else {
			if(client->state.status & STAT_SHADED) {
				AddWindowMenuItem(menu, "Unshade", "unshade");
			} else {
				AddWindowMenuItem(menu, "Shade", "shade");
			}
			AddWindowMenuItem(menu, "Minimize", "minimize");
		}

	}

	if((client->state.border & BORDER_MAX)
		&& (client->state.status & STAT_MAPPED)) {

		AddWindowMenuItem(menu, "Maximize", "maximize");
	}

	if(!(client->state.status & STAT_WMDIALOG)) {

		if(client->state.status & STAT_STICKY) {
			AddWindowMenuItem(menu, "Unstick", "unstick");
		} else {
			AddWindowMenuItem(menu, "Stick", "stick");
		}

		CreateWindowLayerMenu(menu);
		CreateWindowSendToMenu(menu);

	}

	return menu;
}

/****************************************************************************
 ****************************************************************************/
void CreateWindowLayerMenu(MenuType *menu) {
	MenuType *submenu;
	MenuItemType *item;
	char str[10];
	char command[10];
	int x;

	item = Allocate(sizeof(MenuItemType));
	item->iconName = NULL;
	item->next = menu->items;
	menu->items = item;

	item->name = Allocate(strlen(LAYER_TEXT) + 1);
	strcpy(item->name, LAYER_TEXT);
	item->command = NULL;

	submenu = Allocate(sizeof(MenuType));
	item->submenu = submenu;
	submenu->itemHeight = 0;
	submenu->items = NULL;
	submenu->label = NULL;

	strcpy(command, "layer  ");
	command[5] = (LAYER_TOP / 10) + '0';
	command[6] = (LAYER_TOP % 10) + '0';

	if(client->state.layer == LAYER_TOP) {
		AddWindowMenuItem(submenu, "[Top]", command);
	} else {
		AddWindowMenuItem(submenu, "Top", command);
	}

	str[4] = 0;
	for(x = LAYER_TOP - 1; x > LAYER_BOTTOM; x--) {
		command[5] = (x / 10) + '0';
		command[6] = (x % 10) + '0';
		if(x == LAYER_NORMAL) {
			if(client->state.layer == x) {
				AddWindowMenuItem(submenu, "[Normal]", command);
			} else {
				AddWindowMenuItem(submenu, "Normal", command);
			}
		} else {
			if(client->state.layer == x) {
				str[0] = '[';
				str[3] = ']';
			} else {
				str[0] = ' ';
				str[3] = ' ';
			}
			if(x < 10) {
				str[1] = ' ';
			} else {
				str[1] = (x / 10) + '0';
			}
			str[2] = (x % 10) + '0';
			AddWindowMenuItem(submenu, str, command);
		}
	}

	command[5] = (LAYER_BOTTOM / 10) + '0';
	command[6] = (LAYER_BOTTOM % 10) + '0';
	if(client->state.layer == LAYER_BOTTOM) {
		AddWindowMenuItem(submenu, "[Bottom]", command);
	} else {
		AddWindowMenuItem(submenu, "Bottom", command);
	}

}

/****************************************************************************
 ****************************************************************************/
void CreateWindowSendToMenu(MenuType *menu) {
	MenuType *submenu;
	MenuItemType *item;
	char str[4];
	char command[10];
	int x;

	item = Allocate(sizeof(MenuItemType));
	item->iconName = NULL;
	item->next = menu->items;
	menu->items = item;

	item->name = Allocate(strlen(SENDTO_TEXT) + 1);
	strcpy(item->name, SENDTO_TEXT);
	item->command = NULL;

	submenu = Allocate(sizeof(MenuType));
	item->submenu = submenu;
	submenu->itemHeight = 0;
	submenu->items = NULL;
	submenu->label = NULL;

	strcpy(command, "send ");

	str[3] = 0;
	for(x = desktopCount - 1; x >= 0; x--) {
		if(client->state.desktop == x
			|| (client->state.status & STAT_STICKY)) {
			str[0] = '[';
			str[2] = ']';
		} else {
			str[0] = ' ';
			str[2] = ' ';
		}
		str[1] = '1' + x;
		command[4] = '0' + x;
		AddWindowMenuItem(submenu, str, command);
	}

}

/****************************************************************************
 ****************************************************************************/
void AddWindowMenuItem(MenuType *menu, const char *name,
	const char *command) {

	MenuItemType *item;

	item = Allocate(sizeof(MenuItemType));
	item->iconName = NULL;
	item->submenu = NULL;
	item->next = menu->items;
	menu->items = item;
	if(name) {
		item->name = Allocate(strlen(name) + 1);
		strcpy(item->name, name);
	} else {
		item->name = NULL;
	}
	if(command) {
		item->command = Allocate(strlen(command) + 1);
		strcpy(item->command, command);
	} else {
		item->command = NULL;
	}

}

/****************************************************************************
 ****************************************************************************/
void RunWindowCommand(const char *command) {
	int x;

	if(!command) {
		return;
	}

	if(!strcmp(command, "stick")) {
		SetClientSticky(client, 1);
	} else if(!strcmp(command, "unstick")) {
		SetClientSticky(client, 0);
	} else if(!strcmp(command, "maximize")) {
		MaximizeClient(client);
	} else if(!strcmp(command, "minimize")) {
		MinimizeClient(client);
	} else if(!strcmp(command, "restore")) {
		RestoreClient(client);
	} else if(!strcmp(command, "close")) {
		DeleteClient(client);
	} else if(!strncmp(command, "send", 4)) {
		x = command[4] - '0';
		SetClientDesktop(client, x);
	} else if(!strcmp(command, "shade")) {
		ShadeClient(client);
	} else if(!strcmp(command, "unshade")) {
		UnshadeClient(client);
	} else if(!strcmp(command, "move")) {
		MoveClientKeyboard(client);
	} else if(!strcmp(command, "resize")) {
		ResizeClientKeyboard(client);
	} else if(!strcmp(command, "kill")) {
		KillClient(client);
	} else if(!strncmp(command, "layer", 5)) {
		x = (command[5] - '0') * 10;
		x += command[6] - '0';
		SetClientLayer(client, x);
	} else {
		Debug("Uknown window command \"%s\"", command);
	}

}

