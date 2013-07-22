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

#define CONTEXT_COUNT         12

typedef void (*ReleaseCallback)(int x, int y);

/*@{*/
void InitializeMouse();
#define StartupMouse()        (void)(0)
#define ShutdownMouse()       (void)(0)
void DestroyMouse();
/*@}*/

/** Run a mouse bindings for a button event.
 * @param event The event (ButtonPress or ButtonRelease).
 * @param context The context where the event happened.
 */
void RunMouseCommand(const XButtonEvent *event,
                     ContextType context);

/** Register a callback for a mouse grab.
 * This should be done when grabbing the mouse to be notified of a release.
 * The mouse grab should be removed within this callback.
 * @param c The callback.
 */
void SetButtonReleaseCallback(ReleaseCallback c);

/** Insert a mouse binding.
 * This is called while parsing the configuration.
 * @param context The context of the binding.
 * @param action The action to perform.
 * @param button The mouse button.
 * @param modifiers Keyboard modifiers.
 * @param command Extra parameter for the action.
 */
void InsertMouseBinding(ContextType context,
                        ActionType action,
                        int button,
                        const char *modifiers,
                        const char *command);

#endif /* MOUSE_H_ */
