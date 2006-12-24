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

/** Window snap modes. */
typedef enum {
   SNAP_NONE                 = 0,  /**< Don't snap. */
   SNAP_SCREEN               = 1,  /**< Snap to the edges of the screen. */
   SNAP_BORDER               = 2   /**< Snap to all borders. */
} SnapModeType;

/** Window move modes. */
typedef enum {
   MOVE_OPAQUE,   /**< Show window contents while moving. */
   MOVE_OUTLINE   /**< Show an outline while moving. */
} MoveModeType;

/** Move a client window.
 * @param np The client to move.
 * @param startx The starting mouse x-coordinate (window relative).
 * @param starty The starting mouse y-coordinate (window relative).
 * @return 1 if the client moved, 0 otherwise.
 */
int MoveClient(struct ClientNode *np, int startx, int starty);

/** Move a client window using the keyboard (mouse optional).
 * @param np The client to move.
 * @return 1 if the client moved, 0 otherwise.
 */
int MoveClientKeyboard(struct ClientNode *np);

/** Set the snap mode to use.
 * @param mode The snap mode to use.
 */
void SetSnapMode(SnapModeType mode);

/** Set the snap distance to use.
 * @param value A string representation of the distance to use.
 */
void SetSnapDistance(const char *value);

/** Set the snap distance to the default. */
void SetDefaultSnapDistance();

/** Set the move mode to use.
 * @param mode The move mode to use.
 */
void SetMoveMode(MoveModeType mode);

#endif /* MOVE_H */

