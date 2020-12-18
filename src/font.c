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
#include "misc.h"

#ifdef USE_PANGO
#  include <pango/pango.h>
#  include <pango/pangoxft.h>
#  include <pango/pangofc-fontmap.h>
#  include <fontconfig/fontconfig.h>
#endif

#ifdef USE_ICONV
#  ifdef HAVE_LANGINFO_H
#     include <langinfo.h>
#  endif
#  ifdef HAVE_ICONV_H
#     include <iconv.h>
#  endif
#endif

#ifdef USE_PANGO
static const int DEFAULT_SIZE = 12;
static const char *DEFAULT_FONT = "12";
#else
static const char *DEFAULT_FONT = "fixed";
#endif

static const struct {
   const FontType src;
   const FontType dest;
} INHERITED_FONTS[] = {
   { FONT_TRAY, FONT_PAGER       },
   { FONT_TRAY, FONT_CLOCK       },
   { FONT_TRAY, FONT_TASKLIST    },
   { FONT_TRAY, FONT_TRAYBUTTON  }
};

static char *GetUTF8String(const char *str);
static void ReleaseUTF8String(char *utf8String);

#ifdef USE_ICONV
static const char *UTF8_CODESET = "UTF-8";
static iconv_t fromUTF8 = (iconv_t)-1;
static iconv_t toUTF8 = (iconv_t)-1;
#endif

#ifdef USE_PANGO
static PangoFontDescription *fonts[FONT_COUNT];
static PangoLayout *layouts[FONT_COUNT];
static int font_heights[FONT_COUNT];
static int font_ascents[FONT_COUNT];
static PangoFontMap *font_map;
static PangoContext *font_context;
#else
static char *fontNames[FONT_COUNT];
static XFontStruct *fonts[FONT_COUNT];
#endif

/** Initialize font data. */
void InitializeFonts(void)
{
#ifdef USE_ICONV
   const char *codeset;
#endif
   unsigned int x;

   for(x = 0; x < FONT_COUNT; x++) {
      fonts[x] = NULL;
#ifndef USE_PANGO
      fontNames[x] = NULL;
#endif
   }

   /* Allocate a conversion descriptor if we're not using UTF-8. */
#ifdef USE_ICONV
   codeset = nl_langinfo(CODESET);
   if(strcmp(codeset, UTF8_CODESET)) {
      toUTF8 = iconv_open(UTF8_CODESET, codeset);
      fromUTF8 = iconv_open(codeset, UTF8_CODESET);
   } else {
      toUTF8 = (iconv_t)-1;
      fromUTF8 = (iconv_t)-1;
   }
#endif

}

/** Startup font support. */
void StartupFonts(void)
{

   unsigned int x;

   /* Inherit unset fonts from the tray for tray items. */
   for(x = 0; x < ARRAY_LENGTH(INHERITED_FONTS); x++) {
      const FontType dest = INHERITED_FONTS[x].dest;
#ifdef USE_PANGO
      if(!fonts[dest]) {
         const FontType src = INHERITED_FONTS[x].src;
         fonts[dest] = pango_font_description_copy(fonts[src]);
      }
#else
      if(!fontNames[dest]) {
         const FontType src = INHERITED_FONTS[x].src;
         fontNames[dest] = CopyString(fontNames[src]);
      }
#endif
   }

#ifdef USE_PANGO
  font_map = pango_xft_get_font_map(display, rootScreen);
  font_context = pango_font_map_create_context(font_map);
#endif

   for(x = 0; x < FONT_COUNT; x++) {
#ifdef USE_PANGO
      if(!fonts[x]) {
         fonts[x] = pango_font_description_from_string(DEFAULT_FONT);
      }
      if(JLIKELY(fonts[x])) {
        PangoFontMetrics *metrics;

        layouts[x] = pango_layout_new(font_context);
        pango_layout_set_font_description(layouts[x], fonts[x]);
        pango_layout_set_single_paragraph_mode(layouts[x], TRUE);
        pango_layout_set_width(layouts[x], -1);
        pango_layout_set_ellipsize(layouts[x], PANGO_ELLIPSIZE_MIDDLE);

        metrics = pango_context_get_metrics(font_context, fonts[x], NULL);
        font_ascents[x] = pango_font_metrics_get_ascent(metrics);
         font_heights[x] = font_ascents[x]
            + pango_font_metrics_get_descent(metrics);
        pango_font_metrics_unref(metrics);

        pango_font_description_free(fonts[x]);
        
      } else {
        font_ascents[x] = 0;
        font_heights[x] = 0;
      }
#else /* USE_PANGO */
      if(fontNames[x]) {
         fonts[x] = JXLoadQueryFont(display, fontNames[x]);
         if(JUNLIKELY(!fonts[x] && fontNames[x])) {
            Warning(_("could not load font: %s"), fontNames[x]);
         }
      }
      if(!fonts[x]) {
         fonts[x] = JXLoadQueryFont(display, DEFAULT_FONT);
      }
#endif /* USE_PANGO */
      if(JUNLIKELY(!fonts[x])) {
         FatalError(_("could not load the default font: %s"), DEFAULT_FONT);
      }
   }

}

/** Shutdown font support. */
void ShutdownFonts(void)
{
   unsigned int x;
   for(x = 0; x < FONT_COUNT; x++) {
#ifdef USE_PANGO
      if(layouts[x]) {
         g_object_unref(layouts[x]);
         layouts[x] = NULL;
      }
#else
      if(fonts[x]) {
         JXFreeFont(display, fonts[x]);
         fonts[x] = NULL;
      }
#endif
   }

#ifdef USE_PANGO
   g_object_unref(font_context);
#endif
}

/** Destroy font data. */
void DestroyFonts(void)
{
#ifndef USE_PANGO
   unsigned int x;
   for(x = 0; x < FONT_COUNT; x++) {
      if(fontNames[x]) {
         Release(fontNames[x]);
         fontNames[x] = NULL;
      }
   }
#endif
#ifdef USE_ICONV
   if(fromUTF8 != (iconv_t)-1) {
      iconv_close(fromUTF8);
      fromUTF8 = (iconv_t)-1;
   }
   if(toUTF8 != (iconv_t)-1) {
      iconv_close(toUTF8);
      toUTF8 = (iconv_t)-1;
   }
#endif
}

/** Convert a string from UTF-8. */
char *ConvertFromUTF8(char *str)
{
   char *result = (char*)str;
#ifdef USE_ICONV
   if(fromUTF8 != (iconv_t)-1) {
      ICONV_CONST char *inBuf = (ICONV_CONST char*)str;
      char *outBuf;
      size_t inLeft = strlen(str);
      size_t outLeft = 4 * inLeft;
      size_t rc;
      result = Allocate(outLeft + 1);
      outBuf = result;
      rc = iconv(fromUTF8, &inBuf, &inLeft, &outBuf, &outLeft);
      if(rc == (size_t)-1) {
         Warning("iconv conversion from UTF-8 failed");
         iconv_close(fromUTF8);
         fromUTF8 = (iconv_t)-1;
         Release(result);
         result = (char*)str;
      } else {
         Release(str);
         *outBuf = 0;
      }
   }
#endif
   return result;
}

/** Convert a string to UTF-8. */
char *GetUTF8String(const char *str)
{
   char *utf8String = (char*)str;
#ifdef USE_ICONV
   if(toUTF8 != (iconv_t)-1) {
      ICONV_CONST char *inBuf = (ICONV_CONST char*)str;
      char *outBuf;
      size_t inLeft = strlen(str);
      size_t outLeft = 4 * inLeft;
      size_t rc;
      utf8String = Allocate(outLeft + 1);
      outBuf = utf8String;
      rc = iconv(toUTF8, &inBuf, &inLeft, &outBuf, &outLeft);
      if(rc == (size_t)-1) {
         Warning("iconv conversion to UTF-8 failed");
         iconv_close(toUTF8);
         toUTF8 = (iconv_t)-1;
         Release(utf8String);
         utf8String = (char*)str;
      } else {
         *outBuf = 0;
      }
   }
#endif
   return utf8String;
}

/** Release a UTF-8 string. */
void ReleaseUTF8String(char *utf8String)
{
#ifdef USE_ICONV
   if(toUTF8 != (iconv_t)-1) {
      Release(utf8String);
   }
#endif
}

/** Get the width of a string. */
int GetStringWidth(FontType ft, const char *str)
{
   char *utf8String;
   int result;

#ifdef USE_PANGO
  PangoRectangle rect;
#endif

   /* Convert to UTF-8 if necessary. */
   utf8String = GetUTF8String(str);

#ifdef USE_PANGO
   pango_layout_set_text(layouts[ft], utf8String, -1);
   pango_layout_set_width(layouts[ft], -1);
   pango_layout_get_extents(layouts[ft], NULL, &rect);
   result = (rect.width + PANGO_SCALE - 1) / PANGO_SCALE;
#else
   result = XTextWidth(fonts[ft], utf8String, strlen(utf8String));
#endif

   /* Clean up. */
   ReleaseUTF8String(utf8String);

   return result;
}

/** Get the height of a string. */
int GetStringHeight(FontType ft)
{
#ifdef USE_PANGO
   return PANGO_PIXELS(font_heights[ft]);
#else
   return fonts[ft]->ascent + fonts[ft]->descent;
#endif
}

/** Set the font to use for a component. */
void SetFont(FontType type, const char *name)
{
#ifdef USE_PANGO
   FcPattern *pattern;
#endif

   if(!name) {
      Warning(_("empty Font tag"));
      return;
   }

#ifdef USE_PANGO

   if(fonts[type]) {
      pango_font_description_free(fonts[type]);
   }
   pattern = FcNameParse((const FcChar8*)name);
   if(pattern) {
      fonts[type] = pango_fc_font_description_from_pattern(pattern, TRUE);
      FcPatternDestroy(pattern);
      if(pango_font_description_get_size(fonts[type]) == 0) {
         /* Pick a value if no size was specified. */
         pango_font_description_set_size(fonts[type],
            DEFAULT_SIZE * PANGO_SCALE);
      }
   } else {
      fonts[type] = pango_font_description_new();
   }

#else

   if(fontNames[type]) {
      Release(fontNames[type]);
   }
   fontNames[type] = CopyString(name);

#endif
}

/** Display a string. */
void RenderString(Drawable d, FontType font, ColorType color,
                  int x, int y, int width, const char *str)
{
   char *utf8String;
#ifdef USE_PANGO
   XftDraw *xd;
   PangoLayoutLine *line;
   XftColor *xc;
#else
   XRectangle rect;
   Region renderRegion;
   int len;
   XGCValues gcValues;
   unsigned long gcMask;
   GC gc;
#endif

   /* Early return for empty strings. */
   if(!str || !str[0]) {
      return;
   }

   /* Convert to UTF-8 if necessary. */
   utf8String = GetUTF8String(str);

#ifdef USE_PANGO

   pango_layout_set_text(layouts[font], str, -1);
   pango_layout_set_width(layouts[font], width * PANGO_SCALE);

   xd = XftDrawCreate(display, d, rootVisual, rootColormap);
   xc = GetXftColor(color);
   line = pango_layout_get_line_readonly(layouts[font], 0);
   pango_xft_render_layout_line(xd, xc, line, x * PANGO_SCALE,
      y * PANGO_SCALE + font_ascents[font]);

   XftDrawDestroy(xd);
#else

   /* Get the length of the UTF-8 string. */
   len = strlen(utf8String);

   gcMask = GCGraphicsExposures;
   gcValues.graphics_exposures = False;
   gc = JXCreateGC(display, d, gcMask, &gcValues);

   /* Get the bounds for the string based on the specified width. */
   rect.x = x;
   rect.y = y;
   rect.height = GetStringHeight(font);
   rect.width = XTextWidth(fonts[font], utf8String, len);
   rect.width = Min(rect.width, width) + 2;

   /* Combine the width bounds with the region to use. */
   renderRegion = XCreateRegion();
   XUnionRectWithRegion(&rect, renderRegion, renderRegion);

   /* Display the string. */
   JXSetForeground(display, gc, colors[color]);
   JXSetRegion(display, gc, renderRegion);
   JXSetFont(display, gc, fonts[font]->fid);
   JXDrawString(display, d, gc, x, y + fonts[font]->ascent, utf8String, len);

   XDestroyRegion(renderRegion);
   JXFreeGC(display, gc);
#endif

   /* Free any memory used for UTF conversion. */
   ReleaseUTF8String(utf8String);

}
