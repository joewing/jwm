/****************************************************************************
 * Header for client window move functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef MOVE_H
#define MOVE_H

int MoveClient(ClientNode *np, int startx, int starty);
int MoveClientKeyboard(ClientNode *np);

void SetSnapMode(SnapModeType mode);
void SetSnapDistance(const char *value);
void SetDefaultSnapDistance();

void SetMoveMode(MoveModeType mode);

#endif

