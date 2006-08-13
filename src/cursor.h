/**
 * @file confirm.h
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

int GrabMouseForResize(BorderActionType action);
int GrabMouseForMove();

int GrabMouseForMenu();
int GrabMouseForChoose();

Cursor GetFrameCursor(BorderActionType action);

void MoveMouse(Window win, int x, int y);

void SetMousePosition(int x, int y);
void GetMousePosition(int *x, int *y);

unsigned int GetMouseMask();

void SetDefaultCursor(Window w);

void SetDoubleClickSpeed(const char *str);
void SetDoubleClickDelta(const char *str);

#endif


