/***************************************************************************
 * Header for the root menu functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ***************************************************************************/

#ifndef ROOT_H
#define ROOT_H

struct MenuType;

void InitializeRootMenu();
void StartupRootMenu();
void ShutdownRootMenu();
void DestroyRootMenu();

void SetRootMenu(struct MenuType *m);
void SetShowExitConfirmation(int v);

void ShowRootMenu(int x, int y);

void RunCommand(const char *command);

void Restart();
void Exit();

#endif

