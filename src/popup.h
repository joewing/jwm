/****************************************************************************
 * Header for popup functions.
 * Copyright (C) 2006 Joe Wingbermuehle
 ****************************************************************************/

#ifndef POPUP_H
#define POPUP_H

void InitializePopup();
void StartupPopup();
void ShutdownPopup();
void DestroyPopup();

void ShowPopup(int x, int y, const char *text);

void SetPopupEnabled(int e);

int ProcessPopupEvent(const XEvent *event);

#endif

