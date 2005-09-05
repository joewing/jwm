/***************************************************************************
 * Functions to handle the root menu.
 * Copyright (C) 2004 Joe Wingbermuehle
 ***************************************************************************/

#include "jwm.h"

static MenuType *rootMenu = NULL;
static int showExitConfirmation = 1;
static void ExitHandler(ClientNode *np);

/***************************************************************************
 ***************************************************************************/
void InitializeRootMenu() {
}

/***************************************************************************
 ***************************************************************************/
void StartupRootMenu() {
	if(rootMenu) {
		InitializeMenu(rootMenu);
	}
}

/***************************************************************************
 ***************************************************************************/
void ShutdownRootMenu() {
}

/***************************************************************************
 ***************************************************************************/
void DestroyRootMenu() {
	if(rootMenu) {
		DestroyMenu(rootMenu);
		rootMenu = NULL;
	}
}

/***************************************************************************
 ***************************************************************************/
void SetRootMenu(MenuType *m) {
	if(rootMenu) {
		DestroyRootMenu();
	}
	rootMenu = m;
}

/***************************************************************************
 ***************************************************************************/
void SetShowExitConfirmation(int v) {
	showExitConfirmation = v;
}

/***************************************************************************
 ***************************************************************************/
void ShowRootMenu(int x, int y) {

	if(!rootMenu) {
		return;
	}

	ShowMenu(rootMenu, RunCommand, x, y);

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
			exitCommand = Allocate(strlen(command) - 5 + 1);
			strcpy(exitCommand, command + 6);
		}
		Exit();
		return;
	} else if(!strcmp(command, "#restart")) {
		Restart();
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

