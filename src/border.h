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
struct ClientState;

/** Border icon types. */
typedef unsigned char BorderIconType;
#define BI_CLOSE        0
#define BI_MAX          1
#define BI_MAX_ACTIVE   2
#define BI_MENU         3
#define BI_MIN          4
#define BI_COUNT        5

/** Flags to determine what action to take on the border. */
typedef unsigned char BorderActionType;
#define BA_NONE      0     /**< Do nothing. */
#define BA_RESIZE    1     /**< Resize the window. */
#define BA_MOVE      2     /**< Move the window. */
#define BA_CLOSE     3     /**< Close the window. */
#define BA_MAXIMIZE  4     /**< Maximize the window. */
#define BA_MINIMIZE  5     /**< Minimize the window. */
#define BA_MENU      6     /**< Show the window menu. */
#define BA_RESIZE_N  0x10  /**< Resize north. */
#define BA_RESIZE_S  0x20  /**< Resize south. */
#define BA_RESIZE_E  0x40  /**< Resize east. */
#define BA_RESIZE_W  0x80  /**< Resize west. */

/*@{*/
void InitializeBorders(void);
void StartupBorders(void);
#define ShutdownBorders()      (void)(0)
void DestroyBorders(void);
/*@}*/

/** Determine the action to take for a client.
 * @param np The client.
 * @param x The x-coordinate of the mouse (frame relative).
 * @param y The y-coordinate of the mouse (frame relative).
 * @return The action to take.
 */
BorderActionType GetBorderActionType(const struct ClientNode *np, int x, int y);

/** Reset the shape of a window border.
 * @param np The client.
 */
void ResetBorder(const struct ClientNode *np);

/** Draw a window border.
 * @param np The client whose frame to draw.
 */
void DrawBorder(struct ClientNode *np);

/** Get the size of a border icon.
 * @return The size in pixels (note that icons are square).
 */
int GetBorderIconSize(void);

/** Get the size of a window border.
 * @param state The client state.
 * @param north Pointer to the value to contain the north border size.
 * @param south Pointer to the value to contain the south border size.
 * @param east Pointer to the value to contain the east border size.
 * @param west Pointer to the value to contain the west border size.
 */
void GetBorderSize(const struct ClientState *state,
                   int *north, int *south, int *east, int *west);

/** Redraw all borders on the current desktop. */
void ExposeCurrentDesktop(void);

/** Draw a rounded rectangle.
 * @param d The drawable on which to render.
 * @param gc The graphics context.
 * @param x The x-coodinate.
 * @param y The y-coordinate.
 * @param width The width.
 * @param height The height.
 * @param radius The corner radius.
 */
void DrawRoundedRectangle(Drawable d, GC gc, int x, int y,
                          int width, int height, int radius);

/** Set the icon to use for a border button. */
void SetBorderIcon(BorderIconType t, const char *name);

#endif /* BORDER_H */

