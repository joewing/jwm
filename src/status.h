/*************************************************************************
 * Header for the status functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 *************************************************************************/

#ifndef STATUS_H
#define STATUS_H

void CreateMoveWindow();
void UpdateMoveWindow(int x, int y);
void DestroyMoveWindow();

void CreateResizeWindow();
void UpdateResizeWindow(int width, int height);
void DestroyResizeWindow();

#endif


