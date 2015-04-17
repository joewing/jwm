/**
 * @file action.c
 * @author Joe Wingbermuehle
 *
 * @brief Tray component actions.
 */

#include "action.h"
#include "tray.h"

/** Add an action. */
void AddAction(struct ActionType **actions, const char *action, int mask)
{
}

/** Destroy an action list. */
void DestroyActions(struct ActionType *actions)
{
}

/** Process a button press. */
void ProcessActionPress(struct ActionType *actions,
                        struct TrayComponentType *cp,
                        int x, int y, int button)
{
}

/** Process a button release. */
void ProcessActionRelease(struct ActionType *actions,
                          struct TrayComponentType *cp,
                          int x, int y, int button)
{
}
