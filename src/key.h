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

/** Enumeration of key binding types.
 * Note that we use the high bits to store additional information
 * for some key types (for example the desktop number).
 */
typedef unsigned short KeyType;
#define KEY_NONE            0
#define KEY_UP              1
#define KEY_DOWN            2
#define KEY_RIGHT           3
#define KEY_LEFT            4
#define KEY_ESC             5
#define KEY_ENTER           6
#define KEY_NEXT            7
#define KEY_NEXTSTACK       8
#define KEY_PREV            9
#define KEY_PREVSTACK       10
#define KEY_CLOSE           11
#define KEY_MIN             12
#define KEY_MAX             13
#define KEY_SHADE           14
#define KEY_STICK           15
#define KEY_MOVE            16
#define KEY_RESIZE          17
#define KEY_ROOT            18
#define KEY_WIN             19
#define KEY_DESKTOP         20
#define KEY_RDESKTOP        21
#define KEY_LDESKTOP        22
#define KEY_UDESKTOP        23
#define KEY_DDESKTOP        24
#define KEY_SHOWDESK        25
#define KEY_SHOWTRAY        26
#define KEY_EXEC            27
#define KEY_RESTART         28
#define KEY_EXIT            29
#define KEY_FULLSCREEN      30
#define KEY_SENDR           31
#define KEY_SENDL           32
#define KEY_SENDU           33
#define KEY_SENDD           34
#define KEY_MAXTOP          35
#define KEY_MAXBOTTOM       36
#define KEY_MAXLEFT         37
#define KEY_MAXRIGHT        38
#define KEY_MAXV            39
#define KEY_MAXH            40
#define KEY_RESTORE         41

void InitializeKeys(void);
void StartupKeys(void);
void ShutdownKeys(void);
void DestroyKeys(void);

/** Mask of 'lock' keys. */
extern unsigned int lockMask;

/** Get the action to take from a key event.
 * @param event The event.
 */
KeyType GetKey(const XKeyEvent *event);

/** Insert a key binding.
 * @param key The key binding type.
 * @param modifiers The modifier mask.
 * @param stroke The key stroke (not needed if code given).
 * @param code The key code (not needed if stroke given).
 * @param command Extra parameter needed for some key binding types.
 */
void InsertBinding(KeyType key, const char *modifiers, const char *stroke,
                   const char *code, const char *command);

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
void ValidateKeys(void);

#endif /* KEY_H */
