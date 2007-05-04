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
   int x, int y, unsigned int width, unsigned int height) {

   unsigned int line;
   XColor temp;
   float red, green, blue;
   float ared, agreen, ablue;
   float bred, bgreen, bblue;
   float multiplier;
   float amult, bmult;

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
   ared = (float)temp.red / 65535.0;
   agreen = (float)temp.green / 65535.0;
   ablue = (float)temp.blue / 65535.0;

   /** Load the "to" color. */
   temp.pixel = toColor;
   GetColorFromPixel(&temp);
   bred = (float)temp.red / 65535.0;
   bgreen = (float)temp.green / 65535.0;
   bblue = (float)temp.blue / 65535.0;

   multiplier = 1.0 / height;

   /* Loop over each line. */
   for(line = 0; line < height; line++) {

      bmult = line * multiplier;
      amult = 1.0 - bmult;

      /* Determine the color for this line. */
      red = ared * amult + bred * bmult;
      green = agreen * amult + bgreen * bmult;
      blue = ablue * amult + bblue * bmult;

      temp.red = (unsigned short)(red * 65535.9);
      temp.green = (unsigned short)(green * 65535.9);
      temp.blue = (unsigned short)(blue * 65535.9);

      GetColor(&temp);

      /* Draw the line. */
      JXSetForeground(display, g, temp.pixel);
      JXDrawLine(display, d, g, x, y + line, x + width, y + line);

   }

}


