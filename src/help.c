/****************************************************************************
 * Functions for displaying information about JWM.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"

/****************************************************************************
 ****************************************************************************/
void DisplayAbout() {
	printf("JWM v%s by Joe Wingbermuehle\n", VERSION);
	DisplayCompileOptions();
}

/****************************************************************************
 ****************************************************************************/
void DisplayCompileOptions() {

	printf("compiled options: ");

#ifndef DISABLE_CONFIRM
	printf("confirm ");
#endif

#ifdef DEBUG
	printf("debug ");
#endif

#ifdef USE_ICONS
	printf("icons ");
#endif

#ifdef SHOW_LOAD
	printf("load ");
#endif

#ifdef USE_SHAPE
	printf("shape ");
#endif

#ifdef USE_XINERAMA
	printf("xinerama ");
#endif

	printf("\nsystem configuration: %s\n", SYSTEM_CONFIG);

}

/****************************************************************************
 ****************************************************************************/
void DisplayHelp() {
	DisplayUsage();
	printf("  -display X  Set the X display to use\n");
	printf("  -exit       Exit JWM (send _JWM_EXIT to the root)\n");
	printf("  -h          Display this help message\n");
	printf("  -p          Parse the configuration file and exit\n");
	printf("  -restart    Restart JWM (send _JWM_RESTART to the root)\n");
	printf("  -v          Display version information\n");
}

/****************************************************************************
 ****************************************************************************/
void DisplayUsage() {
	DisplayAbout();
	printf("usage: jwm [ options ]\n");
}

