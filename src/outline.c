/**
 * @file outline.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Outlines for moving and resizing client windows.
 *
 */

#include "jwm.h"
#include "outline.h"
#include "main.h"

static GC outlineGC;

static int lastX, lastY;
static int lastWidth, lastHeight;
static int outlineDrawn;

/** Initialize outline data. */
void InitializeOutline() {
}

/** Startup outlines. */
void StartupOutline() {

   XGCValues gcValues;

   gcValues.function = GXinvert;
   gcValues.subwindow_mode = IncludeInferiors;
   gcValues.line_width = 2;
   outlineGC = JXCreateGC(display, rootWindow,
      GCFunction | GCSubwindowMode | GCLineWidth, &gcValues);
   outlineDrawn = 0;

}

/** Shutdown outlines. */
void ShutdownOutline() {
   JXFreeGC(display, outlineGC);
}

/** Release outline data. */
void DestroyOutline() {
}

/** Draw an outline. */
void DrawOutline(int x, int y, int width, int height) {
   if(!outlineDrawn) {
      JXSync(display, False);
      JXGrabServer(display);
      JXDrawRectangle(display, rootWindow, outlineGC, x, y, width, height);
      lastX = x;
      lastY = y;
      lastWidth = width;
      lastHeight = height;
      outlineDrawn = 1;
   }
}

/** Clear the last outline. */
void ClearOutline() {
   if(outlineDrawn) {
      JXDrawRectangle(display, rootWindow, outlineGC,
         lastX, lastY, lastWidth, lastHeight);
      outlineDrawn = 0;
      JXUngrabServer(display);
      JXSync(display, False);
   }
}

