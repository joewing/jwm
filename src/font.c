/**
 * @file font.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions to handle fonts.
 *
 */

#include "jwm.h"
#include "font.h"
#include "main.h"
#include "error.h"
#include "color.h"
#include "misc.h"

static const char *DEFAULT_FONT = "-*-courier-*-r-*-*-14-*-*-*-*-*-*-*";

static char *fontNames[FONT_COUNT];

#ifdef USE_XFT
static XftFont *fonts[FONT_COUNT];
#else
static XFontStruct *fonts[FONT_COUNT];
static GC fontGC;
#endif

/** Initialize font data. */
void InitializeFonts() {

   int x;

   for(x = 0; x < FONT_COUNT; x++) {
      fonts[x] = NULL;
      fontNames[x] = NULL;
   }

}

/** Startup font support. */
void StartupFonts() {

#ifndef USE_XFT
   XGCValues gcValues;
   unsigned long gcMask;
#endif
   int x;

   /* Inherit unset fonts from the tray for tray items. */
   if(!fontNames[FONT_TASK]) {
      fontNames[FONT_TASK] = CopyString(fontNames[FONT_TRAY]);
   }
   if(!fontNames[FONT_TRAYBUTTON]) {
      fontNames[FONT_TRAYBUTTON] = CopyString(fontNames[FONT_TRAY]);
   }
   if(!fontNames[FONT_CLOCK]) {
      fontNames[FONT_CLOCK] = CopyString(fontNames[FONT_TRAY]);
   }
   if(!fontNames[FONT_PAGER]) {
      fontNames[FONT_PAGER] = CopyString(fontNames[FONT_TRAY]);
   }

#ifdef USE_XFT

   for(x = 0; x < FONT_COUNT; x++) {
      if(fontNames[x]) {
         fonts[x] = JXftFontOpenName(display, rootScreen, fontNames[x]);
         if(!fonts[x]) {
            fonts[x] = JXftFontOpenXlfd(display, rootScreen, fontNames[x]);
         }
         if(!fonts[x]) {
            Warning("could not load font: %s", fontNames[x]);
         }
      }
      if(!fonts[x]) {
         fonts[x] = JXftFontOpenXlfd(display, rootScreen, DEFAULT_FONT);
      }
      if(!fonts[x]) {
         FatalError("could not load the default font: %s", DEFAULT_FONT);
      }
   }

#else /* USE_XFT */

   for(x = 0; x < FONT_COUNT; x++) {
      if(fontNames[x]) {
         fonts[x] = JXLoadQueryFont(display, fontNames[x]);
         if(!fonts[x] && fontNames[x]) {
            Warning("could not load font: %s", fontNames[x]);
         }
      }
      if(!fonts[x]) {
         fonts[x] = JXLoadQueryFont(display, DEFAULT_FONT);
      }
      if(!fonts[x]) {
         FatalError("could not load the default font: %s", DEFAULT_FONT);
      }
   }

   gcMask = GCGraphicsExposures;
   gcValues.graphics_exposures = False;
   fontGC = JXCreateGC(display, rootWindow, gcMask, &gcValues);

#endif /* USE_XFT */

}

/** Shutdown font support. */
void ShutdownFonts() {

   int x;

   for(x = 0; x < FONT_COUNT; x++) {
      if(fonts[x]) {
#ifdef USE_XFT
         JXftFontClose(display, fonts[x]);
#else
         JXFreeFont(display, fonts[x]);
#endif
         fonts[x] = NULL;
      }
   }

#ifndef USE_XFT

   JXFreeGC(display, fontGC);

#endif


}

/** Destroy font data. */
void DestroyFonts() {

   int x;

   for(x = 0; x < FONT_COUNT; x++) {
      if(fontNames[x]) {
         Release(fontNames[x]);
         fontNames[x] = NULL;
      }
   }

}

/** Get the width of a string. */
int GetStringWidth(FontType type, const char *str) {
#ifdef USE_XFT

   XGlyphInfo extents;
   unsigned int length;

   Assert(str);
   Assert(fonts[type]);

   length = strlen(str);

   JXftTextExtentsUtf8(display, fonts[type], (const unsigned char*)str,
      length, &extents);

   return extents.width;

#else

   Assert(str);
   Assert(fonts[type]);

   return XTextWidth(fonts[type], str, strlen(str));

#endif
}

/** Get the height of a string. */
int GetStringHeight(FontType type) {

   Assert(fonts[type]);

   return fonts[type]->ascent + fonts[type]->descent;

}

/** Set the font to use for a component. */
void SetFont(FontType type, const char *value) {

   if(!value) {
      Warning("empty Font tag");
      return;
   }

   if(fontNames[type]) {
      Release(fontNames[type]);
   }

   fontNames[type] = CopyString(value);

}

/** Display a string. */
void RenderString(Drawable d, FontType font, ColorType color,
   int x, int y, int width, Region region, const char *str) {

#ifdef USE_XFT
   XftDraw *xd;
#endif

   XRectangle rect;
   Region renderRegion;
   int len;
   char *output;

#ifdef USE_FRIBIDI

   FriBidiChar *temp;
   FriBidiCharType type = FRIBIDI_TYPE_ON;
   int unicodeLength;

#endif

   if(!str) {
      return;
   }

   len = strlen(str);
   if(len == 0) {
      return;
   }

   /* Get the bounds for the string based on the specified width. */
   rect.x = x;
   rect.y = y;
   rect.width = Min(GetStringWidth(font, str), width) + 2;
   rect.height = GetStringHeight(font);

   /* Create a region to use. */
   renderRegion = XCreateRegion();

   /* Combine the width bounds with the region to use. */
   XUnionRectWithRegion(&rect, renderRegion, renderRegion);

   /* Combine the provided region with the region to use. */
   if(region) {
      XIntersectRegion(region, renderRegion, renderRegion);
   }
   
   /* Apply the bidi algorithm if requested. */

#ifdef USE_FRIBIDI

   temp = AllocateStack((len + 1) * sizeof(FriBidiChar));
   unicodeLength = fribidi_utf8_to_unicode((char*)str, len, temp);

   fribidi_log2vis(temp, unicodeLength, &type, temp, NULL, NULL, NULL);

   fribidi_unicode_to_utf8(temp, len, (char*)temp);
   output = (char*)temp;

#else

   output = (char*)str;

#endif

   /* Display the string. */

#ifdef USE_XFT

   xd = XftDrawCreate(display, d, rootVisual, rootColormap);
   XftDrawSetClip(xd, renderRegion);
   JXftDrawStringUtf8(xd, GetXftColor(color), fonts[font],
      x, y + fonts[font]->ascent, (const unsigned char*)output, len);
   XftDrawDestroy(xd);

#else

   JXSetForeground(display, fontGC, colors[color]);
   XSetRegion(display, fontGC, renderRegion);
   JXSetFont(display, fontGC, fonts[font]->fid);
   JXDrawString(display, d, fontGC, x, y + fonts[font]->ascent, output, len);

#endif

   /* Free any memory used for UTF conversion. */

#ifdef USE_FRIBIDI

   ReleaseStack(output);

#endif

   XDestroyRegion(renderRegion);

}

