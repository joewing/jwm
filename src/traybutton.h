/**
 * @file traybutton.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Tray button tray component.
 *
 */

#ifndef TRAY_BUTTON_H
#define TRAY_BUTTON_H

struct TrayComponentType;
struct TimeType;

/*@{*/
void InitializeTrayButtons();
void StartupTrayButtons();
void ShutdownTrayButtons();
void DestroyTrayButtons();
/*@}*/

struct TrayComponentType *CreateTrayButton(
	const char *iconName, const char *label, const char *action,
	const char *popup, int width, int height);

void SignalTrayButton(const struct TimeType *now, int x, int y);

#endif

