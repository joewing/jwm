/****************************************************************************
 ****************************************************************************/

#ifndef ICON_H
#define ICON_H

#ifdef USE_ICONS

void InitializeIcons();
void StartupIcons();
void ShutdownIcons();
void DestroyIcons();

void AddIconPath(const char *path);

void PutIcon(const IconNode *icon, Drawable d, GC g, int x, int y);

void LoadIcon(ClientNode *np);
IconNode *LoadNamedIcon(const char *name, int size);
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

