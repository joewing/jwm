/**
 * @file color.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions to handle loading colors.
 *
 */

#ifndef COLOR_H
#define COLOR_H

/** Enumeration of colors used for various JWM components.
 * For easier parsing, tray components must all have colors ordered the
 * same way as COLOR_TRAY_*.
 */
typedef unsigned char ColorType;
#define COLOR_TITLE_FG                 0
#define COLOR_TITLE_ACTIVE_FG          1
#define COLOR_TITLE_BG1                2
#define COLOR_TITLE_BG2                3
#define COLOR_TITLE_ACTIVE_BG1         4
#define COLOR_TITLE_ACTIVE_BG2         5
#define COLOR_TRAY_FG                  6
#define COLOR_TRAY_BG1                 7
#define COLOR_TRAY_BG2                 8
#define COLOR_TRAY_ACTIVE_FG           9
#define COLOR_TRAY_ACTIVE_BG1          10
#define COLOR_TRAY_ACTIVE_BG2          11
#define COLOR_TRAY_UP                  12
#define COLOR_TRAY_DOWN                13
#define COLOR_TRAY_ACTIVE_UP           14
#define COLOR_TRAY_ACTIVE_DOWN         15
#define COLOR_TASKLIST_FG              16
#define COLOR_TASKLIST_BG1             17
#define COLOR_TASKLIST_BG2             18
#define COLOR_TASKLIST_ACTIVE_FG       19
#define COLOR_TASKLIST_ACTIVE_BG1      20
#define COLOR_TASKLIST_ACTIVE_BG2      21
#define COLOR_TASKLIST_UP              22
#define COLOR_TASKLIST_DOWN            23
#define COLOR_TASKLIST_ACTIVE_UP       24
#define COLOR_TASKLIST_ACTIVE_DOWN     25
#define COLOR_TRAYBUTTON_FG            26
#define COLOR_TRAYBUTTON_BG1           27
#define COLOR_TRAYBUTTON_BG2           28
#define COLOR_TRAYBUTTON_ACTIVE_FG     29
#define COLOR_TRAYBUTTON_ACTIVE_BG1    30
#define COLOR_TRAYBUTTON_ACTIVE_BG2    31
#define COLOR_TRAYBUTTON_UP            32
#define COLOR_TRAYBUTTON_DOWN          33
#define COLOR_TRAYBUTTON_ACTIVE_UP     34
#define COLOR_TRAYBUTTON_ACTIVE_DOWN   35
#define COLOR_PAGER_BG                 36
#define COLOR_PAGER_FG                 37
#define COLOR_PAGER_ACTIVE_BG          38
#define COLOR_PAGER_ACTIVE_FG          39
#define COLOR_PAGER_OUTLINE            40
#define COLOR_PAGER_TEXT               41
#define COLOR_MENU_BG                  42
#define COLOR_MENU_FG                  43
#define COLOR_MENU_UP                  44
#define COLOR_MENU_DOWN                45
#define COLOR_MENU_ACTIVE_BG1          46
#define COLOR_MENU_ACTIVE_BG2          47
#define COLOR_MENU_ACTIVE_FG           48
#define COLOR_MENU_ACTIVE_UP           49
#define COLOR_MENU_ACTIVE_DOWN         50
#define COLOR_POPUP_BG                 51
#define COLOR_POPUP_FG                 52
#define COLOR_POPUP_OUTLINE            53
#define COLOR_TITLE_UP                 54
#define COLOR_TITLE_DOWN               55
#define COLOR_TITLE_ACTIVE_UP          56
#define COLOR_TITLE_ACTIVE_DOWN        57
#define COLOR_CLOCK_FG                 58
#define COLOR_CLOCK_BG1                59
#define COLOR_CLOCK_BG2                60
#define COLOR_COUNT                    61

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
 * @param c The color return value (with pixel and components filled).
 * @return 1 on success, 0 on failure.
 */
char ParseColor(const char *value, XColor *c);

/** Get the color pixel from red, green, and blue values.
 * @param c The structure containing the rgb values and the pixel value.
 */
void GetColor(XColor *c);

#ifdef USE_XFT
/** Get an XFT color.
 * @param type The color whose XFT color to get.
 * @return The XFT color.
 */
XftColor *GetXftColor(ColorType type);
#endif

#endif /* COLOR_H */
