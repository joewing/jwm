/****************************************************************************
 * SunOS dependent functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "os.h"
#include "../debug.h"

#include <kstat.h>

static kstat_ctl_t *kc = NULL;

/****************************************************************************
 * Initialize OS-dependent resources.
 ****************************************************************************/
void InitializeOSDependent() {
}

/****************************************************************************
 * Release OS-dependent resources.
 ****************************************************************************/
void DestroyOSDependent() {
	if(kc) {
		kstat_close(kc);
		kc = NULL;
	}
}

/****************************************************************************
 * Get the current load average.
 ****************************************************************************/
float GetLoad() {

	kstat_t *ks;
	kstat_named_t *kn;

	if(!kc) {
		kc = kstat_open();
		if(!kc) {
			Debug("kstat_open failed in %s at line %u", __FILE__, __LINE__);
			return 0.0;
		}
	}

	kstat_chain_update(kc);
	ks = kstat_lookup(kc, "unix", 0, "system_misc");
	if(!ks) {
		Debug("kstat_lookup failed in %s at line %u", __FILE__, __LINE__);
		return 0.0;
	}
	if(kstat_read(kc, ks, 0) == -1) {
		Debug("kstat_read failed in %s at line %u", __FILE__, __LINE__);
		return 0.0;
	}

	kn = kstat_data_lookup(ks, "avenrun_1min");
	if(!kn) {
		Debug("kstat_data_lookup failed in %s at line %u", __FILE__, __LINE__);
		return 0.0;
	}

	return (float)kn->value.ui32 / 256.0;

}

