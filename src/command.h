/****************************************************************************
 * Handle running startup/shutdown/restart commands.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef COMMAND_H
#define COMMAND_H

void InitializeCommands();
void StartupCommands();
void ShutdownCommands();
void DestroyCommands();

void AddStartupCommand(const char *command);
void AddShutdownCommand(const char *command);
void AddRestartCommand(const char *command);

#endif

