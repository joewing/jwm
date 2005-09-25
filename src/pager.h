/****************************************************************************
 * Pager header file.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef PAGER_H
#define PAGER_H

extern int pagerWidth;

void InitializePager();
void StartupPager();
void ShutdownPager();
void DestroyPager();

void DrawPager(Window w, GC gc, int xoffset);

void SetPagerEnabled(int e);

#endif

