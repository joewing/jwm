/**
 * @file timing.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Timing functions.
 *
 */

#ifndef TIMING_H
#define TIMING_H

#define ZERO_TIME { 0, 0 }

typedef struct TimeType {

	unsigned long seconds;
	int ms;

} TimeType;

void GetCurrentTime(TimeType *t);

unsigned long GetTimeDifference(const TimeType *t1, const TimeType *t2);

const char *GetTimeString(const char *format);

#endif

