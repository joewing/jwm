/**
 * @file action.h
 * @author Joe Wingbermuehle
 *
 * @brief Mouse click action processing for tray buttons and clocks.
 *
 */

#ifndef ACTION_H
#define ACTION_H

struct ActionType;
struct TrayComponentType;

/** Add an action to a list of actions.
 * @param actions The action list to update.
 * @param action The action to add to the list.
 * @param mask The mouse button mask.
 */
void AddAction(struct ActionType **actions, const char *action, int mask);

/** Destroy a list of actions. */
void DestroyActions(struct ActionType *actions);

/** Process a button press event.
 * @param actions The action list.
 * @param cp The tray component.
 * @param x The mouse x-coordinate.
 * @param y The mouse y-coordinate.
 * @param button The mouse button.
 */
void ProcessActionPress(struct ActionType *actions,
                        struct TrayComponentType *cp,
                        int x, int y, int button);

/** Process a button release event.
 * @param actions The action list.
 * @param cp The tray component.
 * @param x The mouse x-coordinate.
 * @param y The mouse y-coordinate.
 * @param button The mouse button.
 */
void ProcessActionRelease(struct ActionType *actions,
                          struct TrayComponentType *cp,
                          int x, int y, int button);

/** Validate actions.
 * @param actions The action list to validate.
 */
void ValidateActions(const struct ActionType *actions);

#endif /* ACTION_H */
