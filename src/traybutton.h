/***************************************************************************
 ***************************************************************************/

#ifndef TRAY_BUTTON_H
#define TRAY_BUTTON_H

struct TrayComponentType;

void InitializeTrayButtons();
void StartupTrayButtons();
void ShutdownTrayButtons();
void DestroyTrayButtons();

struct TrayComponentType *CreateTrayButton(const char *iconName,
	const char *action);

#endif

