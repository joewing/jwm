/**
 * @file cursor.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the cursor functions.
 *
 */

#ifndef CURSOR_H
#define CURSOR_H

#include "border.h"

/*@{*/
void InitializeCursors();
void StartupCursors();
void ShutdownCursors();
void DestroyCursors();
/*@}*/

/** Grab the mouse for resizing a window.
 * @param action The resize action.
 * @return 1 on success, 0 on failure.
 */
int GrabMouseForResize(BorderActionType action);

/** Grab the mouse for moving a window.
 * @return 1 on success, 0 on failure.
 */
int GrabMouseForMove();

/** Grab the mouse.
 * @return 1 on success, 0 on failure.
 */
int GrabMouse(Window w);

/** Grab the mouse to select a window.
 * @return 1 on success, 0 on failure.
 */
int GrabMouseForChoose();

/** Get the cursor to use given a border action.
 * @param action The border action.
 * @return The cursor to use.
 */
Cursor GetFrameCursor(BorderActionType action);

/** Move the mouse cursor.
 * @param win The window to act as an origin for the coordinates.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 */
void MoveMouse(Window win, int x, int y);

/** Set the current mouse position.
 * @param x The x-coordinate (relative to the current desktop).
 * @param y The y-coordinate (relative to the current desktop).
 */
void SetMousePosition(int x, int y);

/** Get the current mouse position.
 * @param x Location to store the x-coordinate.
 * @param y Location to store the y-coordinate.
 */
void GetMousePosition(int *x, int *y);

/** Get a mask of the current mouse buttons pressed.
 * @return A mask of the current mouse buttons pressed.
 */
unsigned int GetMouseMask();

/** Reset to the default cursor on a window.
 * @param w The window whose cursor to change.
 */
void SetDefaultCursor(Window w);

/** Set the double click speed.
 * @param str The speed (ASCII, milliseconds).
 */
void SetDoubleClickSpeed(const char *str);

/** Set the double click delta.
 * @param str The delta (ASCII, pixels).
 */
void SetDoubleClickDelta(const char *str);

#endif /* CURSOR_H */

