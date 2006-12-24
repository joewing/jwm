/**
 * @file border.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header file for border functions.
 *
 */

#ifndef BORDER_H
#define BORDER_H

#include "gradient.h"

struct ClientNode;

/** Flags to determine what action to take on the border. */
typedef enum {
   BA_NONE      = 0,      /**< Do nothing. */
   BA_RESIZE    = 1,      /**< Resize the window. */
   BA_MOVE      = 2,      /**< Move the window. */
   BA_CLOSE     = 3,      /**< Close the window. */
   BA_MAXIMIZE  = 4,      /**< Maximize the window. */
   BA_MINIMIZE  = 5,      /**< Minimize the window. */
   BA_MENU      = 6,      /**< Show the window menu. */
   BA_RESIZE_N  = 0x10,   /**< Resize north. */
   BA_RESIZE_S  = 0x20,   /**< Resize south. */
   BA_RESIZE_E  = 0x40,   /**< Resize east. */
   BA_RESIZE_W  = 0x80    /**< Resize west. */
} BorderActionType;

/*@{*/
void InitializeBorders();
void StartupBorders();
void ShutdownBorders();
void DestroyBorders();
/*@}*/

/** Determine the action to take for a client.
 * @param np The client.
 * @param x The x-coordinate of the mouse (frame relative).
 * @param y The y-coordinate of the mouse (frame relative).
 * @return The action to take.
 */
BorderActionType GetBorderActionType(const struct ClientNode *np, int x, int y);

/** Draw a window border.
 * @param np The client whose frame to draw.
 * @param expose The expose event causing the redraw (or NULL).
 */
void DrawBorder(const struct ClientNode *np, const XExposeEvent *expose);

/** Get the size of a border icon.
 * @return The size in pixels (note that icons are square).
 */
int GetBorderIconSize();

/** Get the size of a window border.
 * @param np The client.
 * @param north Pointer to the value to contain the north border size.
 * @param south Pointer to the value to contain the south border size.
 * @param east Pointer to the value to contain the east border size.
 * @param west Pointer to the value to contain the west border size.
 */
void GetBorderSize(const struct ClientNode *np,
   int *north, int *south, int *east, int *west);

/** Set the size of window borders.
 * @param str The size to use in string form.
 */
void SetBorderWidth(const char *str);

/** Set the size of window title bars.
 * @param str The size to use in string form.
 */
void SetTitleHeight(const char *str);

/** Redraw all borders on the current desktop. */
void ExposeCurrentDesktop();

#endif

