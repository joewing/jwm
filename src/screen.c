/**
 * @file screen.c
 * @author Joe Wingbermuehle
 * @date 2005-2006
 *
 * @brief Screen functions.
 *
 * Note that screen here refers to physical monitors. Screens are
 * determined using the xinerama extension (if available). There will
 * always be at least one screen.
 *
 */

#include "jwm.h"
#include "screen.h"
#include "main.h"
#include "cursor.h"
#include "misc.h"

static ScreenType *screens = NULL;
static int screenCount;

#ifdef USE_XRANDR
/** Name screens after the RandR outputs driving them.
 * Each active CRTC rectangle is matched to the Xinerama screen with
 * the same geometry (modern servers synthesize Xinerama data from the
 * CRTC configuration, so an exact match is expected). */
static void NameScreens(void)
{
   XRRScreenResources *res;
   RROutput primary;
   int eventBase, errorBase;
   int i, x;

   if(!XRRQueryExtension(display, &eventBase, &errorBase)) {
      return;
   }
   res = XRRGetScreenResourcesCurrent(display, rootWindow);
   if(!res) {
      return;
   }
   primary = XRRGetOutputPrimary(display, rootWindow);
   for(i = 0; i < res->noutput; i++) {
      XRROutputInfo *output;
      output = XRRGetOutputInfo(display, res, res->outputs[i]);
      if(!output) {
         continue;
      }
      if(output->connection == RR_Connected && output->crtc != None) {
         XRRCrtcInfo *crtc = XRRGetCrtcInfo(display, res, output->crtc);
         if(crtc) {
            for(x = 0; x < screenCount; x++) {
               if(screens[x].name == NULL
                  && screens[x].x == crtc->x
                  && screens[x].y == crtc->y
                  && screens[x].width == (int)crtc->width
                  && screens[x].height == (int)crtc->height) {
                  screens[x].name = CopyString(output->name);
                  screens[x].primary = res->outputs[i] == primary;
                  Debug("screen %d: %s%s", x, output->name,
                        screens[x].primary ? " (primary)" : "");
                  break;
               }
            }
            XRRFreeCrtcInfo(crtc);
         }
      }
      XRRFreeOutputInfo(output);
   }
   XRRFreeScreenResources(res);
}
#endif /* USE_XRANDR */

/** Startup screens. */
void StartupScreens(void)
{
#ifdef USE_XINERAMA

   XineramaScreenInfo *info;
   int x;

   if(XineramaIsActive(display)) {

      info = XineramaQueryScreens(display, &screenCount);

      screens = Allocate(sizeof(ScreenType) * screenCount);
      for(x = 0; x < screenCount; x++) {
         screens[x].index = x;
         screens[x].x = info[x].x_org;
         screens[x].y = info[x].y_org;
         screens[x].width = info[x].width;
         screens[x].height = info[x].height;
         screens[x].name = NULL;
         screens[x].primary = 0;
      }

      JXFree(info);

#ifdef USE_XRANDR
      NameScreens();
#endif

   } else {

      screenCount = 1;
      screens = Allocate(sizeof(ScreenType));
      screens->index = 0;
      screens->x = 0;
      screens->y = 0;
      screens->width = rootWidth;
      screens->height = rootHeight;
      screens->name = NULL;
      screens->primary = 1;

   }

#else

   screenCount = 1;
   screens = Allocate(sizeof(ScreenType));
   screens->index = 0;
   screens->x = 0;
   screens->y = 0;
   screens->width = rootWidth;
   screens->height = rootHeight;
   screens->name = NULL;
   screens->primary = 1;

#endif /* USE_XINERAMA */
}

/** Shutdown screens. */
void ShutdownScreens(void)
{
   if(screens) {
      int x;
      for(x = 0; x < screenCount; x++) {
         if(screens[x].name) {
            Release(screens[x].name);
         }
      }
      Release(screens);
      screens = NULL;
   }
}

/** Find a screen by RandR output name. */
int FindScreenByName(const char *name)
{
   int x;
   Assert(name);
   for(x = 0; x < screenCount; x++) {
      if(screens[x].name && !strcmp(screens[x].name, name)) {
         return x;
      }
   }
   if(!strcmp(name, "primary")) {
      for(x = 0; x < screenCount; x++) {
         if(screens[x].primary) {
            return x;
         }
      }
   }
   return -1;
}

/** Get the screen given global screen coordinates. */
const ScreenType *GetCurrentScreen(int x, int y)
{

   ScreenType *sp;
   int index;

   x = Max(0, x);
   x = Min(x, rootWidth - 1);
   y = Max(0, y);
   y = Min(y, rootHeight - 1);
   for(index = 1; index < screenCount; index++) {
      sp = &screens[index];
      if(x >= sp->x && x < sp->x + sp->width) {
         if(y >= sp->y && y < sp->y + sp->height) {
            return sp;
         }
      }
   }

   return &screens[0];

}

/** Get the screen with the largest overlap with a rectangle. */
const ScreenType *GetDominantScreen(int x, int y, int width, int height)
{

   const ScreenType *best;
   int bestArea;
   int index;

   best = NULL;
   bestArea = 0;
   for(index = 0; index < screenCount; index++) {
      const ScreenType *sp = &screens[index];
      const int ix = Max(x, sp->x);
      const int iy = Max(y, sp->y);
      const int iw = Min(x + width, sp->x + sp->width) - ix;
      const int ih = Min(y + height, sp->y + sp->height) - iy;
      if(iw > 0 && ih > 0) {
         const int area = iw * ih;
         if(area > bestArea) {
            bestArea = area;
            best = sp;
         }
      }
   }
   if(best) {
      return best;
   }

   /* No overlap with any screen; fall back to the midpoint. */
   return GetCurrentScreen(x + width / 2, y + height / 2);

}

/** Get the screen the mouse is currently on. */
const ScreenType *GetMouseScreen(void)
{
#ifdef USE_XINERAMA

   Window w;
   int x, y;

   GetMousePosition(&x, &y, &w);
   return GetCurrentScreen(x, y);

#else

   return &screens[0];

#endif
}

/** Get data for a screen. */
const ScreenType *GetScreen(int index)
{
   if(index >= 0 && index < screenCount) {
      return &screens[index];
   } else {
      return &screens[0];
   }
}

/** Get the number of screens. */
int GetScreenCount(void)
{
   return screenCount;
}


