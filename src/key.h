/**
 * @file key.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the key binding functions.
 *
 */

#ifndef KEY_H
#define KEY_H

#include "binding.h"

struct ClientNode;

extern unsigned int lockMask;

/*@{*/
void InitializeKeys();
void StartupKeys();
void ShutdownKeys();
void DestroyKeys();
/*@}*/

/** Get the action to take from a key event.
 * @param event The event.
 */
ActionType GetKey(const XKeyEvent *event);

/** Get a key mask from a modifier string. */
unsigned int ParseModifierString(const char *str);

/** Grab keys on a client window.
 * @param np The client.
 */
void GrabKeys(struct ClientNode *np);

/** Insert a key binding.
 * @param modifiers The modifier mask.
 * @param stroke The key stroke (not needed if code given).
 * @param code The key code (not needed if stroke given).
 * @param action The action (arg should not be freed).
 */
void InsertKeyBinding(const char *modifiers,
                      const char *stroke,
                      const char *code,
                      const ActionNode *action);

/** Run a command caused by a key binding.
 * @param event The event causing the command to be run.
 * @param np The active client.
 */
void RunKeyCommand(const XKeyEvent *event);

#endif /* KEY_H */

