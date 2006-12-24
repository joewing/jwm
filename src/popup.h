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

void SetPopupEnabled(int e);
void SetPopupDelay(const char *str);

void SignalPopup(const struct TimeType *now, int x, int y);
int ProcessPopupEvent(const XEvent *event);

extern int popupDelay;

#endif

