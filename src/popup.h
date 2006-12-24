/**
 * @file popup.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for popup functions.
 *
 */

#ifndef POPUP_H
#define POPUP_H

/** Number of pixels the mouse can move before the popup disappears. */
#define POPUP_DELTA 2

struct TimeType;

/*@{*/
void InitializePopup();
void StartupPopup();
void ShutdownPopup();
void DestroyPopup();
/*@}*/

/** Show a popup window.
 * @param x The x coordinate of the popup window.
 * @param y The y coordinate of the popup window.
 * @param text The text to display in the popup.
 */
void ShowPopup(int x, int y, const char *text);

/** Set whether or not popups are enabled.
 * @param e 1 to enable popups, 0 to disable popups.
 */
void SetPopupEnabled(int e);

/** Set the delay before showing popups.
 * @param str The delay (ASCII, milliseconds).
 */
void SetPopupDelay(const char *str);

/** Signal the popup window.
 * @param now The effective time of the signal.
 * @param x The x-coordinate of the mouse.
 * @param y The y-coordinate of the mouse.
 */
void SignalPopup(const struct TimeType *now, int x, int y);

/** Process a popup event.
 * @param event The event to process.
 * @return 1 if handled, 0 otherwise.
 */
int ProcessPopupEvent(const XEvent *event);

/** The popup delay in milliseconds. */
extern int popupDelay;

#endif /* POPUP_H */

