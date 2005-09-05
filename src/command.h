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

void SetStartupCommand(const char *command);
void SetShutdownCommand(const char *command);

#endif

