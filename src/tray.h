/***************************************************************************
 * Header for the tray functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ***************************************************************************/

#ifndef TRAY_H
#define TRAY_H

#include "hint.h"

typedef enum {
	LAYOUT_HORIZONTAL,
	LAYOUT_VERTICAL
} LayoutType;

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

	/* Component size.
	 * Sizing is handled as follows:
	 *  - The component is created via a factory method. It sets its
	 *    preferred size (0 for no preference).
	 *  - The SetSize callback is issued with size constraints
	 *    (0 for no constraint). The component should update its preference
	 *    during SetSize.
	 *  - The Create callback is issued with finalized size information.
	 */
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
	int width, height;
	int border;
	WinLayerType layer;
	LayoutType layout;

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

TrayType *GetTrays();

Window GetSupportingWindow();

int ProcessTrayEvent(const XEvent *event);

void SetAutoHideTray(TrayType *tp, int v);
void SetTrayX(TrayType *tp, const char *str);
void SetTrayY(TrayType *tp, const char *str);
void SetTrayWidth(TrayType *tp, const char *str);
void SetTrayHeight(TrayType *tp, const char *str);
void SetTrayLayout(TrayType *tp, const char *str);
void SetTrayLayer(TrayType *tp, const char *str);
void SetTrayBorder(TrayType *tp, const char *str);

#endif

