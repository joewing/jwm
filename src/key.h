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

#include "action.h"

/** Mouse binding contexts. */
typedef unsigned char MouseContextType;
#define MC_NONE            0     /**< Keyboard/none. */
#define MC_ROOT            1     /**< Root window. */
#define MC_BORDER          2     /**< Resize handle. */
#define MC_MOVE            3     /**< Move handle. */
#define MC_CLOSE           4     /**< Close button. */
#define MC_MAXIMIZE        5     /**< Maximize button. */
#define MC_MINIMIZE        6     /**< Minimize button. */
#define MC_ICON            7     /**< Window menu button. */
#define MC_COUNT           8     /**< Number of contexts. */
#define MC_MASK            0x0F  /**< Context type mask. */
#define MC_BORDER_N        0x10  /**< North border. */
#define MC_BORDER_S        0x20  /**< South border. */
#define MC_BORDER_E        0x40  /**< East border. */
#define MC_BORDER_W        0x80  /**< West border. */

void InitializeKeys(void);
void StartupKeys(void);
void ShutdownKeys(void);
void DestroyKeys(void);

/** Mask of 'lock' keys. */
extern unsigned int lockMask;

/** Get the action to take from a key event. */
ActionType GetKey(MouseContextType context, unsigned state, int code);

/** Parse a modifier string.
 * @param str The modifier string.
 * @return The modifier mask.
 */
unsigned int ParseModifierString(const char *str);

/** Insert a key binding.
 * @param key The key binding type.
 * @param modifiers The modifier mask.
 * @param stroke The key stroke (not needed if code given).
 * @param code The key code (not needed if stroke given).
 * @param command Extra parameter needed for some key binding types.
 */
void InsertBinding(ActionType key, const char *modifiers, const char *stroke,
                   const char *code, const char *command);

/** Insert a mouse binding.
 * A mouse binding maps a mouse click in a certain context to an action.
 * @param button The mouse button.
 * @param mask The modifier mask.
 * @param context The mouse context.
 * @param key The key binding.
 * @param command Extra parameter needed for some bindings.
 */
void InsertMouseBinding(
   int button,
   const char *mask,
   MouseContextType context,
   ActionType key,
   const char *command);

/** Run a command caused by a key binding. */
void RunKeyCommand(MouseContextType context, unsigned state, int code);

/** Show a root menu caused by a key binding.
 * @param event The event that caused the menu to be shown.
 */
void ShowKeyMenu(MouseContextType context, unsigned state, int code);

/** Validate key bindings.
 * This will log an error if an invalid key binding is found.
 * This is called after parsing the configuration file.
 */
void ValidateKeys(void);

#endif /* KEY_H */
