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

struct ActionDataType;

/** Move a client window.
 * @param ad Action data.
 * @param startx The starting mouse x-coordinate (window relative).
 * @param starty The starting mouse y-coordinate (window relative).
 * @param snap 1 to do edge snapping, 0 otherwise.
 * @return 1 if the client moved, 0 otherwise.
 */
void MoveClient(const struct ActionDataType *ad,
                int startx, int starty, char snap);

/** Move a client window using the keyboard (mouse optional).
 * @param ad Action data.
 * @param startx (ignored)
 * @param starty (ignored)
 * @param snap (ignored)
 * @return 1 if the client moved, 0 otherwise.
 */
void MoveClientKeyboard(const struct ActionDataType *np,
                        int startx, int starty, char snap);

#endif /* MOVE_H */

