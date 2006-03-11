/***************************************************************************
 ***************************************************************************/

#ifndef TRAY_BUTTON_H
#define TRAY_BUTTON_H

struct TrayComponentType;
struct TimeType;

void InitializeTrayButtons();
void StartupTrayButtons();
void ShutdownTrayButtons();
void DestroyTrayButtons();

struct TrayComponentType *CreateTrayButton(
	const char *iconName, const char *label, const char *action,
	const char *popup, int width, int height);

void SignalTrayButton(struct TimeType *now, int x, int y);

#endif

