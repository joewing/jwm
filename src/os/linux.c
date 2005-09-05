/****************************************************************************
 * Linux dependent functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "os.h"
#include "../debug.h"

#include <sys/sysinfo.h>

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

	struct sysinfo info;

	if(sysinfo(&info) == -1) {
		Debug("sysinfo failed in %s at line %u", __FILE__, __LINE__);
		return 0.0;
	}

	return (float)info.loads[0] / 65536.0;

}

