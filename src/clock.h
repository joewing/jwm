/****************************************************************************
 * Clock tray component.
 * Copyright (C) 2005 Joe Wingbermuehle
 ****************************************************************************/

#ifndef CLOCK_H
#define CLOCK_H

struct TrayComponentType;
struct TimeType;

void InitializeClock();
void StartupClock();
void ShutdownClock();
void DestroyClock();

struct TrayComponentType *CreateClock(const char *format,
	const char *command, int width, int height);

void SignalClock(const struct TimeType *now, int x, int y);

#endif

