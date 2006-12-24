/**
 * @file command.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Handle running startup, shutdown, and restart commands.
 *
 */

#ifndef COMMAND_H
#define COMMAND_H

/*@{*/
void InitializeCommands();
void StartupCommands();
void ShutdownCommands();
void DestroyCommands();
/*@}*/

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

/** Run a command.
 * @param command The command to run (run in sh).
 */
void RunCommand(const char *command);

#endif /* COMMAND_H */

