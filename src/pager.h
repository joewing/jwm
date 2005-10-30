/****************************************************************************
 * Pager header file.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef PAGER_H
#define PAGER_H

struct TrayComponentType;

void InitializePager();
void StartupPager();
void ShutdownPager();
void DestroyPager();

struct TrayComponentType *CreatePager();

void UpdatePager();

#endif

