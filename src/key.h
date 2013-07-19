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

/** Enumeration of key binding types.
 * Note that we use the high bits to store additional information
 * for some key types (for example the desktop number).
 */
typedef unsigned char KeyType;
#define KEY_NONE           0
#define KEY_UP             1
#define KEY_DOWN           2
#define KEY_RIGHT          3
#define KEY_LEFT           4
#define KEY_ESC            5
#define KEY_ENTER          6
#define KEY_ACTION         7

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
KeyType GetKey(const XKeyEvent *event);

/** Get a key mask from a modifier string. */
unsigned int ParseModifierString(const char *str);

/** Grab keys on a client window.
 * @param np The client.
 */
void GrabKeys(struct ClientNode *np);

/** Insert a key binding.
 * @param key The key binding type.
 * @param modifiers The modifier mask.
 * @param stroke The key stroke (not needed if code given).
 * @param code The key code (not needed if stroke given).
 * @param command Extra parameter needed for some key binding types.
 */
void InsertKeyBinding(KeyType key,
                      ActionType action,
                      const char *modifiers,
                      const char *stroke,
                      const char *code,
                      const char *command);

/** Run a command caused by a key binding.
 * @param event The event causing the command to be run.
 * @param np The active client.
 */
void RunKeyCommand(const XKeyEvent *event);

/** Validate key bindings.
 * This will log an error if an invalid key binding is found.
 * This is called after parsing the configuration file.
 */
void ValidateKeys();

#endif /* KEY_H */

