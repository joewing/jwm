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

/*@{*/
void InitializePopup();
void StartupPopup();
void ShutdownPopup();
void DestroyPopup();
/*@}*/

/** Show a popup window.
 * @param x The x coordinate of the left edge of the popup window.
 * @param y The y coordinate of the bottom edge of the popup window.
 * @param text The text to display in the popup.
 */
void ShowPopup(int x, int y, const char *text);

/** Process a popup event.
 * @param event The event to process.
 * @return 1 if handled, 0 otherwise.
 */
char ProcessPopupEvent(const XEvent *event);

#endif /* POPUP_H */

