/**
 * @file settings.c
 * @author Joe Wingbermuehle
 * @date 2012
 */

#include "jwm.h"
#include "settings.h"
#include "debug.h"
#include "misc.h"

Settings settings;

static void FixRange(unsigned int *value,
                     unsigned int min_value,
                     unsigned int max_value,
                     unsigned int def_value);

/** Initialize settings. */
void InitializeSettings()
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
   settings.minClientOpacity = (unsigned int)(0.5 * UINT_MAX);
   settings.maxClientOpacity = (unsigned int)(0.9 * UINT_MAX);
   settings.deltaClientOpacity = (unsigned int)(0.1 * UINT_MAX);
   settings.borderWidth = 4;
   settings.titleHeight = 20;
   settings.desktopWidth = 4;
   settings.desktopHeight = 1;
   settings.menuOpacity = UINT_MAX;
   settings.taskInsertMode = INSERT_RIGHT;
}

/** Make sure settings are reasonable. */
void StartupSettings()
{
   unsigned int temp;

   if(settings.minClientOpacity > settings.maxClientOpacity) {
      temp = settings.minClientOpacity;
      settings.minClientOpacity = settings.maxClientOpacity;
      settings.maxClientOpacity = temp;
   }

   FixRange(&settings.borderWidth, 1, 128, 4);
   FixRange(&settings.titleHeight, 2, 256, 20);

   FixRange(&settings.doubleClickDelta, 0, 64, 2);
   FixRange(&settings.doubleClickSpeed, 1, 2000, 400);

   FixRange(&settings.desktopWidth, 1, 64, 4);
   FixRange(&settings.desktopHeight, 1, 64, 1);
   settings.desktopCount = settings.desktopWidth * settings.desktopHeight;

}

/** Prepare to destroy settings. */
void ShutdownSettings()
{
}

/** Free memory associated with settings. */
void DestroySettings()
{
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

