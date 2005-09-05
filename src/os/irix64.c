/****************************************************************************
 * Irix64 dependent functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "os.h"
#include "../debug.h"

#include <unistd.h>
#include <sys/sysget.h>

/****************************************************************************
 * Initialize OS-dependent resources.
 ****************************************************************************/
void InitializeOSDependent() {
}

/****************************************************************************
 * Release OS-dependent resources.
 ****************************************************************************/
void DestroyOSDependent() {
}

/****************************************************************************
 * Get the current load average.
 ****************************************************************************/
float GetLoad() {

	struct sgt_cookie cookie;
	int load[3];

	SGT_COOKIE_INIT(&cookie);
	SGT_COOKIE_SET_KSYM(&cookie, KSYM_AVENRUN);
	if(sysget(SGT_KSYM, (char*)load, sizeof(load), SGT_READ, &cookie) == -1) {
		Debug("sysget failed in %s line %u", __FILE__, __LINE__);
		return 0.0;
	}

	return (float)load[0] / 1024.0;

}

