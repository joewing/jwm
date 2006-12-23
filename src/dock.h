/**
 * @file dock.h
 * @author Joe Wingbermuehle
 * @date 2006
 *
 * @brief Header for the dock functions.
 *
 */

#ifndef DOCK_H
#define DOCK_H

struct TrayComponentType;

/*@{*/
void InitializeDock();
void StartupDock();
void ShutdownDock();
void DestroyDock();
/*@}*/

/** Create a dock to be used for notifications.
 * Note that only one dock can be created.
 */
struct TrayComponentType *CreateDock();

/** Handle a client message sent to the dock window.
 * @param event The event.
 */
void HandleDockEvent(const XClientMessageEvent *event);

int HandleDockDestroy(Window win);

int HandleDockSelectionClear(const XSelectionClearEvent *event);

int HandleDockResizeRequest(const XResizeRequestEvent *event);

int HandleDockConfigureRequest(const XConfigureRequestEvent *event);

int HandleDockReparentNotify(const XReparentEvent *event);

#endif

