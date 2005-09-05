/****************************************************************************
 * Outlines for moving and resizing.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef OUTLINE_H
#define OUTLINE_H

void InitializeOutline();
void StartupOutline();
void ShutdownOutline();
void DestroyOutline();

void DrawOutline(int x, int y, int width, int height);
void ClearOutline();

#endif

