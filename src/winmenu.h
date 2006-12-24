/**
 * @file winmenu.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the window menu functions.
 *
 */

#ifndef WINMENU_H
#define WINMENU_H

#include "menu.h"

struct ClientNode;

/** Get the size of a window menu.
 * @param np The client for the window menu.
 * @param width The width return.
 * @param height The height return.
 */
void GetWindowMenuSize(struct ClientNode *np, int *width, int *height);

/** Show a window menu.
 * @param np The client for the window menu.
 * @param x The x-coordinate of the menu (root relative).
 * @param y The y-coordinate of the menu (root relative).
 */
void ShowWindowMenu(struct ClientNode *np, int x, int y);

/** Grab the mouse to select a window.
 * @param action The action to perform when a window is selected.
 */
void ChooseWindow(const MenuAction *action); 

#endif /* WINMENU_H */

