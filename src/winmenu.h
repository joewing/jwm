/**
 * @file tray.h
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

void GetWindowMenuSize(struct ClientNode *np, int *width, int *height);

void ShowWindowMenu(struct ClientNode *np, int x, int y);

void ChooseWindow(const MenuAction *action); 

#endif

