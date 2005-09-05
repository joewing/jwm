/****************************************************************************
 * Header for the cursor functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef CURSOR_H
#define CURSOR_H

void InitializeCursors();
void StartupCursors();
void ShutdownCursors();
void DestroyCursors();

int GrabMouseForMove();
int GrabMouseForMenu();
int GrabMouseForWM(Window w);
int GrabMouseForResize(BorderActionType action);

Cursor GetFrameCursor(BorderActionType action);

void GetMousePosition(int *x, int *y);
unsigned int GetMouseMask();

void SetDefaultCursor(Window w);

void SetDoubleClickSpeed(const char *str);
void SetDoubleClickDelta(const char *str);

#endif


