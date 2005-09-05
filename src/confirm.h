/***************************************************************************
 * Header for the confirm dialog functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ***************************************************************************/

#ifndef CONFIRM_H
#define CONFIRM_H

void InitializeDialogs();
void StartupDialogs();
void ShutdownDialogs();
void DestroyDialogs();

int ProcessDialogEvent(const XEvent *event);

void ShowConfirmDialog(ClientNode *np, void (*action)(ClientNode*), ...);

#endif

