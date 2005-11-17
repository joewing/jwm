/****************************************************************************
 * Clock tray component.
 * Copyright (C) 2005 Joe Wingbermuehle
 ****************************************************************************/

#ifndef CLOCK_H
#define CLOCK_H

struct TrayComponentType;

void InitializeClock();
void StartupClock();
void ShutdownClock();
void DestroyClock();

struct TrayComponentType *CreateClock(const char *format, const char *command);

void UpdateClocks();

#endif

