/**
 * @file color.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the color functions.
 *
 */

#ifndef COLOR_H
#define COLOR_H

/** Enumeration of colors used for various JWM components. */
typedef enum {

   COLOR_TITLE_FG,
   COLOR_TITLE_ACTIVE_FG,

   COLOR_TITLE_BG1,
   COLOR_TITLE_BG2,
   COLOR_TITLE_ACTIVE_BG1,
   COLOR_TITLE_ACTIVE_BG2,

   COLOR_BORDER_LINE,
   COLOR_BORDER_ACTIVE_LINE,

   COLOR_TRAY_BG,
   COLOR_TRAY_FG,

   COLOR_TASK_FG,
   COLOR_TASK_BG1,
   COLOR_TASK_BG2,
   COLOR_TASK_ACTIVE_FG,
   COLOR_TASK_ACTIVE_BG1,
   COLOR_TASK_ACTIVE_BG2,

   COLOR_PAGER_BG,
   COLOR_PAGER_FG,
   COLOR_PAGER_ACTIVE_BG,
   COLOR_PAGER_ACTIVE_FG,
   COLOR_PAGER_OUTLINE,
   COLOR_PAGER_TEXT,

   COLOR_MENU_BG,
   COLOR_MENU_FG,
   COLOR_MENU_ACTIVE_BG1,
   COLOR_MENU_ACTIVE_BG2,
   COLOR_MENU_ACTIVE_FG,
   COLOR_MENU_ACTIVE_OL,

   /* Colors below this point are calculated from the above values. */

   COLOR_TRAY_UP,
   COLOR_TRAY_DOWN,

   COLOR_TASK_UP,
   COLOR_TASK_DOWN,
   COLOR_TASK_ACTIVE_UP,
   COLOR_TASK_ACTIVE_DOWN,

   COLOR_MENU_UP,
   COLOR_MENU_DOWN,
   COLOR_MENU_ACTIVE_UP,
   COLOR_MENU_ACTIVE_DOWN,

   COLOR_POPUP_BG,
   COLOR_POPUP_FG,
   COLOR_POPUP_OUTLINE,

   COLOR_TRAYBUTTON_BG,
   COLOR_TRAYBUTTON_FG,

   COLOR_CLOCK_BG,
   COLOR_CLOCK_FG,

   COLOR_COUNT

} ColorType;

extern unsigned long colors[COLOR_COUNT];

/*@{*/
void InitializeColors();
void StartupColors();
void ShutdownColors();
void DestroyColors();
/*@}*/

/** Set the color to use for a component.
 * @param c The component whose color to set.
 * @param value The color to use.
 */
void SetColor(ColorType c, const char *value);

/** Parse a color.
 * @param value The color name or hex value.
 * @param color The color return value (with pixel and components filled).
 * @return 1 on success, 0 on failure.
 */
int ParseColor(const char *value, XColor *color);

/** Get the color pixel from red, green, and blue values.
 * @param c The structure containing the rgb values and the pixel value.
 */
void GetColor(XColor *c);

/** Get the RGB components from a color pixel.
 * This does the reverse of GetColor.
 * @param c The structure containing the rgb values and pixel value.
 */
void GetColorFromPixel(XColor *c);

/** Get an RGB pixel value from RGB components.
 * This is used when loading images from external sources. When doing
 * this we need to know the color components even if we are using a
 * color map so we just pretend to have a linear RGB colormap.
 * This prevents calls to XQueryColor.
 * @param c The structure containing the rgb values and pixel value.
 */
void GetColorIndex(XColor *c);

/** Extract the RGB components from a RGB linear pixel value.
 * This does the reverse of GetColorIndex.
 * @param c The structure containing the rgb values and pixel value.
 */
void GetColorFromIndex(XColor *c);

#ifdef USE_XFT
/** Get an XFT color.
 * @param type The color whose XFT color to get.
 * @return The XFT color.
 */
XftColor *GetXftColor(ColorType type);
#endif

#endif /* COLOR_H */

