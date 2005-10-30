/****************************************************************************
 ****************************************************************************/

#ifndef ICON_H
#define ICON_H

struct ClientNode;

/****************************************************************************
 ****************************************************************************/
typedef struct ScaledIconNode {

	int width;
	int height;

	Pixmap image;
	Pixmap mask;
#ifdef USE_XRENDER
	Picture imagePicture;
	Picture maskPicture;
#endif

	struct ScaledIconNode *next;

} ScaledIconNode;

/****************************************************************************
 ****************************************************************************/
typedef struct IconNode {

	char *name;

	struct ImageNode *image;

	struct ScaledIconNode *nodes;
	
	struct IconNode *next;
	struct IconNode *prev;

} IconNode;

#ifdef USE_ICONS

void InitializeIcons();
void StartupIcons();
void ShutdownIcons();
void DestroyIcons();

void AddIconPath(const char *path);

void PutIcon(IconNode *icon, Drawable d, GC g, int x, int y,
	int width, int height);

void LoadIcon(struct ClientNode *np);
IconNode *LoadNamedIcon(const char *name);
void DestroyIcon(IconNode *icon);

IconNode *CreateIcon();

#else

#define ICON_DUMMY_FUNCTION 0

#define InitializeIcons()        ICON_DUMMY_FUNCTION
#define DestroyIcons()           ICON_DUMMY_FUNCTION
#define AddIconPath( a )         ICON_DUMMY_FUNCTION
#define PutIcon( a, b, c, d, e ) ICON_DUMMY_FUNCTION
#define LoadIcon( a )            ICON_DUMMY_FUNCTION
#define LoadNamedIcon( a )       ICON_DUMMY_FUNCTION
#define DestroyIcon( a )         ICON_DUMMY_FUNCTION

#endif /* USE_ICONS */

#endif /* ICON_H */

