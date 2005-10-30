/****************************************************************************
 * Header for client window resize functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef RESIZE_H
#define RESIZE_H

#include "border.h"

struct ClientNode;

typedef enum {
	RESIZE_OPAQUE,
	RESIZE_OUTLINE
} ResizeModeType;

void ResizeClient(struct ClientNode *np, BorderActionType action,
	int startx, int starty);
void ResizeClientKeyboard(struct ClientNode *np);

void SetResizeMode(ResizeModeType mode);

#endif

