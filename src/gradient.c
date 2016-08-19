/**
 * @file gradient.c
 * @author Joe Wingbermuehle
 * @date 2006
 *
 * @brief Gradient fill functions.
 *
 */

#include "jwm.h"
#include "gradient.h"
#include "color.h"
#include "main.h"

/** Draw a horizontal gradient. */
void DrawHorizontalGradient(Drawable d, GC g,
                            long fromColor, long toColor,
                            int x, int y,
                            unsigned int width, unsigned int height)
{

   const int shift = 15;
   unsigned int line;
   XColor colors[2];
   int red, green, blue;
   int ared, agreen, ablue;
   int bred, bgreen, bblue;
   int redStep, greenStep, blueStep;

   /* Return if there's nothing to do. */
   if(width == 0 || height == 0) {
      return;
   }

   /* Here we assume that the background was filled elsewhere. */
   if(fromColor == toColor) {
      return;
   }

   /* Query the from/to colors. */
   colors[0].pixel = fromColor;
   colors[1].pixel = toColor;
   JXQueryColors(display, rootColormap, colors, 2);

   /* Set the "from" color. */
   ared = (unsigned int)colors[0].red << shift;
   agreen = (unsigned int)colors[0].green << shift;
   ablue = (unsigned int)colors[0].blue << shift;

   /* Set the "to" color. */
   bred = (unsigned int)colors[1].red << shift;
   bgreen = (unsigned int)colors[1].green << shift;
   bblue = (unsigned int)colors[1].blue << shift;

   /* Determine the step. */
   redStep = (bred - ared) / (int)height;
   greenStep = (bgreen - agreen) / (int)height;
   blueStep = (bblue - ablue) / (int)height;

   /* Loop over each line. */
   red = ared;
   blue = ablue;
   green = agreen;
   for(line = 0; line < height; line++) {

      /* Determine the color for this line. */
      colors[0].red = (unsigned short)(red >> shift);
      colors[0].green = (unsigned short)(green >> shift);
      colors[0].blue = (unsigned short)(blue >> shift);

      GetColor(&colors[0]);

      /* Draw the line. */
      JXSetForeground(display, g, colors[0].pixel);
      JXDrawLine(display, d, g, x, y + line, x + width - 1, y + line);

      red += redStep;
      green += greenStep;
      blue += blueStep;

   }
}
