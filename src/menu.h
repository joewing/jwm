/***************************************************************************
 * Header for the menu functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ***************************************************************************/

#ifndef MENU_H
#define MENU_H

typedef void (*RunMenuCommandType)(const char *command);

void InitializeMenu(MenuType *menu);
void ShowMenu(MenuType *menu, RunMenuCommandType runner, int x, int y);
void DestroyMenu(MenuType *menu);

extern int menuShown;

#endif

