/***************************************************************************
 ***************************************************************************/

#ifndef DESKTOP_H
#define DESKTOP_H

struct MenuType;

extern char **desktopNames;

void InitializeDesktops();
void StartupDesktops();
void ShutdownDesktops();
void DestroyDesktops();

void NextDesktop();
void PreviousDesktop();
void ChangeDesktop(int desktop);

void CreateDesktopMenu(const char *name, unsigned int mask,
	struct MenuType *menu);

void SetDesktopCount(const char *str);
void SetDesktopName(int desktop, const char *str);

#endif

