/****************************************************************************
 * Pager header file.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef PAGER_H
#define PAGER_H

void InitializePager();
void StartupPager();
void ShutdownPager();
void DestroyPager();

TrayComponentType *CreatePager(int width, int height);

void UpdatePager();

#endif

