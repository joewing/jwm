/**
 * @file icon.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header file for icon functions.
 *
 */

#ifndef ICON_H
#define ICON_H

struct ClientNode;

/** Structure to hold a scaled icon. */
typedef struct ScaledIconNode {

   int width;   /**< The scaled width of the icon. */
   int height;  /**< The scaled height of the icon. */

   Pixmap image;
   Pixmap mask;
#ifdef USE_XRENDER
   Picture imagePicture;
   Picture maskPicture;
#endif

   struct ScaledIconNode *next;

} ScaledIconNode;

/** Structure to hold an icon. */
typedef struct IconNode {

   char *name;                    /**< The name of the icon. */
   struct ImageNode *image;       /**< The image data. */
   struct ScaledIconNode *nodes;  /**< Scaled versions of the icon. */
	int useRender;						 /**< 1 if render can be used. */
   
   struct IconNode *next;         /**< The next icon in the list. */
   struct IconNode *prev;         /**< The previous icon in the list. */

} IconNode;

#ifdef USE_ICONS

/*@{*/
void InitializeIcons();
void StartupIcons();
void ShutdownIcons();
void DestroyIcons();
/*@}*/

/** Add an icon path.
 * This adds a path to the list of icon search paths.
 * @param path The icon path to add.
 */
void AddIconPath(char *path);

/** Render an icon.
 * This will scale an icon if necessary to fit the requested size. The
 * aspect ratio of the icon is preserved.
 * @param icon The icon to render.
 * @param d The drawable on which to place the icon.
 * @param x The x offset on the drawable to render the icon.
 * @param y The y offset on the drawable to render the icon.
 * @param width The width of the icon to display.
 * @param height The height of the icon to display.
 */
void PutIcon(IconNode *icon, Drawable d, int x, int y,
   int width, int height);

/** Load an icon for a client.
 * @param np The client.
 */
void LoadIcon(struct ClientNode *np);

/** Load an icon.
 * @param name The name of the icon to load.
 * @return A pointer to the icon (NULL if not found).
 */
IconNode *LoadNamedIcon(const char *name);

/** Destroy an icon.
 * @param icon The icon to destroy.
 */
void DestroyIcon(IconNode *icon);

/** Create and initialize a new icon structure.
 * @return The new icon structure.
 */
IconNode *CreateIcon();

#else

#define ICON_DUMMY_FUNCTION 0

#define InitializeIcons()               ICON_DUMMY_FUNCTION
#define StartupIcons()                  ICON_DUMMY_FUNCTION
#define ShutdownIcons()                 ICON_DUMMY_FUNCTION
#define DestroyIcons()                  ICON_DUMMY_FUNCTION
#define AddIconPath( a )                ICON_DUMMY_FUNCTION
#define PutIcon( a, b, c, d, e, f )     ICON_DUMMY_FUNCTION
#define LoadIcon( a )                   ICON_DUMMY_FUNCTION
#define LoadNamedIcon( a )              ICON_DUMMY_FUNCTION
#define DestroyIcon( a )                ICON_DUMMY_FUNCTION

#endif /* USE_ICONS */

#endif /* ICON_H */

