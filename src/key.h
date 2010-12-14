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

struct ClientNode;

/** Enumeration of key binding types. */
typedef enum {
   KEY_NONE,
   KEY_UP,
   KEY_DOWN,
   KEY_RIGHT,
   KEY_LEFT,
   KEY_ESC,
   KEY_ENTER,
   KEY_NEXT,
   KEY_NEXTSTACK,
   KEY_PREV,
   KEY_PREVSTACK,
   KEY_CLOSE,
   KEY_MIN,
   KEY_MAX,
   KEY_SHADE,
   KEY_STICK,
   KEY_MOVE,
   KEY_RESIZE,
   KEY_ROOT,
   KEY_WIN,
   KEY_DESKTOP,
   KEY_RDESKTOP,
   KEY_LDESKTOP,
   KEY_UDESKTOP,
   KEY_DDESKTOP,
   KEY_SHOWDESK,
   KEY_SHOWTRAY,
   KEY_EXEC,
   KEY_RESTART,
   KEY_EXIT
} KeyType;

void InitializeKeys();
void StartupKeys();
void ShutdownKeys();
void DestroyKeys();

/** Get the action to take from a key event.
 * @param event The event.
 */
KeyType GetKey(const XKeyEvent *event);

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
void InsertBinding(KeyType key, const char *modifiers,
   const char *stroke, const char *code, const char *command);

/** Run a command caused by a key binding.
 * @param event The event causing the command to be run.
 */
void RunKeyCommand(const XKeyEvent *event);

/** Show a root menu caused by a key binding.
 * @param event The event that caused the menu to be shown.
 */
void ShowKeyMenu(const XKeyEvent *event);

/** Validate key bindings.
 * This will log an error if an invalid key binding is found.
 * This is called after parsing the configuration file.
 */
void ValidateKeys();

#endif /* KEY_H */

