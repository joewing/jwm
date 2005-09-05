/****************************************************************************
 * Darwin dependent functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "os.h"
#include "../debug.h"

#include <stdlib.h>

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

	double avgload;

	if(getloadavg(&avgload, 1) < 0) {
		Debug("getavgload failed in %s at line %u", __FILE__, __LINE__);
		return 0.0;
	}

	return (float)avgload;

}

