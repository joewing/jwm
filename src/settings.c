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

/** Initialize settings. */
void InitializeSettings()
{
   int i;
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

   if(JUNLIKELY(settings.desktopWidth == 0)) {
      settings.desktopWidth = 4;
   }
   if(JUNLIKELY(settings.desktopHeight == 0)) {
      settings.desktopHeight = 1;
   }
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

