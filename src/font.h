/**
 * @file font.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the font functions.
 *
 */

#ifndef FONT_H
#define FONT_H

#include "color.h"

/** Enumeration of different components that use fonts. */
typedef enum {

   FONT_BORDER,
   FONT_MENU,
   FONT_TASK,
   FONT_POPUP,
   FONT_CLOCK,
   FONT_TRAY,
   FONT_TRAYBUTTON,
   FONT_PAGER,

   FONT_COUNT

} FontType;

void InitializeFonts();
void StartupFonts();
void ShutdownFonts();
void DestroyFonts();

/** Set the font to use for a component.
 * @param type The font component.
 * @param value The font to use.
 */
void SetFont(FontType type, const char *value);

/** Render a string.
 * @param d The drawable on which to render the string.
 * @param font The font to use.
 * @param color The text color to use.
 * @param x The x-coordinate at which to render.
 * @param y The y-coordinate at which to render.
 * @param width The maximum width allowed.
 * @param region A clip region (may be None).
 * @param str The string to render.
 */
void RenderString(Drawable d, FontType font, ColorType color,
   int x, int y, int width, Region region, const char *str);

/** Get the width of a string.
 * @param type The font used to determine the width.
 * @param str The string whose width to get.
 * @return The width of the string in pixels.
 */
int GetStringWidth(FontType type, const char *str);

/** Get the height of a string.
 * @param type The font used to determine the height.
 * @return The height in pixels.
 */
int GetStringHeight(FontType type);

#endif /* FONT_H */

