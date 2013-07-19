
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

/*@{*/
void InitializeMouse();
#define StartupMouse()        (void)(0)
#define ShutdownMouse()       (void)(0)
void DestroyMouse();
/*@}*/

void RunMouseCommand(const XButtonEvent *event,
                     ContextType context);

void InsertMouseBinding(ContextType context,
                        ActionType action,
                        int button,
                        const char *modifiers,
                        const char *command);

#endif /* MOUSE_H_ */
