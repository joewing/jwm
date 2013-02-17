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

static void DisplayUsage();

/** Display program name, version, and compiled options . */
void DisplayAbout()
{
   printf("JWM v" PACKAGE_VERSION " by Joe Wingbermuehle\n");
   DisplayCompileOptions();
}

/** Display compiled options. */
void DisplayCompileOptions()
{
   printf("compiled options: "
#ifndef DISABLE_CONFIRM
          "confirm "
#endif
#ifdef DEBUG
          "debug "
#endif
#ifdef USE_FRIBIDI
          "fribidi "
#endif
#ifdef USE_ICONS
          "icons "
#endif
#ifdef USE_PNG
          "png "
#endif
#ifdef USE_SHAPE
          "shape "
#endif
#ifdef USE_XFT
          "xft "
#endif
#ifdef USE_XINERAMA
          "xinerama "
#endif
#ifdef USE_XPM
          "xpm "
#endif
#ifdef USE_XRENDER
          "xrender "
#endif
          "\nsystem configuration: " SYSTEM_CONFIG "\n");
}

/** Display all help. */
void DisplayHelp()
{
   DisplayUsage();
   printf("  -display X  Set the X display to use\n"
          "  -exit       Exit JWM (send _JWM_EXIT to the root)\n"
          "  -h          Display this help message\n"
          "  -p          Parse the configuration file and exit\n"
          "  -reload     Reload menu (send _JWM_RELOAD to the root)\n"
          "  -restart    Restart JWM (send _JWM_RESTART to the root)\n"
          "  -v          Display version information\n");
}

/** Display program usage information. */
void DisplayUsage()
{
   DisplayAbout();
   printf("usage: jwm [ options ]\n");
}

