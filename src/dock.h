/***************************************************************************
 * Header for the dock functions.
 * Copyright (C) 2006 Joe Wingbermuehle
 ***************************************************************************/

#ifndef DOCK_H
#define DOCK_H

struct TrayComponentType;

void InitializeDock();
void StartupDock();
void ShutdownDock();
void DestroyDock();

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

#endif

