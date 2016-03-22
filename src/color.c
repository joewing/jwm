/**
 * @file color.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions to handle loading colors.
 *
 */

#include "jwm.h"
#include "main.h"
#include "color.h"
#include "error.h"
#include "misc.h"
#include "settings.h"

/** Mapping between color types and default values. */
typedef struct {
   ColorType type;
   unsigned int value;
} DefaultColorNode;

unsigned long colors[COLOR_COUNT];
static unsigned long rgbColors[COLOR_COUNT];

/* Map a linear 8-bit RGB space to pixel values. */
static unsigned long *rgbToPixel;

/* Map 8-bit pixel values to a 24-bit linear RGB space. */
static unsigned long *pixelToRgb;

/* Maximum number of colors to allocate for icons. */
static const unsigned MAX_COLORS = 64;

#ifdef USE_XFT
static XftColor *xftColors[COLOR_COUNT] = { NULL };
#endif

static const DefaultColorNode DEFAULT_COLORS[] = {

   { COLOR_TITLE_FG,                0xFFFFFF    },
   { COLOR_TITLE_ACTIVE_FG,         0xFFFFFF    },

   { COLOR_TITLE_BG1,               0x555555    },
   { COLOR_TITLE_BG2,               0x555555    },
   { COLOR_TITLE_ACTIVE_BG1,        0x0077CC    },
   { COLOR_TITLE_ACTIVE_BG2,        0x0077CC    },

   { COLOR_TRAY_FG,                 0x000000    },
   { COLOR_TRAY_BG1,                0x999999    },
   { COLOR_TRAY_BG2,                0x999999    },
   { COLOR_TRAY_ACTIVE_FG,          0xFFFFFF    },
   { COLOR_TRAY_ACTIVE_BG1,         0x555555    },
   { COLOR_TRAY_ACTIVE_BG2,         0x555555    },

   { COLOR_PAGER_BG,                0x999999    },
   { COLOR_PAGER_FG,                0x333333    },
   { COLOR_PAGER_ACTIVE_BG,         0x004488    },
   { COLOR_PAGER_ACTIVE_FG,         0x0077CC    },
   { COLOR_PAGER_OUTLINE,           0x000000    },
   { COLOR_PAGER_TEXT,              0x000000    },

   { COLOR_MENU_BG,                 0x999999    },
   { COLOR_MENU_FG,                 0x000000    },
   { COLOR_MENU_ACTIVE_BG1,         0x0077CC    },
   { COLOR_MENU_ACTIVE_BG2,         0x0077CC    },
   { COLOR_MENU_ACTIVE_FG,          0x000000    },

   { COLOR_POPUP_BG,                0x999999    },
   { COLOR_POPUP_FG,                0x000000    },
   { COLOR_POPUP_OUTLINE,           0x000000    }

};
static const unsigned DEFAULT_COUNT = ARRAY_LENGTH(DEFAULT_COLORS);

static struct {
   ColorType src;
   ColorType dest;
} INHERITED_COLORS[] = {
   { COLOR_TRAY_FG,     COLOR_CLOCK_FG    },
   { COLOR_TRAY_BG1,    COLOR_CLOCK_BG1   },
   { COLOR_TRAY_BG2,    COLOR_CLOCK_BG2   }
};
static const unsigned INHERITED_COUNT = ARRAY_LENGTH(INHERITED_COLORS);

static struct {
   ColorType base;
   ColorType up;
   ColorType down;
} DERIVED_COLORS[] = {
   { COLOR_TITLE_BG1,        COLOR_TITLE_UP,        COLOR_TITLE_DOWN        },
   { COLOR_TITLE_ACTIVE_BG1, COLOR_TITLE_ACTIVE_UP, COLOR_TITLE_ACTIVE_DOWN },
   { COLOR_TRAY_BG1,         COLOR_TRAY_UP,         COLOR_TRAY_DOWN         },
   { COLOR_TRAY_ACTIVE_BG1,  COLOR_TRAY_ACTIVE_UP,  COLOR_TRAY_ACTIVE_DOWN  },
   { COLOR_MENU_BG,          COLOR_MENU_UP,         COLOR_MENU_DOWN         },
   { COLOR_MENU_ACTIVE_BG1,  COLOR_MENU_ACTIVE_UP,  COLOR_MENU_ACTIVE_DOWN  }
};
static const unsigned DERIVED_COUNT = ARRAY_LENGTH(DERIVED_COLORS);

static char **names = NULL;

static unsigned redShift;
static unsigned greenShift;
static unsigned blueShift;
static unsigned redBits;
static unsigned greenBits;
static unsigned blueBits;

static unsigned ComputeShift(unsigned long maskIn, unsigned *shiftOut);
static unsigned long GetRGBFromXColor(const XColor *c);

static void GetDirectPixel(XColor *c);
static void GetMappedPixel(XColor *c);
static void AllocateColor(ColorType type, XColor *c);

static void SetDefaultColor(ColorType type); 

static unsigned long ReadHex(const char *hex);
char ParseColorToRGB(const char *value, XColor *c);

static void InitializeNames(void);

static XColor GetXColorFromRGB(unsigned long rgb);
static void LightenColor(ColorType oldColor, ColorType newColor);
static void DarkenColor(ColorType oldColor, ColorType newColor);

/** Startup color support. */
void StartupColors(void)
{
   unsigned int x;
   XColor c;

   /* Determine how to convert between RGB triples and pixels. */
   switch(rootVisual->class) {
   case DirectColor:
   case TrueColor:
      redBits = ComputeShift(rootVisual->red_mask, &redShift);
      greenBits = ComputeShift(rootVisual->green_mask, &greenShift);
      blueBits = ComputeShift(rootVisual->blue_mask, &blueShift);
      rgbToPixel = NULL;
      break;
   default:
      /* Restrict icons to 64 colors (RGB 2, 2, 2). */
      redBits = ComputeShift(0x30, &redShift);
      greenBits = ComputeShift(0x0C, &greenShift);
      blueBits = ComputeShift(0x03, &blueShift);
      rgbToPixel = Allocate(sizeof(unsigned long) * MAX_COLORS);
      memset(rgbToPixel, 0xFF, sizeof(unsigned long) * MAX_COLORS);
      pixelToRgb = Allocate(sizeof(unsigned long) * MAX_COLORS);
      break;
   }

   /* Get color information used for JWM stuff. */
   for(x = 0; x < COLOR_COUNT; x++) {
      if(names && names[x]) {
         if(ParseColorToRGB(names[x], &c)) {
            AllocateColor(x, &c);
         } else {
            SetDefaultColor(x);
         }
      } else {
         SetDefaultColor(x);
      }
   }

   /* Derive colors. */
   for(x = 0; x < DERIVED_COUNT; x++) {
      if(!names || !names[DERIVED_COLORS[x].up]) {
         LightenColor(DERIVED_COLORS[x].base, DERIVED_COLORS[x].up);
      }
      if(!names || !names[DERIVED_COLORS[x].down]) {
         DarkenColor(DERIVED_COLORS[x].base, DERIVED_COLORS[x].down);
      }
   }

   /* Inherit colors. */
   for(x = 0; x < INHERITED_COUNT; x++) {
      const ColorType dest = INHERITED_COLORS[x].dest;
      if(!names || !names[dest]) {
         const ColorType src = INHERITED_COLORS[x].src;
         colors[dest] = colors[src];
         rgbColors[dest] = rgbColors[src];
      }
   }

   DestroyColors();
}

/** Shutdown color support. */
void ShutdownColors(void)
{
   unsigned x;

#ifdef USE_XFT
   for(x = 0; x < COLOR_COUNT; x++) {
      if(xftColors[x]) {
         JXftColorFree(display, rootVisual, rootColormap, xftColors[x]);
         Release(xftColors[x]);
         xftColors[x] = NULL;
      }
   }
#endif

   if(rgbToPixel) {
      for(x = 0; x < MAX_COLORS; x++) {
         if(rgbToPixel[x] != ULONG_MAX) {
            JXFreeColors(display, rootColormap, &rgbToPixel[x], 1, 0);
         }
      }
      JXFreeColors(display, rootColormap, colors, COLOR_COUNT, 0);
      Release(rgbToPixel);
      Release(pixelToRgb);
      rgbToPixel = NULL;
   }

}

/** Release color data. */
void DestroyColors(void)
{
   if(names) {
      unsigned int x;
      for(x = 0; x < COLOR_COUNT; x++) {
         if(names[x]) {
            Release(names[x]);
         }
      }
      Release(names);
      names = NULL;
   }
}

/** Compute the shift and bits for a linear RGB colormap. */
unsigned ComputeShift(unsigned long maskIn, unsigned *shiftOut)
{
   unsigned bits = 0;
   unsigned shift = 0;
   while(maskIn && (maskIn & 1) == 0) {
      shift += 1;
      maskIn >>= 1;
   }
   *shiftOut = shift;
   while(maskIn) {
      bits += 1;
      maskIn >>= 1;
   }
   return bits;
}

/** Get an RGB value from an XColor. */
unsigned long GetRGBFromXColor(const XColor *c)
{
   unsigned long red, green, blue;
   red   = (c->red   >> 8) << 16;
   green = (c->green >> 8) << 8;
   blue  = (c->blue  >> 8) << 0;
   return red | green | blue;
}

/** Set the color to use for a component. */
void SetColor(ColorType c, const char *value)
{
   if(JUNLIKELY(!value)) {
      Warning("empty color tag");
      return;
   }

   InitializeNames();

   if(names[c]) {
      Release(names[c]);
   }

   names[c] = CopyString(value);
}

/** Parse a color without lookup. */
char ParseColorToRGB(const char *value, XColor *c)
{
   if(JUNLIKELY(!value)) {
      return 0;
   }

   if(value[0] == '#' && strlen(value) == 7) {
      const unsigned long rgb = ReadHex(value + 1);
      c->red = ((rgb >> 16) & 0xFF) * 257;
      c->green = ((rgb >> 8) & 0xFF) * 257;
      c->blue = (rgb & 0xFF) * 257;
   } else {
      if(!JXParseColor(display, rootColormap, value, c)) {
         Warning("bad color: \"%s\"", value);
         return 0;
      }
   }

   return 1;
}

/** Parse a color for a component. */
char ParseColor(const char *value, XColor *c)
{
   if(JLIKELY(ParseColorToRGB(value, c))) {
      GetColor(c);
      return 1;
   } else {
      return 0;
   }
}

/** Set the specified color to its default. */
void SetDefaultColor(ColorType type)
{
   XColor c;
   unsigned int x;

   for(x = 0; x < DEFAULT_COUNT; x++) {
      if(DEFAULT_COLORS[x].type == type) {
         const unsigned int rgb = DEFAULT_COLORS[x].value;
         c.red = ((rgb >> 16) & 0xFF) * 257;
         c.green = ((rgb >> 8) & 0xFF) * 257;
         c.blue = (rgb & 0xFF) * 257;
         AllocateColor(type, &c);
         return;
      }
   }
}

/** Initialize color names to NULL. */
void InitializeNames(void)
{
   if(names == NULL) {
      unsigned int x;
      names = Allocate(sizeof(char*) * COLOR_COUNT);
      for(x = 0; x < COLOR_COUNT; x++) {
         names[x] = NULL;
      }
   }
}

/** Convert a hex value to an unsigned long. */
unsigned long ReadHex(const char *hex)
{
   unsigned long value = 0;
   unsigned int x;

   for(x = 0; hex[x]; x++) {
      value *= 16;
      if(hex[x] >= '0' && hex[x] <= '9') {
         value += hex[x] - '0';
      } else if(hex[x] >= 'A' && hex[x] <= 'F') {
         value += hex[x] - 'A' + 10;
      } else if(hex[x] >= 'a' && hex[x] <= 'f') {
         value += hex[x] - 'a' + 10;
      }
   }

   return value;
}

/** Compute the pixel value from RGB components. */
void GetDirectPixel(XColor *c)
{
   unsigned long red   = c->red;
   unsigned long green = c->green;
   unsigned long blue  = c->blue;

   red   = (red   >> (16 - redBits  )) << redShift;
   green = (green >> (16 - greenBits)) << greenShift;
   blue  = (blue  >> (16 - blueBits )) << blueShift;

   c->pixel = red | green | blue;
}

/** Compute the pixel value from RGB components. */
void GetMappedPixel(XColor *c)
{
   unsigned long index;
   GetDirectPixel(c);
   index = c->pixel;
   if(rgbToPixel[index] == ULONG_MAX) {
      c->red &= 0xC000;
      c->green &= 0xC000;
      c->blue &= 0xC000;
      c->red |= c->red >> 8;
      c->red |= c->red >> 4;
      c->red |= c->red >> 2;
      c->green |= c->green >> 8;
      c->green |= c->green >> 4;
      c->green |= c->green >> 2;
      c->blue |= c->blue >> 8;
      c->blue |= c->blue >> 4;
      c->blue |= c->blue >> 2;
      JXAllocColor(display, rootColormap, c);
      rgbToPixel[index] = c->pixel;
      pixelToRgb[c->pixel & (MAX_COLORS - 1)] = index;
   } else {
      c->pixel = rgbToPixel[index];
   }
}

/** Allocate a pixel from RGB components. */
void AllocateColor(ColorType type, XColor *c)
{
   switch(rootVisual->class) {
   case DirectColor:
   case TrueColor:
      GetDirectPixel(c);
      break;
   default:
      JXAllocColor(display, rootColormap, c);
      break;
   }
   colors[type] = c->pixel;
   rgbColors[type] = GetRGBFromXColor(c);
}

/** Compute the pixel value from RGB components. */
void GetColor(XColor *c)
{
   switch(rootVisual->class) {
   case DirectColor:
   case TrueColor:
      GetDirectPixel(c);
      return;
   default:
      GetMappedPixel(c);
      return;
   }
}

/** Get the RGB components from a pixel value. */
void GetColorFromPixel(XColor *c)
{
   unsigned long pixel = c->pixel;
   switch(rootVisual->class) {
   case DirectColor:
   case TrueColor:
      /* Nothing to do. */
      break;
   default:
      /* Convert from a pixel value to a linear RGB space. */
      pixel = pixelToRgb[pixel & (MAX_COLORS - 1)];
      break;
   }

   /* Extract the RGB components from the linear RGB pixel value. */
   c->red   = (pixel >> redShift)   & ((1 << redBits)   - 1);
   c->green = (pixel >> greenShift) & ((1 << greenBits) - 1);
   c->blue  = (pixel >> blueShift)  & ((1 << blueBits)  - 1);

   /* Expand to 16 bits. */
   c->red   <<= 16 - redBits;
   c->green <<= 16 - greenBits;
   c->blue  <<= 16 - blueBits;
}

/** Get an XFT color for the specified component. */
#ifdef USE_XFT
XftColor *GetXftColor(ColorType type)
{

   if(!xftColors[type]) {
      XRenderColor rcolor;
      const unsigned long rgb = rgbColors[type];
      xftColors[type] = Allocate(sizeof(XftColor));
      rcolor.alpha = 65535;
      rcolor.red = ((rgb >> 16) & 0xFF) * 257;
      rcolor.green = ((rgb >> 8) & 0xFF) * 257;
      rcolor.blue = (rgb & 0xFF) * 257;
      JXftColorAllocValue(display, rootVisual, rootColormap, &rcolor,
                          xftColors[type]);
   }

   return xftColors[type];

}
#endif

/** Convert an RGB value to an XColor. */
XColor GetXColorFromRGB(unsigned long rgb)
{
   XColor ret = { 0 };

   ret.flags = DoRed | DoGreen | DoBlue;
   ret.red = (unsigned short)(((rgb >> 16) & 0xFF) * 257);
   ret.green = (unsigned short)(((rgb >> 8) & 0xFF) * 257);
   ret.blue = (unsigned short)((rgb & 0xFF) * 257);

   return ret;
}

/** Compute a color lighter than the input. */
void LightenColor(ColorType oldColor, ColorType newColor)
{

   XColor temp;
   int red, green, blue;

   temp = GetXColorFromRGB(rgbColors[oldColor]);

   /* Convert to 0.0 to 1.0 in fixed point with 8 bits for the fraction. */
   red   = temp.red   >> 8;
   green = temp.green >> 8;
   blue  = temp.blue  >> 8;

   /* Multiply by 1.45 which is 371. */
   red   = (red   * 371) >> 8;
   green = (green * 371) >> 8;
   blue  = (blue  * 371) >> 8;

   /* Convert back to 0-65535. */
   red   |= red << 8;
   green |= green << 8;
   blue  |= blue << 8;

   /* Cap at 65535. */
   red   = Min(65535, red);
   green = Min(65535, green);
   blue  = Min(65535, blue);

   temp.red = red;
   temp.green = green;
   temp.blue = blue;

   AllocateColor(newColor, &temp);
}

/** Compute a color darker than the input. */
void DarkenColor(ColorType oldColor, ColorType newColor)
{

   XColor temp;
   int red, green, blue;

   temp = GetXColorFromRGB(rgbColors[oldColor]);

   /* Convert to 0.0 to 1.0 in fixed point with 8 bits for the fraction. */
   red   = temp.red   >> 8;
   green = temp.green >> 8;
   blue  = temp.blue  >> 8;

   /* Multiply by 0.55 which is 141. */
   red   = (red   * 141) >> 8;
   green = (green * 141) >> 8;
   blue  = (blue  * 141) >> 8;

   /* Convert back to 0-65535. */
   red   |= red << 8;
   green |= green << 8;
   blue  |= blue << 8;

   temp.red = red;
   temp.green = green;
   temp.blue = blue;

   AllocateColor(newColor, &temp);
}
