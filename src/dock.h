/**
 * @file dock.h
 * @author Joe Wingbermuehle
 * @date 2006
 *
 * @brief Dock tray component (used for system notifications).
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

/** Handle a destroy event.
 * @param win The window that was destroyed.
 * @return 1 if handled, 0 otherwise.
 */
int HandleDockDestroy(Window win);

/** Handle a selection clear event.
 * @param event The selection clear event.
 * @return 1 if handled, 0 otherwise.
 */
int HandleDockSelectionClear(const XSelectionClearEvent *event);

/** Handle a resize request.
 * @param event The resize request event.
 * @return 1 if handled, 0 otherwise.
 */
int HandleDockResizeRequest(const XResizeRequestEvent *event);

/** Handle a configure request.
 * @param event The configure request event.
 * @return 1 if handled, 0 otherwise.
 */
int HandleDockConfigureRequest(const XConfigureRequestEvent *event);

/** Handle a reparent notify event.
 * @param event The reparent notify event.
 * @return 1 if handled, 0 otherwise.
 */
int HandleDockReparentNotify(const XReparentEvent *event);

#endif

