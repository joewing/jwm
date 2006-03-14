/****************************************************************************
 * Header for screen functions.
 * Copyright (C) 2005 Joe Wingbermuehle
 ****************************************************************************/

#ifndef SCREEN_H
#define SCREEN_H

void InitializeScreens();
void StartupScreens();
void ShutdownScreens();
void DestroyScreens();

int GetCurrentScreen(int x, int y);
int GetMouseScreen();
int GetScreenWidth(int index);
int GetScreenHeight(int index);
int GetScreenX(int index);
int GetScreenY(int index);

int GetScreenCount();

#endif

