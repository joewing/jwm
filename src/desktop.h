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
void ChangeDesktop(unsigned int desktop);

struct MenuType *CreateDesktopMenu(unsigned int mask);

void SetDesktopCount(const char *str);
void SetDesktopName(unsigned int desktop, const char *str);

#endif

