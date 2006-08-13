/**
 * @file desktop.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the desktop management functions.
 *
 */

#ifndef DESKTOP_H
#define DESKTOP_H

struct MenuType;

extern char **desktopNames;

/*@{*/
void InitializeDesktops();
void StartupDesktops();
void ShutdownDesktops();
void DestroyDesktops();
/*@}*/

/** Switch to the next desktop. */
void NextDesktop();

/** Switch to the previous desktop. */
void PreviousDesktop();

/** Switch to a specific desktop.
 * @param desktop The desktop to show (0 based).
 */
void ChangeDesktop(unsigned int desktop);

/** Toggle the "show desktop" state.
 * This will either minimize or restore all items on the current desktop.
 */
void ShowDesktop();

/** Create a menu containing a list of desktops.
 * @param mask A bit mask of desktops to highlight.
 * @return A menu containing all the desktops.
 */
struct Menu *CreateDesktopMenu(unsigned int mask);

/** Set the number of desktops.
 * This is called before startup.
 * @param str ASCII representation of the number of desktops.
 */
void SetDesktopCount(const char *str);

/** Set the name of a desktop.
 * This is called before startup.
 * @param desktop The desktop to name (0 based).
 * @param str The name to assign.
 */
void SetDesktopName(unsigned int desktop, const char *str);

#endif

