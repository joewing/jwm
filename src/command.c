/****************************************************************************
 * Handle running startup/shutdown/restart commands.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"
#include "command.h"
#include "root.h"
#include "misc.h"

typedef struct CommandNode {
	char *command;
	struct CommandNode *next;
} CommandNode;

static CommandNode *startupCommands;
static CommandNode *shutdownCommands;

static void RunCommands(CommandNode *commands);
static void ReleaseCommands(CommandNode **commands);
static void AddCommand(CommandNode **commands, const char *command);

/****************************************************************************
 ****************************************************************************/
void InitializeCommands() {
	startupCommands = NULL;
	shutdownCommands = NULL;
}

/****************************************************************************
 ****************************************************************************/
void StartupCommands() {

	RunCommands(startupCommands);

}

/****************************************************************************
 ****************************************************************************/
void ShutdownCommands() {

	RunCommands(shutdownCommands);

}

/****************************************************************************
 ****************************************************************************/
void DestroyCommands() {
	ReleaseCommands(&startupCommands);
	ReleaseCommands(&shutdownCommands);
}

/****************************************************************************
 ****************************************************************************/
void RunCommands(CommandNode *commands) {

	CommandNode *cp;

	for(cp = commands; cp; cp = cp->next) {
		RunCommand(cp->command);
	}

}

/****************************************************************************
 ****************************************************************************/
void ReleaseCommands(CommandNode **commands) {

	CommandNode *cp;

	while(*commands) {
		cp = (*commands)->next;
		Release((*commands)->command);
		Release(*commands);
		*commands = cp;
	}

}

/****************************************************************************
 ****************************************************************************/
void AddCommand(CommandNode **commands, const char *command) {

	CommandNode *cp;

	if(!command) {
		return;
	}

	cp = Allocate(sizeof(CommandNode));
	cp->next = *commands;
	*commands = cp;

	cp->command = CopyString(command);

}

/****************************************************************************
 ****************************************************************************/
void AddStartupCommand(const char *command) {
	AddCommand(&startupCommands, command);
}

/****************************************************************************
 ****************************************************************************/
void AddShutdownCommand(const char *command) {
	AddCommand(&shutdownCommands, command);
}

