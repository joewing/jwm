/****************************************************************************
 * Header for client window resize functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef RESIZE_H
#define RESIZE_H

void ResizeClient(ClientNode *np, BorderActionType action,
	int startx, int starty);
void ResizeClientKeyboard(ClientNode *np);

void SetResizeMode(ResizeModeType mode);

#endif

