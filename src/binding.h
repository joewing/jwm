/**
 * @file binding.h
 * @author Joe Wingbermuehle
 * @date 2013
 *
 * @brief Header for the binding functions that are shared between key
 *        and mouse bindings.
 *
 */

#ifndef BINDING_H_
#define BINDING_H_

struct ClientNode;

typedef unsigned char ActionType;
#define ACTION_NONE           0     /**< No action */
#define ACTION_SELECT         1     /**< Select current item (for menus) */
#define ACTION_CLOSE          2     /**< Close window */
#define ACTION_MIN            3     /**< Minimize window */
#define ACTION_VMAX           4     /**< Maximize window (vertically) */
#define ACTION_HMAX           5     /**< Maximize window (horizontally) */
#define ACTION_MAX            6     /**< Maximize window (default) */
#define ACTION_SHADE          7     /**< Shade window */
#define ACTION_STICK          8     /**< Make window sticky */
#define ACTION_RESIZE         9     /**< Resize window */
#define ACTION_MOVE           10    /**< Move window */
#define ACTION_ROOT           11    /**< Show root menu */
#define ACTION_WIN            12    /**< Show window menu */
#define ACTION_EXEC           13    /**< Execute command */
#define ACTION_DESKTOP        14    /**< Change desktop */
#define ACTION_RDESKTOP       15    /**< Change to the right desktop */
#define ACTION_LDESKTOP       16    /**< Change to the left desktop */
#define ACTION_DDESKTOP       17    /**< Change to the desktop below */
#define ACTION_UDESKTOP       18    /**< Change to the desktop above */
#define ACTION_SHOWDESK       19    /**< Show the desktop */
#define ACTION_SHOWTRAY       20    /**< Show the tray */
#define ACTION_NEXT           21    /**< Next window by list */
#define ACTION_NEXTSTACK      22    /**< Next window by stack */
#define ACTION_PREV           23    /**< Previous window by list */
#define ACTION_PREVSTACK      24    /**< Previous window by stack */
#define ACTION_RESTART        25    /**< Restart JWM */
#define ACTION_EXIT           26    /**< Exit JWM */
#define ACTION_FULLSCREEN     27    /**< Make window full screen */
#define ACTION_SENDTO         28    /**< Send window to a specific desktop */
#define ACTION_SENDLEFT       29    /**< Send window to the left desktop */
#define ACTION_SENDRIGHT      30    /**< Send window to the right desktop */
#define ACTION_SENDUP         31    /**< Send window to the above desktop */
#define ACTION_SENDDOWN       32    /**< Send window to the below desktop */
#define ACTION_ENTER          33    /**< Enter key */
#define ACTION_ESC            34    /**< Escape key */
#define ACTION_RIGHT          35    /**< Left key */
#define ACTION_LEFT           36    /**< Right key */
#define ACTION_UP             37    /**< Up key */
#define ACTION_DOWN           38    /**< Down key */
#define ACTION_KILL           39    /**< Kill a window */
#define ACTION_LAYER          40    /**< Set window layer */

typedef struct {
   char *arg;
   ActionType type;
} ActionNode;

typedef struct ActionContext {

   struct ClientNode *client;
   void *data;
   int desktop;
   int x;
   int y;
   void (*MoveFunc)(const struct ActionContext *ac, int x, int y, char snap);
   void (*ResizeFunc)(const struct ActionContext *ac, int x, int y);

} ActionContext;

/** Initialize action context.
 * @param ad The action context to initialize.
 */
void InitActionContext(ActionContext *ac);

/** Run an action.
 * @param context The context in which to run the action.
 * @param action The action to perform.
 * @return 1 if a menu was shown, 0 otherwise.
 */
char RunAction(const ActionContext *context, const ActionNode *action);

#endif /* BINDING_H_ */
