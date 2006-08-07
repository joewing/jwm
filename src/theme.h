/****************************************************************************
 * Header for the theme functions.
 * Copyright (C) 2006 Joe Wingbermuehle
 ****************************************************************************/

#ifndef THEME_H
#define THEME_H

void InitializeThemes();
void StartupThemes();
void ShutdownThemes();
void DestroyThemes();

void AddThemePath(char *path);

void SetTheme(const char *name);

#endif

