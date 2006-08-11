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

/** Add a command to be executed at startup.
 * @param command The command to execute.
 */
void AddStartupCommand(const char *command);

/** Add a command to be executed at shutdown.
 * @param command The command to execute.
 */
void AddShutdownCommand(const char *command);

/** Add a command to be executed after a restart.
 * @param command The command to execute.
 */
void AddRestartCommand(const char *command);

#endif

