/**
 * @file settings.c
 * @author Joe Wingbermuehle
 * @date 2012
 */

#include "jwm.h"
#include "settings.h"
#include "misc.h"

Settings settings;

static void FixRange(unsigned int *value,
                     unsigned int min_value,
                     unsigned int max_value,
                     unsigned int def_value);

/** Initialize settings. */
void InitializeSettings(void)
{
   settings.doubleClickSpeed = 400;
   settings.doubleClickDelta = 2;
   settings.snapMode = SNAP_BORDER;
   settings.snapDistance = 5;
   settings.moveMode = MOVE_OPAQUE;
   settings.moveStatusType = SW_SCREEN;
   settings.resizeStatusType = SW_SCREEN;
   settings.focusModel = FOCUS_SLOPPY;
   settings.resizeMode = RESIZE_OPAQUE;
   settings.popupDelay = 600;
   settings.desktopDelay = 1000;
   settings.trayOpacity = UINT_MAX;
   settings.popupEnabled = 1;
   settings.activeClientOpacity = UINT_MAX;
   settings.inactiveClientOpacity = (unsigned int)(0.75 * UINT_MAX);
   settings.borderWidth = 5;
   settings.titleHeight = 22;
   settings.desktopWidth = 4;
   settings.desktopHeight = 1;
   settings.menuOpacity = UINT_MAX;
   settings.exitConfirmation = 1;
   settings.cornerRadius = 4;
   settings.handles = 0;
   settings.groupTasks = 0;
}

/** Make sure settings are reasonable. */
void StartupSettings(void)
{

   FixRange(&settings.cornerRadius, 0, 5, 4);

   FixRange(&settings.borderWidth, 1, 128, 4);
   FixRange(&settings.titleHeight, 2, 256, 20);

   FixRange(&settings.doubleClickDelta, 0, 64, 2);
   FixRange(&settings.doubleClickSpeed, 1, 2000, 400);

   FixRange(&settings.desktopWidth, 1, 64, 4);
   FixRange(&settings.desktopHeight, 1, 64, 1);
   settings.desktopCount = settings.desktopWidth * settings.desktopHeight;

}

/** Update a string setting. */
void SetPathString(char **dest, const char *src)
{
   if(*dest) {
      Release(*dest);
   }
   *dest = CopyString(src);
   if(JLIKELY(*dest)) {
      ExpandPath(dest);
   }
}

/** Make sure a value is in range. */
void FixRange(unsigned int *value,
              unsigned int min_value,
              unsigned int max_value,
              unsigned int def_value)
{
   if(JUNLIKELY(*value < min_value || *value > max_value)) {
      *value = def_value;
   }
}
