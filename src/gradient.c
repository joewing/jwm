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

unsigned int gradients[GRADIENT_COUNT];

/** Set the color to use for a component. */
void SetGradient(GradientType c, const int value)
{
   gradients[c] = value;
}

/** Draw a gradient. */
void DrawGradient(Drawable d, GC g,
                            long fromColor, long toColor,
                            int x, int y,
                            unsigned int width, unsigned int height,
                            int type)
{

   const int shift = 15;
   unsigned int counter;
   unsigned int limit;
   XColor colors[2];
   int red, green, blue;
   int ared, agreen, ablue;
   int bred, bgreen, bblue;
   int redStep, greenStep, blueStep;

   /* Return if there's nothing to do or if the background was filled elsewhere. */
   if((width == 0 || height == 0) || (fromColor == toColor)) {
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
   if (type == GRADIENT_VERTICAL) {
      redStep = (bred - ared) / (int)height;
      greenStep = (bgreen - agreen) / (int)height;
      blueStep = (bblue - ablue) / (int)height;
      limit = height;
   } else {
      redStep = (bred - ared) / (int)width;
      greenStep = (bgreen - agreen) / (int)width;
      blueStep = (bblue - ablue) / (int)width;
      limit = width - 1;
   }

   /* Loop over each line or column. */
   red = ared;
   blue = ablue;
   green = agreen;
   for(counter = 0; counter < limit; counter++) {

      /* Determine the color for this line. */
      colors[0].red = (unsigned short)(red >> shift);
      colors[0].green = (unsigned short)(green >> shift);
      colors[0].blue = (unsigned short)(blue >> shift);

      GetColor(&colors[0]);

      /* Draw the line. */
      JXSetForeground(display, g, colors[0].pixel);
      if (type == GRADIENT_VERTICAL) {
         JXDrawLine(display, d, g, x, y + counter, x + width - 1, y + counter);
      } else {
         JXDrawLine(display, d, g, x + counter, y, x + counter + 1, y  + height - 1);
      }
      
      red += redStep;
      green += greenStep;
      blue += blueStep;

   }
}
