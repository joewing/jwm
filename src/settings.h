/**
 * @file settings.h
 * @author Joe Wingbermuehle
 * @date 2012
 *
 * @brief JWM settings.
 *
 */

#ifndef SETTINGS_H
#define SETTINGS_H

/** Border button image masks. */
typedef enum {
   BP_CLOSE,
   BP_MINIMIZE,
   BP_MAXIMIZE,
   BP_MAXIMIZE_ACTIVE,
   BP_COUNT
} BorderPixmapType;

/** Window snap modes. */
typedef enum {
   SNAP_NONE,     /**< Don't snap. */
   SNAP_SCREEN,   /**< Snap to the edges of the screen. */
   SNAP_BORDER    /**< Snap to all borders. */
} SnapModeType;

/** Window move modes. */
typedef enum {
   MOVE_OPAQUE,   /**< Show window contents while moving. */
   MOVE_OUTLINE   /**< Show an outline while moving. */
} MoveModeType;

/** Window resize modes. */
typedef enum {
   RESIZE_OPAQUE,   /**< Show window contents while resizing. */
   RESIZE_OUTLINE   /**< Show an outline while resizing. */
} ResizeModeType;

/** Status window types. */
typedef enum {
   SW_OFF,        /**< No status window. */
   SW_SCREEN,     /**< Centered on screen. */
   SW_WINDOW,     /**< Centered on window. */
   SW_CORNER      /**< Upper-left corner. */
} StatusWindowType;

/** Focus models. */
typedef enum {
   FOCUS_SLOPPY,
   FOCUS_CLICK
} FocusModelType;

/** Settings. */
typedef struct {
   char *borderButtonBitmaps[BP_COUNT];
   unsigned int doubleClickSpeed;
   unsigned int doubleClickDelta;
   unsigned int snapDistance;
   unsigned int popupDelay;
   unsigned int trayOpacity;
   unsigned int activeClientOpacity;
   unsigned int minClientOpacity;
   unsigned int maxClientOpacity;
   unsigned int deltaClientOpacity;
   unsigned int borderWidth;
   unsigned int titleHeight;
   unsigned int desktopWidth;
   unsigned int desktopHeight;
   unsigned int desktopCount;
   SnapModeType snapMode;
   MoveModeType moveMode;
   StatusWindowType moveStatusType;
   StatusWindowType resizeStatusType;
   FocusModelType focusModel;
   ResizeModeType resizeMode;
   char popupEnabled;
} Settings;

extern Settings settings;

/*@{*/
void InitializeSettings();
void StartupSettings();
void ShutdownSettings();
void DestroySettings();
/*@}*/

/** Update a string setting. */
void SetPathString(char **dest, const char *src);

#endif

