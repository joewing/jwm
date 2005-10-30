/****************************************************************************
 * Header for client window move functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef MOVE_H
#define MOVE_H

struct ClientNode;

typedef enum {
	SNAP_NONE                 = 0,
	SNAP_SCREEN               = 1,
	SNAP_BORDER               = 2
} SnapModeType;

typedef enum {
	MOVE_OPAQUE,
	MOVE_OUTLINE
} MoveModeType;

int MoveClient(struct ClientNode *np, int startx, int starty);
int MoveClientKeyboard(struct ClientNode *np);

void SetSnapMode(SnapModeType mode);
void SetSnapDistance(const char *value);
void SetDefaultSnapDistance();

void SetMoveMode(MoveModeType mode);

#endif

