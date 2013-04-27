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

#ifdef USE_XFT
static const char *DEFAULT_FONT = "FreeSans-9";
#else
static const char *DEFAULT_FONT = "fixed";
#endif

static char *fontNames[FONT_COUNT];

#ifdef USE_XFT
static XftFont *fonts[FONT_COUNT];
static XftDraw *xd;
#else
static XFontStruct *fonts[FONT_COUNT];
static GC fontGC;
#endif

/** Initialize font data. */
void InitializeFonts()
{
   unsigned int x;
   for(x = 0; x < FONT_COUNT; x++) {
      fonts[x] = NULL;
      fontNames[x] = NULL;
   }
}

/** Startup font support. */
void StartupFonts()
{

#ifndef USE_XFT
   XGCValues gcValues;
   unsigned long gcMask;
#endif
   unsigned int x;

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
         if(JUNLIKELY(!fonts[x])) {
            Warning(_("could not load font: %s"), fontNames[x]);
         }
      }
      if(!fonts[x]) {
         fonts[x] = JXftFontOpenName(display, rootScreen, DEFAULT_FONT);
      }
      if(JUNLIKELY(!fonts[x])) {
         FatalError(_("could not load the default font: %s"), DEFAULT_FONT);
      }
   }

   xd = XftDrawCreate(display, rootWindow, rootVisual, rootColormap);

#else /* USE_XFT */

   for(x = 0; x < FONT_COUNT; x++) {
      if(fontNames[x]) {
         fonts[x] = JXLoadQueryFont(display, fontNames[x]);
         if(JUNLIKELY(!fonts[x] && fontNames[x])) {
            Warning(_("could not load font: %s"), fontNames[x]);
         }
      }
      if(!fonts[x]) {
         fonts[x] = JXLoadQueryFont(display, DEFAULT_FONT);
      }
      if(JUNLIKELY(!fonts[x])) {
         FatalError(_("could not load the default font: %s"), DEFAULT_FONT);
      }
   }

   gcMask = GCGraphicsExposures;
   gcValues.graphics_exposures = False;
   fontGC = JXCreateGC(display, rootWindow, gcMask, &gcValues);

#endif /* USE_XFT */

}

/** Shutdown font support. */
void ShutdownFonts()
{
   unsigned int x;
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

#ifdef USE_XFT
   XftDrawDestroy(xd);
#else
   JXFreeGC(display, fontGC);
#endif
}

/** Destroy font data. */
void DestroyFonts()
{
   unsigned int x;
   for(x = 0; x < FONT_COUNT; x++) {
      if(fontNames[x]) {
         Release(fontNames[x]);
         fontNames[x] = NULL;
      }
   }
}

/** Get the width of a string. */
int GetStringWidth(FontType ft, const char *str)
{
#ifdef USE_XFT
   XGlyphInfo extents;
#endif
#ifdef USE_FRIBIDI
   FriBidiChar *temp;
   FriBidiParType type = FRIBIDI_PAR_ON;
   int unicodeLength;
#endif
   int len;
   char *output;
   int result;

   len = strlen(str);

   /* Apply the bidi algorithm if requested. */
#ifdef USE_FRIBIDI
   temp = AllocateStack((len + 1) * sizeof(FriBidiChar));
   unicodeLength = fribidi_charset_to_unicode(FRIBIDI_CHAR_SET_UTF8,
                                              (char*)str, len, temp);
   fribidi_log2vis(temp, unicodeLength, &type, temp, NULL, NULL, NULL);
   output = AllocateStack(4 * len + 1);
   fribidi_unicode_to_charset(FRIBIDI_CHAR_SET_UTF8, temp, unicodeLength,
                              (char*)output);
   len = strlen(output);
#else
   output = (char*)str;
#endif

   /* Get the width of the string. */
#ifdef USE_XFT
   JXftTextExtentsUtf8(display, fonts[ft], (const unsigned char*)output,
                       len, &extents);
   result = extents.xOff;
#else
   result = XTextWidth(fonts[ft], output, len);
#endif

   /* Clean up. */
#ifdef USE_FRIBIDI
   ReleaseStack(temp);
   ReleaseStack(output);
#endif

   return result;
}

/** Get the height of a string. */
int GetStringHeight(FontType ft)
{
   Assert(fonts[ft]);
   return fonts[ft]->ascent + fonts[ft]->descent;
}

/** Set the font to use for a component. */
void SetFont(FontType type, const char *value)
{
   if(JUNLIKELY(!value)) {
      Warning(_("empty Font tag"));
      return;
   }
   if(fontNames[type]) {
      Release(fontNames[type]);
   }
   fontNames[type] = CopyString(value);
}

/** Display a string. */
void RenderString(Drawable d, FontType font, ColorType color,
                  int x, int y, int width, const char *str)
{

   XRectangle rect;
   Region renderRegion;
   int len;
   char *output;
#ifdef USE_FRIBIDI
   FriBidiChar *temp;
   FriBidiParType type = FRIBIDI_PAR_ON;
   int unicodeLength;
#endif
#ifdef USE_XFT
   XGlyphInfo extents;
#endif

   if(!str) {
      return;
   }

   len = strlen(str);
   if(len == 0) {
      return;
   }

   /* Apply the bidi algorithm if requested. */
#ifdef USE_FRIBIDI
   temp = AllocateStack((len + 1) * sizeof(FriBidiChar));
   unicodeLength = fribidi_charset_to_unicode(FRIBIDI_CHAR_SET_UTF8,
                                              (char*)str, len, temp);
   fribidi_log2vis(temp, unicodeLength, &type, temp, NULL, NULL, NULL);
   output = AllocateStack(4 * len + 1);
   fribidi_unicode_to_charset(FRIBIDI_CHAR_SET_UTF8, temp, unicodeLength,
                              (char*)output);
   len = strlen(output);
#else
   output = (char*)str;
#endif

   /* Get the bounds for the string based on the specified width. */
   rect.x = x;
   rect.y = y;
   rect.height = GetStringHeight(font);
#ifdef USE_XFT
   JXftTextExtentsUtf8(display, fonts[font], (const unsigned char*)output,
                       len, &extents);
   rect.width = extents.xOff + 2;
#else
   rect.width = XTextWidth(fonts[font], output, len) + 2;
#endif

   /* Combine the width bounds with the region to use. */
   renderRegion = XCreateRegion();
   XUnionRectWithRegion(&rect, renderRegion, renderRegion);

   /* Display the string. */
#ifdef USE_XFT
   JXftDrawChange(xd, d);
   JXftDrawSetClip(xd, renderRegion);
   JXftDrawStringUtf8(xd, GetXftColor(color), fonts[font],
                      x, y + fonts[font]->ascent,
                      (const unsigned char*)output, len);
   JXftDrawChange(xd, rootWindow);
#else
   JXSetForeground(display, fontGC, colors[color]);
   JXSetRegion(display, fontGC, renderRegion);
   JXSetFont(display, fontGC, fonts[font]->fid);
   JXDrawString(display, d, fontGC, x, y + fonts[font]->ascent, output, len);
#endif

   /* Free any memory used for UTF conversion. */
#ifdef USE_FRIBIDI
   ReleaseStack(temp);
   ReleaseStack(output);
#endif

   XDestroyRegion(renderRegion);

}

