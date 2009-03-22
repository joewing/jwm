/**
 * @file root.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the root menu functions.
 *
 */

#ifndef ROOT_H
#define ROOT_H

struct Menu;

/*@{*/
void InitializeRootMenu();
void StartupRootMenu();
void ShutdownRootMenu();
void DestroyRootMenu();
/*@}*/

/** Set the root menu to be used for the specified indexes.
 * @param indexes The indexes (ASCII string of '0' to '9').
 * @param m The menu to use for the specified indexes.
 */
void SetRootMenu(const char *indexes, struct Menu *m);

/** Set whether a confirmation dialog is displayed on exit.
 * @param v 1 to display confirmation, 0 to just exit.
 */
void SetShowExitConfirmation(int v);

/** Determine if a root menu is defined for the specified index.
 * @return 1 if it is defined, 0 if not.
 */
int IsRootMenuDefined(int index);

/** Get the size of a root menu.
 * @param index The root menu index.
 * @param width The width output.
 * @param height The height output.
 */
void GetRootMenuSize(int index, int *width, int *height);

/** Show a root menu.
 * @param index The root menu index.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return 1 if a menu was displayed, 0 if not.
 */
int ShowRootMenu(int index, int x, int y);

/** Restart the window manager. */
void Restart();

/** Exit the window manager. */
void Exit();

#endif /* ROOT_H */

