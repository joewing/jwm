/****************************************************************************
 * Timing functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"

static const unsigned long MAX_TIME_SECONDS = 60;

static char *clockFormat = NULL;
static int clockEnabled = 1;

/****************************************************************************
 ****************************************************************************/
void InitializeTiming() {
	clockFormat = NULL;
}

/****************************************************************************
 ****************************************************************************/
void StartupTiming() {
}

/****************************************************************************
 ****************************************************************************/
void ShutdownTiming() {
}

/****************************************************************************
 ****************************************************************************/
void DestroyTiming() {
	if(clockFormat) {
		Release(clockFormat);
		clockFormat = NULL;
	}
}

/****************************************************************************
 ****************************************************************************/
void SetClockFormat(const char *f) {
	if(!f || strlen(f) == 0) {
		if(clockFormat) {
			Release(clockFormat);
			clockFormat = NULL;
		}
	} else {
		clockFormat = Allocate(strlen(f) + 1);
		strcpy(clockFormat, f);
	}
}

/****************************************************************************
 ****************************************************************************/
void SetClockEnabled(int e) {
	clockEnabled = e;
}

/****************************************************************************
 * Get the current time in milliseconds since midnight 1970-01-01 UTC.
 ****************************************************************************/
void GetCurrentTime(TimeType *t) {
	struct timeval val;
	gettimeofday(&val, NULL);
	t->seconds = val.tv_sec;
	t->ms = val.tv_usec / 1000;
}

/****************************************************************************
 * Get the absolute difference between two times in milliseconds.
 * If the difference is larger than a MAX_TIME_SECONDS, then
 * MAX_TIME_SECONDS will be returned.
 * Note that the times must be normalized.
 ****************************************************************************/
unsigned long GetTimeDifference(const TimeType *t1, const TimeType *t2) {
	unsigned long deltaSeconds;
	int deltaMs;

	if(t1->seconds > t2->seconds) {
		deltaSeconds = t1->seconds - t2->seconds;
		deltaMs = t1->ms - t2->ms;
	} else if(t1->seconds < t2->seconds) {
		deltaSeconds = t2->seconds - t1->seconds;
		deltaMs = t2->ms - t1->ms;
	} else if(t1->ms > t2->ms) {
		deltaSeconds = 0;
		deltaMs = t1->ms - t2->ms;
	} else {
		deltaSeconds = 0;
		deltaMs = t2->ms - t1->ms;
	}

	if(deltaSeconds > MAX_TIME_SECONDS) {
		return MAX_TIME_SECONDS * 1000;
	} else {
		return deltaSeconds * 1000 + deltaMs;
	}

}

/****************************************************************************
 * Not reentrant
 ****************************************************************************/
char *GetShortTimeString() {
	static char str[MAX_CLOCK_LENGTH + 1];
	time_t clock;
	int x, len;
	const char *f;

	if(clockEnabled) {

		if(clockFormat) {
			f = clockFormat;
		} else {
			f = DEFAULT_CLOCK_FORMAT;
		}

		clock = time(NULL);
		len = strftime(str, sizeof(str), f, localtime(&clock));

		if(str[0] == '0') {
			for(x = 0; x < len; x++) {
				str[x] = str[x + 1];
			}
		}

	} else {
		str[0] = 0;
	}

	return str;
}

/****************************************************************************
 * Not reentrant
 ****************************************************************************/
char *GetLongTimeString() {
	static char str[80];
	time_t clock;

	clock = time(NULL);
	strftime(str, sizeof(str), "%a %b %d %Y %H:%M:%S", localtime(&clock));

	return str;
}


