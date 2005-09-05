/****************************************************************************
 * Header for the event functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef EVENT_H
#define EVENT_H

void WaitForEvent();
void ProcessEvent(XEvent *event);

void SetShowMenuOnRoot(int v);

#endif

