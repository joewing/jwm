/****************************************************************************
 * Header for popup functions.
 * Copyright (C) 2006 Joe Wingbermuehle
 ****************************************************************************/

#ifndef POPUP_H
#define POPUP_H

#define POPUP_DELTA 2

void InitializePopup();
void StartupPopup();
void ShutdownPopup();
void DestroyPopup();

void ShowPopup(int x, int y, const char *text);

void SetPopupEnabled(int e);
void SetPopupDelay(const char *str);

int ProcessPopupEvent(const XEvent *event);

extern int popupDelay;

#endif

