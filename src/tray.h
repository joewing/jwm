/***************************************************************************
 * Header for the tray functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ***************************************************************************/

#ifndef TRAY_H
#define TRAY_H

#include "hint.h"

struct TimeType;

typedef enum {
	LAYOUT_HORIZONTAL,
	LAYOUT_VERTICAL
} LayoutType;

typedef enum {
	TALIGN_FIXED,
	TALIGN_LEFT,
	TALIGN_TOP,
	TALIGN_CENTER,
	TALIGN_RIGHT,
	TALIGN_BOTTOM
} TrayAlignmentType;

typedef struct TrayComponentType {

	/* The tray containing the component.
	 * UpdateSpecificTray(TrayType*, TrayComponentType*) should be called
	 * when content changes.
	 */
	struct TrayType *tray;

	/* Additional information needed for the component. */
	void *object;

	/* Coordinates on the tray (valid only after Create). */
	int x, y;

	/* Coordinates on the screen (valid only after Create). */
	int screenx, screeny;

	/* Sizing is handled as follows:
	 *  - The component is created via a factory method. It sets its
	 *    requested size (0 for no preference).
	 *  - The SetSize callback is issued with size constraints
	 *    (0 for no constraint). The component should update
	 *    width and height in SetSize.
	 *  - The Create callback is issued with finalized size information.
	 * Resizing is handled as follows:
	 *  - A component determines that it needs to change size. It updates
	 *    its requested size (0 for no preference).
	 *  - The component calls ResizeTray.
	 *  - The SetSize callback is issued with size constraints
	 *    (0 for no constraint). The component should update
	 *    width and height in SetSize.
	 *  - The Resize callback is issued with finalized size information.
	 */

	/* Requested component size. */
	int requestedWidth, requestedHeight;

	/* Actual component size. */
	int width, height;

	/* Content. */
	Window window;
	Pixmap pixmap;

	/* Create the component. */
	void (*Create)(struct TrayComponentType *cp);

	/* Destroy the component. */
	void (*Destroy)(struct TrayComponentType *cp);

	/* Set the size known so far for items that need width/height ratios.
	 * Either width or height may be zero.
	 * This is called before Create.
	 */
	void (*SetSize)(struct TrayComponentType *cp, int width, int height);

	/* Resize the component. */
	void (*Resize)(struct TrayComponentType *cp);

	/* Callback for mouse clicks. */
	void (*ProcessButtonEvent)(struct TrayComponentType *cp,
		int x, int y, int mask);

	/* Callback for mouse motion. */
	void (*ProcessMotionEvent)(struct TrayComponentType *cp,
		int x, int y, int mask);

	/* The next component in the tray. */
	struct TrayComponentType *next;

} TrayComponentType;

typedef struct TrayType {

	int x, y;
	int requestedWidth, requestedHeight;
	int width, height;
	WinLayerType layer;
	LayoutType layout;
	TrayAlignmentType valign, halign;

	int autoHide;
	int hidden;

	Window window;
	GC gc;

	struct TrayComponentType *components;
	struct TrayComponentType *componentsTail;

	struct TrayType *next;

} TrayType;


void InitializeTray();
void StartupTray();
void ShutdownTray();
void DestroyTray();

TrayType *CreateTray();
TrayComponentType *CreateTrayComponent();
void AddTrayComponent(TrayType *tp, TrayComponentType *cp);

void ShowTray(TrayType *tp);
void HideTray(TrayType *tp);

void DrawTray();
void DrawSpecificTray(const TrayType *tp);
void UpdateSpecificTray(const TrayType *tp, const TrayComponentType *cp);

void ResizeTray(TrayType *tp);

TrayType *GetTrays();

Window GetSupportingWindow();

int ProcessTrayEvent(const XEvent *event);

void SignalTray(struct TimeType *now, int x, int y);

void SetAutoHideTray(TrayType *tp, int v);
void SetTrayX(TrayType *tp, const char *str);
void SetTrayY(TrayType *tp, const char *str);
void SetTrayWidth(TrayType *tp, const char *str);
void SetTrayHeight(TrayType *tp, const char *str);
void SetTrayLayout(TrayType *tp, const char *str);
void SetTrayLayer(TrayType *tp, const char *str);
void SetTrayBorder(TrayType *tp, const char *str);
void SetTrayHorizontalAlignment(TrayType *tp, const char *str);
void SetTrayVerticalAlignment(TrayType *tp, const char *str);

#endif

