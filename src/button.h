/**
 * @file button.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header file for button functions.
 *
 */

#ifndef BUTTON_H
#define BUTTON_H

#include "font.h"

struct IconNode;

/** Button types. */
typedef enum {
   BUTTON_LABEL,        /**< Label. */
   BUTTON_MENU,         /**< Menu item. */
   BUTTON_MENU_ACTIVE,  /**< Active menu item. */
   BUTTON_TASK,         /**< Item in the task list. */
   BUTTON_TASK_ACTIVE   /**< Active item in the task list. */
} ButtonType;

/** Alignment of content in a button. */
typedef enum {
   ALIGN_LEFT,   /**< Left align. */
   ALIGN_CENTER  /**< Center align. */
} AlignmentType;

/** Data used for drawing a button. */
typedef struct {

   ButtonType type;         /**< The type of button to draw. */
   Drawable drawable;       /**< The place to put the button. */
   GC gc;                   /**< Graphics context used for drawing. */
   FontType font;           /**< The font for button text. */
   AlignmentType alignment; /**< Alignment of the button content. */

   int x, y;           /**< The coordinates to render the button. */
   int width, height;  /**< The size of the button. */

   struct IconNode *icon;  /**< Icon used in the button. */
   const char *text;       /**< Text used in the button. */

} ButtonNode;

/** Draw a button.
 * @param bp The button to draw.
 */
void DrawButton(ButtonNode *bp);

/** Reset the contents of a ButtonNode structure.
 * @param bp The structure to reset.
 * @param d The drawable to use.
 * @param g The graphics context to use.
 */
void ResetButton(ButtonNode *bp, Drawable d, GC g);

#endif /* BUTTON_H */

