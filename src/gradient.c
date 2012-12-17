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
   XColor temp;
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

   /* Load the "from" color. */
   temp.pixel = fromColor;
   GetColorFromPixel(&temp);
   ared = (unsigned int)temp.red << shift;
   agreen = (unsigned int)temp.green << shift;
   ablue = (unsigned int)temp.blue << shift;

   /* Load the "to" color. */
   temp.pixel = toColor;
   GetColorFromPixel(&temp);
   bred = (unsigned int)temp.red << shift;
   bgreen = (unsigned int)temp.green << shift;
   bblue = (unsigned int)temp.blue << shift;

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
      temp.red = (unsigned short)(red >> shift);
      temp.green = (unsigned short)(green >> shift);
      temp.blue = (unsigned short)(blue >> shift);

      GetColor(&temp);

      /* Draw the line. */
      JXSetForeground(display, g, temp.pixel);
      JXDrawLine(display, d, g, x, y + line, x + width, y + line);

      red += redStep;
      green += greenStep;
      blue += blueStep;

   }

}

