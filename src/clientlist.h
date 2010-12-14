/**
 * @file clientlist.h
 * @author Joe Wingbermuehle
 * @date 2007
 *
 * @brief Functions to manage lists of clients.
 *
 */

#ifndef CLIENTLIST_H
#define CLIENTLIST_H

#include "hint.h"

struct ClientNode;

/** Client windows in linked lists for each layer. */
extern struct ClientNode *nodes[LAYER_COUNT];

/** Client windows in linked lists for each layer (pointer to the tail). */
extern struct ClientNode *nodeTail[LAYER_COUNT];

/** Determine if a client is allowed focus.
 * @param np The client.
 * @return 1 if focus is allowed, 0 otherwise.
 */
int ShouldFocus(const struct ClientNode *np);

/** Start walking the window stack. */
void StartWindowStackWalk();

/** Move to the next/previous window in the window stack. */
void WalkWindowStack(int forward);

/** Stop walking the window stack. */
void StopWindowStackWalk();

/** Set the keyboard focus to the next client.
 * This is used to focus the next client in the stacking order.
 * @param np The client before the client to focus.
 */
void FocusNextStacked(struct ClientNode *np);

#endif /* CLIENTLIST_H */

