/***************************************************************************
 ***************************************************************************/

#ifndef TRAY_BUTTON_H
#define TRAY_BUTTON_H

void InitializeTrayButtons();
void StartupTrayButtons();
void ShutdownTrayButtons();
void DestroyTrayButtons();

TrayComponentType *CreateTrayButton(const char *iconName,
	const char *action);

#endif

