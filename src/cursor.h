/****************************************************************************
 * Header for the cursor functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef CURSOR_H
#define CURSOR_H

#include "border.h"

void InitializeCursors();
void StartupCursors();
void ShutdownCursors();
void DestroyCursors();

int GrabMouseForResize(BorderActionType action);
int GrabMouseForMove(Window w);

int GrabMouseForMenu();

Cursor GetFrameCursor(BorderActionType action);

void MoveMouse(Window win, int x, int y);

void SetMousePosition(int x, int y);
void GetMousePosition(int *x, int *y);

unsigned int GetMouseMask();

void SetDefaultCursor(Window w);

void SetDoubleClickSpeed(const char *str);
void SetDoubleClickDelta(const char *str);

#endif


