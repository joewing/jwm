/****************************************************************************
 * Timing functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef TIMING_H
#define TIMING_H

#define ZERO_TIME { 0, 0 }

void InitializeTiming();
void StartupTiming();
void ShutdownTiming();
void DestroyTiming();

void GetCurrentTime(TimeType *t);

unsigned long GetTimeDifference(const TimeType *t1, const TimeType *t2);

char *GetShortTimeString();
char *GetLongTimeString();

void SetClockFormat(const char *f);

#endif

