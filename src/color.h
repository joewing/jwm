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
typedef unsigned char ColorType;
#define COLOR_TITLE_FG              0
#define COLOR_TITLE_ACTIVE_FG       1
#define COLOR_TITLE_BG1             2
#define COLOR_TITLE_BG2             3
#define COLOR_TITLE_ACTIVE_BG1      4
#define COLOR_TITLE_ACTIVE_BG2      5
#define COLOR_BORDER_LINE           6
#define COLOR_BORDER_ACTIVE_LINE    7
#define COLOR_TRAY_FG               8
#define COLOR_TRAY_BG1              9
#define COLOR_TRAY_BG2              10
#define COLOR_TRAY_ACTIVE_FG        11
#define COLOR_TRAY_ACTIVE_BG1       12
#define COLOR_TRAY_ACTIVE_BG2       13
#define COLOR_PAGER_BG              14
#define COLOR_PAGER_FG              15
#define COLOR_PAGER_ACTIVE_BG       16
#define COLOR_PAGER_ACTIVE_FG       17
#define COLOR_PAGER_OUTLINE         18
#define COLOR_PAGER_TEXT            19
#define COLOR_MENU_BG               20
#define COLOR_MENU_FG               21
#define COLOR_MENU_UP               22
#define COLOR_MENU_DOWN             23
#define COLOR_MENU_ACTIVE_BG1       24
#define COLOR_MENU_ACTIVE_BG2       25
#define COLOR_MENU_ACTIVE_FG        26
#define COLOR_MENU_ACTIVE_UP        27
#define COLOR_MENU_ACTIVE_DOWN      28
#define COLOR_POPUP_BG              29
#define COLOR_POPUP_FG              30
#define COLOR_TITLE_UP              31
#define COLOR_TITLE_DOWN            32
#define COLOR_TITLE_ACTIVE_UP       33
#define COLOR_TITLE_ACTIVE_DOWN     34
#define COLOR_TRAY_UP               35
#define COLOR_TRAY_DOWN             36
#define COLOR_TRAY_ACTIVE_UP        37
#define COLOR_TRAY_ACTIVE_DOWN      38
#define COLOR_COUNT                 39

extern unsigned long colors[COLOR_COUNT];

/*@{*/
#define InitializeColors() (void)(0)
void StartupColors(void);
void ShutdownColors(void);
void DestroyColors(void);
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
char ParseColor(const char *value, XColor *color);

/** Get the color pixel from red, green, and blue values.
 * @param c The structure containing the rgb values and the pixel value.
 * @param alloc Set to allocate the color if necessary.
 */
void GetColor(XColor *c, char alloc);

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
