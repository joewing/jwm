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

/** Enumeration of tray layouts. */
typedef enum {
   LAYOUT_HORIZONTAL,   /**< Left-to-right. */
   LAYOUT_VERTICAL      /**< Top-to-bottom. */
} LayoutType;

/** Enumeration of tray alignments. */
typedef enum {
   TALIGN_FIXED,     /**< Fixed at user specified x and y coordinates. */
   TALIGN_LEFT,      /**< Left aligned. */
   TALIGN_TOP,       /**< Top aligned. */
   TALIGN_CENTER,    /**< Center aligned. */
   TALIGN_RIGHT,     /**< Right aligned. */
   TALIGN_BOTTOM     /**< Bottom aligned. */
} TrayAlignmentType;

/** Structure to hold common tray component data.
 * Sizing is handled as follows:
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
typedef struct TrayComponentType {

   /** The tray containing the component.
    * UpdateSpecificTray(TrayType*, TrayComponentType*) should be called
    * when content changes.
    */
   struct TrayType *tray;

   /** Additional information needed for the component. */
   void *object;

   int x;   /**< x-coordinate on the tray (valid only after Create). */
   int y;   /**< y-coordinate on the tray (valid only after Create). */

   int screenx;   /**< x-coordinate on the screen (valid only after Create). */
   int screeny;   /**< y-coordinate on the screen (valid only after Create). */


   int requestedWidth;  /**< Requested width. */
   int requestedHeight; /**< Requested height. */

   int width;     /**< Actual width. */
   int height;    /**< Actual height. */

   int grabbed;   /**< 1 if the mouse was grabbed by this component. */

   Window window;    /**< Content (if a window, otherwise None). */
   Pixmap pixmap;    /**< Content (if a pixmap, otherwise None). */

   /** Callback to create the component. */
   void (*Create)(struct TrayComponentType *cp);

   /** Callback to destroy the component. */
   void (*Destroy)(struct TrayComponentType *cp);

   /** Callback to set the size known so far.
    * This is needed for items that maintain width/height ratios.
    * Either width or height may be zero.
    * This is called before Create.
    */
   void (*SetSize)(struct TrayComponentType *cp, int width, int height);

   /** Callback to resize the component. */
   void (*Resize)(struct TrayComponentType *cp);

   /** Callback for mouse presses. */
   void (*ProcessButtonPress)(struct TrayComponentType *cp,
      int x, int y, int mask);

   /** Callback for mouse releases. */
   void (*ProcessButtonRelease)(struct TrayComponentType *cp,
      int x, int y, int mask);

   /** Callback for mouse motion. */
   void (*ProcessMotionEvent)(struct TrayComponentType *cp,
      int x, int y, int mask);

   /** The next component in the tray. */
   struct TrayComponentType *next;

} TrayComponentType;

/** Structure to represent a tray. */
typedef struct TrayType {

   int x;   /**< The x-coordinate of the tray. */
   int y;   /**< The y-coordinate of the tray. */

   int requestedWidth;  /**< Total requested width of the tray. */
   int requestedHeight; /**< Total requested height of the tray. */

   int width;     /**< Actual width of the tray. */
   int height;    /**< Actual height of the tray. */

   int border;                /**< Border size in pixels. */
   WinLayerType layer;        /**< Layer. */
   LayoutType layout;         /**< Layout. */
   TrayAlignmentType valign;  /**< Vertical alignment. */
   TrayAlignmentType halign;  /**< Horizontal alignment. */

   int autoHide;  /**< 1 for autohide, 0 otherwise. */
   int hidden;    /**< 1 if hidden (due to autohide), 0 otherwise. */

   Window window; /**< The tray window. */

   /** Start of the tray components. */
   struct TrayComponentType *components;

   /** End of the tray components. */
   struct TrayComponentType *componentsTail;

   struct TrayType *next;  /**< Next tray. */

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

/** Show all trays. */
void ShowAllTrays();

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

/** Get the number of trays.
 * @return The number of trays.
 */
int GetTrayCount();

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

/** Set whether auto hide is enabled for a tray.
 * @param tp The tray.
 * @param v 1 to enable, 0 to disable.
 */
void SetAutoHideTray(TrayType *tp, int v);

/** Set the tray x-coordinate.
 * @param tp The tray.
 * @param str The x-coordinate (ASCII, pixels, negative ok).
 */
void SetTrayX(TrayType *tp, const char *str);

/** Set the tray y-coordinate.
 * @param tp The tray.
 * @param str The y-coordinate (ASCII, pixels, negative ok).
 */
void SetTrayY(TrayType *tp, const char *str);

/** Set the tray width.
 * @param tp The tray.
 * @param str The width (ASCII, pixels).
 */
void SetTrayWidth(TrayType *tp, const char *str);

/** Set the tray height.
 * @param tp The tray.
 * @param str The height (ASCII, pixels).
 */
void SetTrayHeight(TrayType *tp, const char *str);

/** Set the tray layout.
 * @param tp The tray.
 * @param str A string representation of the layout to use.
 */
void SetTrayLayout(TrayType *tp, const char *str);

/** Set the tray layer.
 * @param tp The tray.
 * @param str The layer (ASCII).
 */
void SetTrayLayer(TrayType *tp, const char *str);

/** Set the tray border size.
 * @param tp The tray.
 * @param str The border size (ASCII, pixels).
 */
void SetTrayBorder(TrayType *tp, const char *str);

/** Set the tray horizontal alignment.
 * @param tp The tray.
 * @param str The alignment(ASCII).
 */
void SetTrayHorizontalAlignment(TrayType *tp, const char *str);

/** Set the tray vertical alignment.
 * @param tp The tray.
 * @param str The alignment(ASCII).
 */
void SetTrayVerticalAlignment(TrayType *tp, const char *str);

/** Set the tray transparency level.
 * @param str The value (ASCII).
 */
void SetTrayOpacity(const char *str);

#endif /* TRAY_H */

