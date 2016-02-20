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
#define COLOR_TRAY_FG               6
#define COLOR_TRAY_BG1              7
#define COLOR_TRAY_BG2              8
#define COLOR_TRAY_ACTIVE_FG        9
#define COLOR_TRAY_ACTIVE_BG1       10
#define COLOR_TRAY_ACTIVE_BG2       11
#define COLOR_PAGER_BG              12
#define COLOR_PAGER_FG              13
#define COLOR_PAGER_ACTIVE_BG       14
#define COLOR_PAGER_ACTIVE_FG       15
#define COLOR_PAGER_OUTLINE         16
#define COLOR_PAGER_TEXT            17
#define COLOR_MENU_BG               18
#define COLOR_MENU_FG               19
#define COLOR_MENU_UP               20
#define COLOR_MENU_DOWN             21
#define COLOR_MENU_ACTIVE_BG1       22
#define COLOR_MENU_ACTIVE_BG2       23
#define COLOR_MENU_ACTIVE_FG        24
#define COLOR_MENU_ACTIVE_UP        25
#define COLOR_MENU_ACTIVE_DOWN      26
#define COLOR_POPUP_BG              27
#define COLOR_POPUP_FG              28
#define COLOR_POPUP_OUTLINE         29
#define COLOR_TITLE_UP              30
#define COLOR_TITLE_DOWN            31
#define COLOR_TITLE_ACTIVE_UP       32
#define COLOR_TITLE_ACTIVE_DOWN     33
#define COLOR_TRAY_UP               34
#define COLOR_TRAY_DOWN             35
#define COLOR_TRAY_ACTIVE_UP        36
#define COLOR_TRAY_ACTIVE_DOWN      37
#define COLOR_CLOCK_FG              38
#define COLOR_CLOCK_BG1             39
#define COLOR_CLOCK_BG2             40
#define COLOR_COUNT                 41

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
 */
void GetColor(XColor *c);

/** Get the RGB components from a color pixel.
 * This does the reverse of GetColor.
 * @param c The structure containing the rgb values and pixel value.
 */
void GetColorFromPixel(XColor *c);

#ifdef USE_XFT
/** Get an XFT color.
 * @param type The color whose XFT color to get.
 * @return The XFT color.
 */
XftColor *GetXftColor(ColorType type);
#endif

#endif /* COLOR_H */
