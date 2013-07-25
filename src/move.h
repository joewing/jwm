/**
 * @file move.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for client window move functions.
 *
 */

#ifndef MOVE_H
#define MOVE_H

struct ClientNode;

/** Move a client window.
 * @param np The client to move.
 * @param startx The starting mouse x-coordinate (window relative).
 * @param starty The starting mouse y-coordinate (window relative).
 * @param snap 1 to do edge snapping, 0 otherwise.
 * @return 1 if the client moved, 0 otherwise.
 */
char MoveClient(struct ClientNode *np, int startx, int starty, char snap);

/** Move a client window using the keyboard (mouse optional).
 * @param np The client to move.
 * @param startx (ignored)
 * @param starty (ignored)
 * @param snap (ignored)
 * @return 1 if the client moved, 0 otherwise.
 */
char MoveClientKeyboard(struct ClientNode *np, int startx, int starty,
                        char snap);

#endif /* MOVE_H */

