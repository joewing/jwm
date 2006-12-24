/**
 * @file help.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions for displaying information about JWM.
 *
 */

#include "jwm.h"
#include "help.h"

/** Display program name, version, and compiled options . */
void DisplayAbout() {
   printf("JWM v%s by Joe Wingbermuehle\n", PACKAGE_VERSION);
   DisplayCompileOptions();
}

/** Display compiled options. */
void DisplayCompileOptions() {

   printf("compiled options: ");

#ifndef DISABLE_CONFIRM
   printf("confirm ");
#endif

#ifdef DEBUG
   printf("debug ");
#endif

#ifdef USE_FRIBIDI
   printf("fribidi ");
#endif

#ifdef USE_ICONS
   printf("icons ");
#endif

#ifdef USE_PNG
   printf("png ");
#endif

#ifdef USE_SHAPE
   printf("shape ");
#endif

#ifdef USE_XFT
   printf("xft ");
#endif

#ifdef USE_XINERAMA
   printf("xinerama ");
#endif

#ifdef USE_XPM
   printf("xpm ");
#endif

#ifdef USE_XRENDER
   printf("xrender ");
#endif

   printf("\nsystem configuration: %s\n", SYSTEM_CONFIG);

}

/** Display all help. */
void DisplayHelp() {
   DisplayUsage();
   printf("  -display X  Set the X display to use\n");
   printf("  -exit       Exit JWM (send _JWM_EXIT to the root)\n");
   printf("  -h          Display this help message\n");
   printf("  -p          Parse the configuration file and exit\n");
   printf("  -restart    Restart JWM (send _JWM_RESTART to the root)\n");
   printf("  -v          Display version information\n");
}

/** Display program usage information. */
void DisplayUsage() {
   DisplayAbout();
   printf("usage: jwm [ options ]\n");
}

