/**
 * @file screen.h
 * @author Joe Wingbermuehle
 * @date 2005-2006
 *
 * @brief Header for screen functions.
 *
 */

#ifndef SCREEN_H
#define SCREEN_H

typedef struct ScreenType {
	int index;
	int x, y;
	int width, height;
} ScreenType;

void InitializeScreens();
void StartupScreens();
void ShutdownScreens();
void DestroyScreens();

const ScreenType *GetCurrentScreen(int x, int y);
const ScreenType *GetMouseScreen();
const ScreenType *GetScreen(int index);

int GetScreenCount();

#endif

