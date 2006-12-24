/**
 * @file tray.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the tray functions.
 *
 */

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
   int border;
   WinLayerType layer;
   LayoutType layout;
   TrayAlignmentType valign, halign;

   int autoHide;
   int hidden;

   Window window;

   struct TrayComponentType *components;
   struct TrayComponentType *componentsTail;

   struct TrayType *next;

} TrayType;


void InitializeTray();
void StartupTray();
void ShutdownTray();
void DestroyTray();

/** Create a new tray.
 * @return A new, empty tray.
 */
TrayType *CreateTray();

/** Create a tray component.
 * @return A new tray component structure.
 */
TrayComponentType *CreateTrayComponent();

/** Add a tray component to a tray.
 * @param tp The tray to update.
 * @param cp The tray component to add.
 */
void AddTrayComponent(TrayType *tp, TrayComponentType *cp);

/** Show a tray.
 * @param tp The tray to show.
 */
void ShowTray(TrayType *tp);

/** Hide a tray.
 * @param tp The tray to hide.
 */
void HideTray(TrayType *tp);

/** Draw all trays. */
void DrawTray();

/** Draw a specific tray.
 * @param tp The tray to draw.
 */
void DrawSpecificTray(const TrayType *tp);

/** Update a component on a tray.
 * @param tp The tray containing the component.
 * @param cp The component that needs updating.
 */
void UpdateSpecificTray(const TrayType *tp, const TrayComponentType *cp);

/** Resize a tray.
 * @param tp The tray to resize containing the new requested size information.
 */
void ResizeTray(TrayType *tp);

/** Get a linked list of trays.
 * @return The trays.
 */
TrayType *GetTrays();

/** Get a window to use as the supporting window.
 * This is used by clients to validate that compliant window manager is
 * running.
 * @return The supporting window.
 */
Window GetSupportingWindow();

/** Process an event that may be for a tray.
 * @param event The event to process.
 * @return 1 if this event was for a tray, 0 otherwise.
 */
int ProcessTrayEvent(const XEvent *event);

/** Signal the trays.
 * This function is called regularly so that autohide, etc. can take place.
 * @param now The current time.
 * @param x The mouse x-coordinate (root relative).
 * @param y The mouse y-coordinate (root relative).
 */
void SignalTray(const struct TimeType *now, int x, int y);

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

