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

/** Enumeration of Gradient type used for determine gradient function to use.
 */
typedef unsigned char GradientType;
#define GRADIENT_TITLE_ACTIVE   1
#define GRADIENT_TITLE          2
#define GRADIENT_COUNT          3

#define GRADIENT_HORIZONTAL        0
#define GRADIENT_VERTICAL          1

extern unsigned int gradients[GRADIENT_COUNT];

#define InitializeGradients() (void)(0)

/** Set the gradient type to use for a component.
 * @param c The component whose color to set.
 * @param value The gradient type to use.
 */
void SetGradient(GradientType c, const int value);

/** Draw a gradient.
 * Note that no action is taken if fromColor == toColor.
 * @param d The drawable on which to draw the gradient.
 * @param g The graphics context to use.
 * @param fromColor The starting color pixel value.
 * @param toColor The ending color pixel value.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @param width The width of the area to fill.
 * @param height The height of the area to fill.
 * @param type The gradient type
 */
void DrawGradient(Drawable d, GC g,
                            long fromColor, long toColor,
                            int x, int y,
                            unsigned int width, unsigned int height,
                            int type);


/** Draw a vertical gradient.
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
void DrawVerticalGradient(Drawable d, GC g,
                            long fromColor, long toColor,
                            int x, int y,
                            unsigned int width, unsigned int height);


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
                            int x, int y,
                            unsigned int width, unsigned int height);

#endif /* GRADIENT_H */

