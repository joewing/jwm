/****************************************************************************
 * Handle running startup/shutdown/restart commands.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"
#include "command.h"
#include "root.h"

static char *startupCommand = NULL;
static char *shutdownCommand = NULL;

/****************************************************************************
 ****************************************************************************/
void InitializeCommands() {
	startupCommand = NULL;
	shutdownCommand = NULL;
}

/****************************************************************************
 ****************************************************************************/
void StartupCommands() {
	if(startupCommand) {
		RunCommand(startupCommand);
		Release(startupCommand);
		startupCommand = NULL;
	}
}

/****************************************************************************
 ****************************************************************************/
void ShutdownCommands() {
	if(shutdownCommand) {
		RunCommand(shutdownCommand);
		Release(shutdownCommand);
		shutdownCommand = NULL;
	}
}

/****************************************************************************
 ****************************************************************************/
void DestroyCommands() {
	if(startupCommand) {
		Release(startupCommand);
		startupCommand = NULL;
	}
	if(shutdownCommand) {
		Release(shutdownCommand);
		shutdownCommand = NULL;
	}
}

/****************************************************************************
 ****************************************************************************/
void SetStartupCommand(const char *command) {
	if(startupCommand) {
		Release(startupCommand);
		startupCommand = NULL;
	}
	if(command) {
		startupCommand = Allocate(strlen(command) + 1);
		strcpy(startupCommand, command);
	}
}

/****************************************************************************
 ****************************************************************************/
void SetShutdownCommand(const char *command) {
	if(shutdownCommand) {
		Release(shutdownCommand);
		shutdownCommand = NULL;
	}
	if(command) {
		shutdownCommand = Allocate(strlen(command) + 1);
		strcpy(shutdownCommand, command);
	}
}

