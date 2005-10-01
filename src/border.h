/****************************************************************************
 * Header for border functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef BORDER_H
#define BORDER_H

void InitializeBorders();
void StartupBorders();
void ShutdownBorders();
void DestroyBorders();

BorderActionType GetBorderActionType(const ClientNode *np, int x, int y);
void DrawBorder(const ClientNode *np);

int GetBorderIconSize();

void SetBorderWidth(const char *str);
void SetTitleHeight(const char *str);

void ExposeCurrentDesktop();

#endif

