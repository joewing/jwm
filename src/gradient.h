/**
 * @file gradient.h
 * @author Joe Wingbermuehle
 * @date 2006
 *
 * @brief Gradient fill functions.
 *
 */

#ifndef GRADIENT_H
#define GRADIENT_H

/** Draw a horizontal gradient.
 * Note that no action is taken if fromColor == toColor.
 * @param d The drawable on which to draw the gradient.
 * @param g The graphics context to use.
 * @param fromColor The starting color pixel value.
 * @param toColor The ending color pixel value.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @param width The width of the area to fill.
 * @param height The height of the area to fill.
 */
void DrawHorizontalGradient(Drawable d, GC g,
   long fromColor, long toColor,
   int x, int y, unsigned int width, unsigned int height);

#endif /* GRADIENT_H */

