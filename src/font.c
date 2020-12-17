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
static const char *DEFAULT_FONT = "sans 9";
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

#ifdef USE_PANGO
static const StringMappingType STRETCH_MAP[] = {
   { "condensed",          PANGO_STRETCH_CONDENSED          },
   { "expanded",           PANGO_STRETCH_EXPANDED           },
   { "extra_condensed",    PANGO_STRETCH_EXTRA_CONDENSED    },
   { "normal",             PANGO_STRETCH_NORMAL             },
   { "ultra_condensed",    PANGO_STRETCH_ULTRA_CONDENSED    }
};
static const unsigned STRETCH_MAP_COUNT = ARRAY_LENGTH(STRETCH_MAP);

static const StringMappingType STYLE_MAP[] = {
   { "italic",    PANGO_STYLE_ITALIC   },
   { "normal",    PANGO_STYLE_NORMAL   },
   { "oblique",   PANGO_STYLE_OBLIQUE  }
};
static const unsigned STYLE_MAP_COUNT = ARRAY_LENGTH(STYLE_MAP);

static const StringMappingType VARIANT_MAP[] = {
   { "normal",       PANGO_VARIANT_NORMAL       },
   { "small_caps",   PANGO_VARIANT_SMALL_CAPS   }
};
static const unsigned VARIANT_MAP_COUNT = ARRAY_LENGTH(VARIANT_MAP);

static const StringMappingType WEIGHT_MAP[] = {
   { "bold",         PANGO_WEIGHT_BOLD          },
   { "heavy",        PANGO_WEIGHT_HEAVY         },
   { "light",        PANGO_WEIGHT_LIGHT         },
   { "normal",       PANGO_WEIGHT_NORMAL        },
   { "semibold",     PANGO_WEIGHT_SEMIBOLD      },
   { "ultrabold",    PANGO_WEIGHT_ULTRABOLD     },
   { "ultralight",   PANGO_WEIGHT_ULTRALIGHT    }
};
static const unsigned WEIGHT_MAP_COUNT = ARRAY_LENGTH(WEIGHT_MAP);
#endif

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
static PangoLanguage *language;
#else
static char *fontNames[FONT_COUNT];
static XFontStruct *fonts[FONT_COUNT];
#endif

#ifndef USE_PANGO
static unsigned XlfdLen(const char *src);
static void XlfdCat(char *dest, const char *src);
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
  language = pango_language_get_default();
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

        metrics = pango_context_get_metrics(font_context, fonts[x], language);
        font_ascents[x] = pango_font_metrics_get_ascent(metrics);
         font_heights[x] = font_ascents[x]
            + pango_font_metrics_get_descent(metrics);
        pango_font_metrics_unref(metrics);
        
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
      if(fonts[x]) {
#ifdef USE_PANGO
        g_object_unref(layouts[x]);
        pango_font_description_free(fonts[x]);
#else
         JXFreeFont(display, fonts[x]);
#endif
         fonts[x] = NULL;
      }
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

   /* Get the width of the string. */
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

#ifndef USE_PANGO
unsigned XlfdLen(const char *src) {
   if(src) {
      return strlen(src) + 1;
   } else {
      return 2;
   }
}
#endif

#ifndef USE_PANGO
void XlfdCat(char *dest, const char *src) {
   if(src) {
      strcat(dest, "-");
      strcat(dest, src);
   } else {
      strcat(dest, "-*");
   }
}
#endif

/** Set the font to use for a component. */
void SetFont(FontType type, FontAttributes attrs)
{
   /* We attempt to build a font based on the provided attributes.
    * Since older versions of JWM relied on an explicit font description,
    * we fall back to that (if it exists).
    */
   const int have_attrs = attrs.family
      || attrs.style
      || attrs.variant
      || attrs.weight
      || attrs.stretch
      || attrs.size;

#ifdef USE_PANGO

   if(fonts[type]) {
      pango_font_description_free(fonts[type]);
   }
   if(have_attrs) {
      PangoFontDescription *desc = pango_font_description_new();
      fonts[type] = desc;

      if(attrs.family) {
         pango_font_description_set_family(desc, attrs.family);
      }

      if(attrs.style) {
         const int style = FindValue(STYLE_MAP, STYLE_MAP_COUNT, attrs.style);
         if(style >= 0) {
            pango_font_description_set_style(desc, style);
         } else {
            Warning(_("invalid font style: %s"), attrs.style);
         }
      }

      if(attrs.variant) {
         const int variant = FindValue(VARIANT_MAP, VARIANT_MAP_COUNT,
            attrs.variant);
         if(variant >= 0) {
            pango_font_description_set_variant(desc, variant);
         } else {
            Warning(_("invalid font variant: %s"), attrs.variant);
         }
      }

      if(attrs.weight) {
         const int weight = FindValue(WEIGHT_MAP, WEIGHT_MAP_COUNT,
            attrs.weight);
         if(weight >= 0) {
            pango_font_description_set_weight(desc, weight);
         } else {
            Warning(_("invalid font weight: %s"), attrs.weight);
         }
      }

      if(attrs.stretch) {
         const int stretch = FindValue(STRETCH_MAP,
            STRETCH_MAP_COUNT, attrs.stretch);
         if(stretch >= 0) {
            pango_font_description_set_stretch(desc, stretch);
         } else {
            Warning(_("invalid font stretch: %s"), attrs.stretch);
         }
      }

      if(attrs.size) {
         const int size = atoi(attrs.size);
         if(size >= 0) {
            pango_font_description_set_size(desc, size * PANGO_SCALE);
         } else {
            Warning(_("invalid font size: %s"), attrs.size);
         }
      }

   } else if(attrs.name) {
      fonts[type] = pango_font_description_from_string(attrs.name);
   } else {
      fonts[type] = pango_font_description_new();
   }

#else

   /* Build XLFD string using attributes if provided,
    * otherwise use the font name, and if that doesn't
    * exist, use the default font.
    */
   if(fontNames[type]) {
      Release(fontNames[type]);
   }

   if(have_attrs) {
      const unsigned len = 1
          + 1                       /* foundry */
          + XlfdLen(attrs.family)   /* family */
          + XlfdLen(attrs.weight)   /* weight */
          + XlfdLen(attrs.style)    /* style */
          + XlfdLen(attrs.stretch)  /* stretch */
          + XlfdLen(attrs.variant)  /* add_style */
          + 2                       /* pixel_size */
          + XlfdLen(attrs.size)     /* point_size */
          + 2                       /* res_x */
          + 2                       /* res_y */
          + 2                       /* spacing */
          + 2                       /* average_width */
          + 2                       /* registry */
          + 2                       /* encoding */
      ;
      fontNames[type] = Allocate(len + 1);
      strcpy(fontNames[type], "-*");
      XlfdCat(fontNames[type], attrs.family);
      XlfdCat(fontNames[type], attrs.weight);
      XlfdCat(fontNames[type], attrs.style);
      XlfdCat(fontNames[type], attrs.stretch);
      XlfdCat(fontNames[type], attrs.variant);
      XlfdCat(fontNames[type], attrs.size);
      XlfdCat(fontNames[type], NULL);
      strcat(fontNames[type], "-*-*-*-*-*-*");
      printf("[%s]\n", fontNames[type]);
   } else if(attrs.name) {
      fontNames[type] = CopyString(attrs.name);
      printf("name [%s]\n", fontNames[type]);
   } else {
      fontNames[type] = NULL;
   }

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
