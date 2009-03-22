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

typedef struct {
   ColorType type;
   const char *value;
} DefaultColorNode;

static const float COLOR_DELTA = 0.45;

unsigned long colors[COLOR_COUNT];
static unsigned long rgbColors[COLOR_COUNT];

/* Map a linear 8-bit RGB space to pixel values. */
static unsigned long *map;

/* Map 8-bit pixel values to a 24-bit linear RGB space. */
static unsigned long *rmap;

#ifdef USE_XFT
static XftColor *xftColors[COLOR_COUNT] = { NULL };
#endif

static DefaultColorNode DEFAULT_COLORS[] = {

   { COLOR_TITLE_FG,           "black"   },
   { COLOR_TITLE_ACTIVE_FG,    "black"   },

   { COLOR_TITLE_BG1,          "gray"    },
   { COLOR_TITLE_BG2,          "gray"    },
   { COLOR_TITLE_ACTIVE_BG1,   "red"     },
   { COLOR_TITLE_ACTIVE_BG2,   "red"     },

   { COLOR_BORDER_LINE,        "black"   },
   { COLOR_BORDER_ACTIVE_LINE, "black"   },

   { COLOR_TRAY_BG,            "gray"    },
   { COLOR_TRAY_FG,            "black"   },

   { COLOR_TASK_FG,            "black"   },
   { COLOR_TASK_BG1,           "gray"    },
   { COLOR_TASK_BG2,           "gray"    },
   { COLOR_TASK_ACTIVE_FG,     "white"   },
   { COLOR_TASK_ACTIVE_BG1,    "red"     },
   { COLOR_TASK_ACTIVE_BG2,    "red"     },

   { COLOR_PAGER_BG,           "black"   },
   { COLOR_PAGER_FG,           "gray"    },
   { COLOR_PAGER_ACTIVE_BG,    "red"     },
   { COLOR_PAGER_ACTIVE_FG,    "red"     },
   { COLOR_PAGER_OUTLINE,      "black"   },
   { COLOR_PAGER_TEXT,         "black"   },

   { COLOR_MENU_BG,            "gray"    },
   { COLOR_MENU_FG,            "black"   },
   { COLOR_MENU_ACTIVE_BG1,    "red"     },
   { COLOR_MENU_ACTIVE_BG2,    "red"     },
   { COLOR_MENU_ACTIVE_FG,     "white"   },
   { COLOR_MENU_ACTIVE_OL,     "black"   },

   { COLOR_POPUP_BG,           "yellow"  },
   { COLOR_POPUP_FG,           "black"   },
   { COLOR_POPUP_OUTLINE,      "black"   },

   { COLOR_TRAYBUTTON_FG,      "black"   },
   { COLOR_TRAYBUTTON_BG,      "gray"    },

   { COLOR_CLOCK_FG,           "black"   },
   { COLOR_CLOCK_BG,           "gray"    },
   { COLOR_COUNT,              NULL      }

};

static char **names = NULL;

static unsigned long redShift;
static unsigned long greenShift;
static unsigned long blueShift;
static unsigned long redMask;
static unsigned long greenMask;
static unsigned long blueMask;

static void ComputeShiftMask(unsigned long maskIn,
   unsigned long *shiftOut, unsigned long *maskOut);

static void GetDirectPixel(XColor *c);
static void GetMappedPixel(XColor *c);

static void SetDefaultColor(ColorType type); 

static unsigned long ReadHex(const char *hex);

static unsigned long GetRGBFromXColor(const XColor *c);
static XColor GetXColorFromRGB(unsigned long rgb);

static int GetColorByName(const char *str, XColor *c);
static void InitializeNames();

static void LightenColor(ColorType oldColor, ColorType newColor);
static void DarkenColor(ColorType oldColor, ColorType newColor);

/** Initialize color data. */
void InitializeColors() {

}

/** Startup color support. */
void StartupColors() {

   int x;
   int red, green, blue;
   XColor c;

   /* Determine how to convert between RGB triples and pixels. */
   Assert(rootVisual);
   switch(rootVisual->class) {
   case DirectColor:
   case TrueColor:
      ComputeShiftMask(rootVisual->red_mask, &redShift, &redMask);
      ComputeShiftMask(rootVisual->green_mask, &greenShift, &greenMask);
      ComputeShiftMask(rootVisual->blue_mask, &blueShift, &blueMask);
      map = NULL;
      break;
   default:

      /* Attempt to get 256 colors, pretend it worked. */
      redMask = 0xE0;
      greenMask = 0x1C;
      blueMask = 0x03;
      ComputeShiftMask(redMask, &redShift, &redMask);
      ComputeShiftMask(greenMask, &greenShift, &greenMask);
      ComputeShiftMask(blueMask, &blueShift, &blueMask);
      map = Allocate(sizeof(unsigned long) * 256);

      /* RGB: 3, 3, 2 */
      x = 0;
      for(red = 0; red < 8; red++) {
         for(green = 0; green < 8; green++) {
            for(blue = 0; blue < 4; blue++) {
               c.red = (unsigned short)(74898 * red / 8);
               c.green = (unsigned short)(74898 * green / 8);
               c.blue = (unsigned short)(87381 * blue / 4);
               c.flags = DoRed | DoGreen | DoBlue;
               JXAllocColor(display, rootColormap, &c);
               map[x] = c.pixel;
               ++x;
            }
         }
      }

      /* Compute the reverse pixel mapping (pixel -> 24-bit RGB). */
      rmap = Allocate(sizeof(unsigned long) * 256);
      for(x = 0; x < 256; x++) {
         c.pixel = x;
         JXQueryColor(display, rootColormap, &c);
         GetDirectPixel(&c);
         rmap[x] = c.pixel;
      }

      break;
   }

   /* Inherit unset colors from the tray for tray items. */
   if(names) {

      if(!names[COLOR_TASK_BG1]) {
         names[COLOR_TASK_BG1] = CopyString(names[COLOR_TRAY_BG]);
      }
      if(!names[COLOR_TASK_BG2]) {
         names[COLOR_TASK_BG2] = CopyString(names[COLOR_TRAY_BG]);
      }

      if(!names[COLOR_TRAYBUTTON_BG]) {
         names[COLOR_TRAYBUTTON_BG] = CopyString(names[COLOR_TRAY_BG]);
      }
      if(!names[COLOR_CLOCK_BG]) {
         names[COLOR_CLOCK_BG] = CopyString(names[COLOR_TRAY_BG]);
      }
      if(!names[COLOR_TASK_FG]) {
         names[COLOR_TASK_FG] = CopyString(names[COLOR_TRAY_FG]);
      }
      if(!names[COLOR_TRAYBUTTON_FG]) {
         names[COLOR_TRAYBUTTON_FG] = CopyString(names[COLOR_TRAY_FG]);
      }
      if(!names[COLOR_CLOCK_FG]) {
         names[COLOR_CLOCK_FG] = CopyString(names[COLOR_TRAY_FG]);
      }
   }

   /* Get color information used for JWM stuff. */
   for(x = 0; x < COLOR_COUNT; x++) {
      if(names && names[x]) {
         if(ParseColor(names[x], &c)) {
            colors[x] = c.pixel;
            rgbColors[x] = GetRGBFromXColor(&c);
         } else {
            SetDefaultColor(x);
         }
      } else {
         SetDefaultColor(x);
      }
   }

   if(names) {
      for(x = 0; x < COLOR_COUNT; x++) {
         if(names[x]) {
            Release(names[x]);
         }
      }
      Release(names);
      names = NULL;
   }

   LightenColor(COLOR_TRAY_BG, COLOR_TRAY_UP);
   DarkenColor(COLOR_TRAY_BG, COLOR_TRAY_DOWN);

   LightenColor(COLOR_TASK_BG1, COLOR_TASK_UP);
   DarkenColor(COLOR_TASK_BG1, COLOR_TASK_DOWN);

   LightenColor(COLOR_TASK_ACTIVE_BG1, COLOR_TASK_ACTIVE_UP);
   DarkenColor(COLOR_TASK_ACTIVE_BG1, COLOR_TASK_ACTIVE_DOWN);

   LightenColor(COLOR_MENU_BG, COLOR_MENU_UP);
   DarkenColor(COLOR_MENU_BG, COLOR_MENU_DOWN);

   LightenColor(COLOR_MENU_ACTIVE_BG1, COLOR_MENU_ACTIVE_UP);
   DarkenColor(COLOR_MENU_ACTIVE_BG1, COLOR_MENU_ACTIVE_DOWN);

}

/** Shutdown color support. */
void ShutdownColors() {

#ifdef USE_XFT

   int x;

   for(x = 0; x < COLOR_COUNT; x++) {
      if(xftColors[x]) {
         JXftColorFree(display, rootVisual, rootColormap, xftColors[x]);
         Release(xftColors[x]);
         xftColors[x] = NULL;
      }
   }

#endif

   if(map != NULL) {
      JXFreeColors(display, rootColormap, map, 256, 0);
      Release(map);
      map = NULL;
      Release(rmap);
      rmap = NULL;
   }

}

/** Release color data. */
void DestroyColors() {

   int x;

   if(names) {
      for(x = 0; x < COLOR_COUNT; x++) {
         if(names[x]) {
            Release(names[x]);
         }
      }
      Release(names);
      names = NULL;
   }

}

/** Compute the mask for computing colors in a linear RGB colormap. */
void ComputeShiftMask(unsigned long maskIn,
   unsigned long *shiftOut, unsigned long *maskOut) {

   int shift;

   Assert(shiftOut);
   Assert(maskOut);

   /* Components are stored in 16 bits.
    * When computing pixels, we'll first shift left 16 bits
    * so to the shift will be an offset from that 32 bit entity.
    * shift = 16 - <shift-to-ones> + <shift-to-zeros>
    */

   shift = 0;
   *maskOut = maskIn;
   while(maskIn && (maskIn & (1 << 31)) == 0) {
      ++shift;
      maskIn <<= 1;
   }
   *shiftOut = shift;

}

/** Get an RGB value from an XColor. */
unsigned long GetRGBFromXColor(const XColor *c) {

   float red, green, blue;
   unsigned long rgb;

   Assert(c);

   red = (float)c->red / 65535.0;
   green = (float)c->green / 65535.0;
   blue = (float)c->blue / 65535.0;

   rgb = (unsigned long)(red * 255.0) << 16;
   rgb |= (unsigned long)(green * 255.0) << 8;
   rgb |= (unsigned long)(blue * 255.0);

   return rgb;

}

/** Convert an RGB value to an XColor. */
XColor GetXColorFromRGB(unsigned long rgb) {

   XColor ret = { 0 };

   ret.flags = DoRed | DoGreen | DoBlue;
   ret.red = (unsigned short)(((rgb >> 16) & 0xFF) * 257);
   ret.green = (unsigned short)(((rgb >> 8) & 0xFF) * 257);
   ret.blue = (unsigned short)((rgb & 0xFF) * 257);

   return ret;

}

/** Set the color to use for a component. */
void SetColor(ColorType c, const char *value) {

   if(!value) {
      Warning("empty color tag");
      return;
   }

   InitializeNames();

   if(names[c]) {
      Release(names[c]);
   }

   names[c] = CopyString(value);

}

/** Parse a color for a component. */
int ParseColor(const char *value, XColor *c) {

   unsigned long rgb;

   if(!value) {
      return 0;
   }

   if(value[0] == '#' && strlen(value) == 7) {
      rgb = ReadHex(value + 1);
      c->red = ((rgb >> 16) & 0xFF) * 257;
      c->green = ((rgb >> 8) & 0xFF) * 257;
      c->blue = (rgb & 0xFF) * 257;
      c->flags = DoRed | DoGreen | DoBlue;
      GetColor(c);
   } else {
      if(!GetColorByName(value, c)) {
         Warning("bad color: \"%s\"", value);
         return 0;
      }
   }

   return 1;

}

/** Set the specified color to its default. */
void SetDefaultColor(ColorType type) {

   XColor c;
   int x;

   for(x = 0; DEFAULT_COLORS[x].value; x++) {
      if(DEFAULT_COLORS[x].type == type) {
         ParseColor(DEFAULT_COLORS[x].value, &c);
         colors[type] = c.pixel;
         rgbColors[type] = GetRGBFromXColor(&c);
         return;
      }
   }

}

/** Initialize color names to NULL. */
void InitializeNames() {

   int x;

   if(names == NULL) {
      names = Allocate(sizeof(char*) * COLOR_COUNT);
      for(x = 0; x < COLOR_COUNT; x++) {
         names[x] = NULL;
      }
   }

}

/** Convert a hex value to an unsigned long. */
unsigned long ReadHex(const char *hex) {

   unsigned long value = 0;
   int x;

   Assert(hex);

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

/** Compute a color lighter than the input. */
void LightenColor(ColorType oldColor, ColorType newColor) {

   XColor temp;
   float red, green, blue;
   float delta = 1.0 + COLOR_DELTA;

   temp = GetXColorFromRGB(rgbColors[oldColor]);

   red = (float)temp.red / 65535.0;
   green = (float)temp.green / 65535.0;
   blue = (float)temp.blue / 65535.0;

   red = Min(delta * red, 1.0);
   green = Min(delta * green, 1.0);
   blue = Min(delta * blue, 1.0);

   temp.red = red * 65535.0;
   temp.green = green * 65535.0;
   temp.blue = blue * 65535.0;

   GetColor(&temp);
   colors[newColor] = temp.pixel;
   rgbColors[newColor] = GetRGBFromXColor(&temp);

}

/** Compute a color darker than the input. */
void DarkenColor(ColorType oldColor, ColorType newColor) {

   XColor temp;
   float red, green, blue;
   float delta = 1.0 - COLOR_DELTA;

   temp = GetXColorFromRGB(rgbColors[oldColor]);

   red = (float)temp.red / 65535.0;
   green = (float)temp.green / 65535.0;
   blue = (float)temp.blue / 65535.0;

   red = delta * red;
   green = delta * green;
   blue = delta * blue;

   temp.red = red * 65535.0;
   temp.green = green * 65535.0;
   temp.blue = blue * 65535.0;

   GetColor(&temp);
   colors[newColor] = temp.pixel;
   rgbColors[newColor] = GetRGBFromXColor(&temp);

}

/** Look up a color by name. */
int GetColorByName(const char *str, XColor *c) {

   Assert(str);
   Assert(c);

   if(!JXParseColor(display, rootColormap, str, c)) {
      return 0;
   }

   GetColor(c);

   return 1;

}

/** Compute the RGB components from an index into our RGB colormap. */
void GetColorFromIndex(XColor *c) {

   unsigned long red;
   unsigned long green;
   unsigned long blue;

   Assert(c);

   red = (c->pixel & redMask) << redShift;
   green = (c->pixel & greenMask) << greenShift;
   blue = (c->pixel & blueMask) << blueShift;

   c->red = red >> 16;
   c->green = green >> 16;
   c->blue = blue >> 16;

}

/** Compute the pixel value from RGB components. */
void GetDirectPixel(XColor *c) {

   unsigned long red;
   unsigned long green;
   unsigned long blue;

   Assert(c);

   /* Normalize. */
   red = c->red << 16;
   green = c->green << 16;
   blue = c->blue << 16;

   /* Shift to the correct offsets and mask. */
   red = (red >> redShift) & redMask;
   green = (green >> greenShift) & greenMask;
   blue = (blue >> blueShift) & blueMask;

   /* Combine. */
   c->pixel = red | green | blue;

}

/** Compute the pixel value from RGB components. */
void GetMappedPixel(XColor *c) {

   Assert(c);

   GetDirectPixel(c);
   c->pixel = map[c->pixel];

}

/** Compute the pixel value from RGB components. */
void GetColor(XColor *c) {

   Assert(c);
   Assert(rootVisual);

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
void GetColorFromPixel(XColor *c) {

   Assert(c);

	switch(rootVisual->class) {
	case DirectColor:
	case TrueColor:
		/* Nothing to do. */
		break;
	default:
   	/* Convert from a pixel value to a linear RGB space. */
   	c->pixel = rmap[c->pixel & 255];
		break;
	}

   /* Extract the RGB components from the linear RGB pixel value. */
   GetColorFromIndex(c);

}


/** Get an RGB pixel value from RGB components. */
void GetColorIndex(XColor *c) {

   Assert(c);

   GetDirectPixel(c);

}

/** Get an XFT color for the specified component. */
#ifdef USE_XFT
XftColor *GetXftColor(ColorType type) {

   unsigned long rgb;
   XRenderColor rcolor;

   if(!xftColors[type]) {
      rgb = rgbColors[type];
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

