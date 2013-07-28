/**
 * @file mouse.h
 * @author Joe Wingbermuehle
 * @date 2013
 *
 * @brief Header for the mouse binding functions.
 *
 */

#ifndef MOUSE_H_
#define MOUSE_H_

#include "binding.h"

struct ClientNode;

typedef unsigned char ContextType;
#define CONTEXT_NONE          0
#define CONTEXT_BORDER        1
#define CONTEXT_TITLE         2
#define CONTEXT_MENU          3
#define CONTEXT_MIN           4
#define CONTEXT_MAX           5
#define CONTEXT_CLOSE         6
#define CONTEXT_BUTTON        7
#define CONTEXT_PAGER         8
#define CONTEXT_CLOCK         9
#define CONTEXT_ROOT          10
#define CONTEXT_WINDOW        11
#define CONTEXT_TASK          12

#define CONTEXT_COUNT         13

typedef void (*ReleaseCallback)(const XButtonEvent *event, void *arg);

/*@{*/
void InitializeMouse();
#define StartupMouse()        (void)(0)
#define ShutdownMouse()       (void)(0)
void DestroyMouse();
/*@}*/

/** Check for and process a mouse grab.
 * @param event The event.
 * @return 1 if there was a grab, 0 otherwise.
 */
char CheckMouseGrab(const XButtonEvent *event);

/** Run a mouse bindings for a button event.
 * @param event The event (ButtonPress or ButtonRelease).
 * @param context The context where the event happened.
 * @param ac The action context.
 * @return 1 if the button was released (for a menu), 0 otherwise.
 */
char RunMouseCommand(const XButtonEvent *event,
                     ContextType context,
                     const ActionContext *ac);

/** Register a callback for a mouse grab.
 * This should be done when grabbing the mouse to be notified of a release.
 * The mouse grab should be removed within this callback.
 * @param c The callback.
 * @param a An argument for the callback.
 */
void SetButtonReleaseCallback(ReleaseCallback c, void *arg);

/** Insert a mouse binding.
 * This is called while parsing the configuration.
 * @param context The context of the binding.
 * @param button The mouse button.
 * @param modifiers Keyboard modifiers.
 * @param action The action.
 */
void InsertMouseBinding(ContextType context,
                        int button,
                        const char *modifiers,
                        const ActionNode *action);

#endif /* MOUSE_H_ */
