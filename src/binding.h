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
#define ACTION_NONE           0
#define ACTION_SELECT         1
#define ACTION_CLOSE          2
#define ACTION_MIN            3
#define ACTION_VMAX           4
#define ACTION_HMAX           5
#define ACTION_MAX            6
#define ACTION_SHADE          7
#define ACTION_STICK          8
#define ACTION_RESIZE         9
#define ACTION_MOVE           10
#define ACTION_ROOT           11
#define ACTION_WIN            12
#define ACTION_EXEC           13
#define ACTION_DESKTOP        14
#define ACTION_RDESKTOP       15
#define ACTION_LDESKTOP       16
#define ACTION_DDESKTOP       17
#define ACTION_UDESKTOP       18
#define ACTION_SHOWDESK       19
#define ACTION_SHOWTRAY       20
#define ACTION_NEXT           21
#define ACTION_NEXTSTACK      22
#define ACTION_PREV           23
#define ACTION_PREVSTACK      24
#define ACTION_RESTART        25
#define ACTION_EXIT           26
#define ACTION_FULLSCREEN     27
#define ACTION_SENDTO         28
#define ACTION_SENDLEFT       29
#define ACTION_SENDRIGHT      30
#define ACTION_SENDUP         31
#define ACTION_SENDDOWN       32

typedef struct {

   struct ClientNode *client;
   int desktop;
   int x;
   int y;
   char (*MoveFunc)(struct ClientNode *np, int x, int y, char snap);
   void (*ResizeFunc)(struct ClientNode *np, int x, int y);

} ActionDataType;

/** Run an action.
 * @param action The action to perform.
 * @param arg An argument to the action.
 * @param data Data for the action.
 * @return 1 if a menu was shown, 0 otherwise.
 */
char RunAction(ActionType action,
               const char *arg,
               const ActionDataType *data);

#endif /* BINDING_H_ */
