/****************************************************************************
 * Header for the load functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef LOAD_H
#define LOAD_H

extern int loadWidth;

void InitializeLoadDisplay();
void StartupLoadDisplay();
void ShutdownLoadDisplay();
void DestroyLoadDisplay();

void UpdateLoadDisplay(Window w, GC gc, int xoffset);

void SetLoadEnabled(int e);

#endif

