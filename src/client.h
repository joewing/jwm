/**
 * @file client.h
 * @author Joe Wingbermuehle
 * @date 2004-2007
 *
 * @brief Header file client window functions.
 *
 */

#ifndef CLIENT_H
#define CLIENT_H

#include "border.h"
#include "hint.h"

/** Window border flags. */
typedef enum {
   BORDER_NONE    = 0,
   BORDER_OUTLINE = 1 << 0,   /**< Window has a border. */
   BORDER_TITLE   = 1 << 1,   /**< Window has a title bar. */
   BORDER_MIN     = 1 << 2,   /**< Window supports minimize. */
   BORDER_MAX     = 1 << 3,   /**< Window supports maximize. */
   BORDER_CLOSE   = 1 << 4,   /**< Window supports close. */
   BORDER_RESIZE  = 1 << 5,   /**< Window supports resizing. */
   BORDER_MOVE    = 1 << 6,   /**< Window supports moving. */
   BORDER_MAX_V   = 1 << 7,   /**< Maximize vertically. */
   BORDER_MAX_H   = 1 << 8    /**< Maximize horizontally. */
} BorderFlags;

/** The default border flags. */
#define BORDER_DEFAULT ( \
        BORDER_OUTLINE  \
      | BORDER_TITLE    \
      | BORDER_MIN      \
      | BORDER_MAX      \
      | BORDER_CLOSE    \
      | BORDER_RESIZE   \
      | BORDER_MOVE     \
      | BORDER_MAX_V    \
      | BORDER_MAX_H    )

/** Window status flags. */
typedef enum {
   STAT_NONE       = 0,
   STAT_ACTIVE     = 1 << 0,  /**< This client has focus. */
   STAT_MAPPED     = 1 << 1,  /**< This client is shown (on some desktop). */
   STAT_HMAX       = 1 << 2,  /**< This client is maximized horizonatally. */
   STAT_VMAX       = 1 << 3,  /**< This client is maximized vertically. */
   STAT_HIDDEN     = 1 << 4,  /**< This client is not on the current desktop. */
   STAT_STICKY     = 1 << 5,  /**< This client is on all desktops. */
   STAT_NOLIST     = 1 << 6,  /**< Skip this client in the task list. */
   STAT_MINIMIZED  = 1 << 7,  /**< This client is minimized. */
   STAT_SHADED     = 1 << 8,  /**< This client is shaded. */
   STAT_WMDIALOG   = 1 << 9,  /**< This is a JWM dialog window. */
   STAT_PIGNORE    = 1 << 10, /**< Ignore the program-specified position. */
   STAT_SHAPE      = 1 << 11, /**< This client uses the shape extension. */
   STAT_SDESKTOP   = 1 << 12, /**< This client was minimized to show desktop. */
   STAT_FULLSCREEN = 1 << 13, /**< This client wants to be full screen. */
   STAT_OPACITY    = 1 << 14, /**< This client has a fixed opacity. */
   STAT_NOFOCUS    = 1 << 15  /**< Don't focus on map. */
} StatusFlags;

/** Colormap window linked list. */
typedef struct ColormapNode {
   Window window;             /**< A window containing a colormap. */
   struct ColormapNode *next; /**< Next value in the linked list. */
} ColormapNode;

/** The aspect ratio of a window. */
typedef struct AspectRatio {
   int minx;   /**< The x component of the minimum aspect ratio. */
   int miny;   /**< The y component of the minimum aspect ratio. */
   int maxx;   /**< The x component of the maximum aspect ratio. */
   int maxy;   /**< The y component of the maximum aspect ratio. */
} AspectRatio;

/** Struture to store information about a client window. */
typedef struct ClientNode {

   Window window;             /**< The client window. */
   Window parent;             /**< The frame window. */

   Window owner;              /**< The owner window (for transients). */

   int x, y;                  /**< The location of the window. */
   int width;                 /**< The width of the window. */
   int height;                /**< The height of the window. */
   int oldx;                  /**< The old x coordinate (for maximize). */
   int oldy;                  /**< The old y coordinate (for maximize). */
   int oldWidth;              /**< The old width (for maximize). */
   int oldHeight;             /**< The old height (for maximize). */

   long sizeFlags;            /**< Size flags from XGetWMNormalHints. */
   int baseWidth;             /**< Base width for resizing. */
   int baseHeight;            /**< Base height for resizing. */
   int minWidth;              /**< Minimum width of this window. */
   int minHeight;             /**< Minimum height of this window. */
   int maxWidth;              /**< Maximum width of this window. */
   int maxHeight;             /**< Maximum height of this window. */
   int xinc;                  /**< Resize x increment. */
   int yinc;                  /**< Resize y increment. */
   AspectRatio aspect;        /**< Aspect ratio. */
   int gravity;               /**< Gravity for reparenting. */

   Colormap cmap;             /**< This window's colormap. */
   ColormapNode *colormaps;   /**< Colormaps assigned to this window. */

   char *name;                /**< Name of this window for display. */
   char *instanceName;        /**< Name of this window for properties. */
   char *className;           /**< Name of the window class. */

   ClientState state;         /**< Window state. */

   BorderActionType borderAction;

   struct IconNode *icon;     /**< Icon assigned to this window. */

   /** Callback to stop move/resize. */
   void (*controller)(int wasDestroyed);

   struct ClientNode *prev;   /**< The previous client in this layer. */
   struct ClientNode *next;   /**< The next client in this layer. */

} ClientNode;

/** Find a client by window or parent window.
 * @param w The window.
 * @return The client (NULL if not found).
 */
ClientNode *FindClientByWindow(Window w);

/** Find a client by its parent window.
 * @param p The parent window.
 * @return The client (NULL if not found).
 */
ClientNode *FindClientByParent(Window p);

/** Get the active client.
 * @return The active client (NULL if no client is active).
 */
ClientNode *GetActiveClient();

void InitializeClients();
void StartupClients();
void ShutdownClients();
void DestroyClients();

/** Add a window to management.
 * @param w The client window.
 * @param alreadyMapped 1 if the window is mapped, 0 if not.
 * @param notOwner 1 if JWM doesn't own this window, 0 if JWM is the owner.
 * @return The client window data.
 */
ClientNode *AddClientWindow(Window w, int alreadyMapped, int notOwner);

/** Remove a client from management.
 * @param np The client to remove.
 */
void RemoveClient(ClientNode *np);

/** Minimize a client.
 * @param np The client to minimize.
 */
void MinimizeClient(ClientNode *np);

/** Shade a client.
 * @param np The client to shade.
 */
void ShadeClient(ClientNode *np);

/** Unshade a client.
 * @param np The client to unshade.
 */
void UnshadeClient(ClientNode *np);

/** Set a client's status to withdrawn.
 * A withdrawn client is a client that is not visible in any way to the
 * user. This may be a window that an application keeps around so that
 * it can be reused at a later time.
 * @param np The client whose status to change.
 */
void SetClientWithdrawn(ClientNode *np);

/** Restore a client from minimized state.
 * @param np The client to restore.
 * @param raise 1 to raise the client, 0 to leave stacking unchanged.
 */
void RestoreClient(ClientNode *np, int raise);

/** Maximize a client.
 * @param np The client to maximize.
 * @param horiz Set to maximize the client horizontally.
 * @param vert Set to maximize the client vertically.
 */
void MaximizeClient(ClientNode *np, int horiz, int vert);

/** Maximize a client using the default maximize settings.
 * @param np The client to maximize.
 */
void MaximizeClientDefault(ClientNode *np);

/** Set the full screen status of a client.
 * @param np The client.
 * @param fullScreen 1 to make full screen, 0 to make not full screen.
 */
void SetClientFullScreen(ClientNode *np, int fullScreen);

/** Set the keyboard focus to a client.
 * @param np The client to focus.
 */
void FocusClient(ClientNode *np);

/** Set the keyboard focus back to the active client. */
void RefocusClient();

/** Tell a client to exit.
 * @param np The client to delete.
 */
void DeleteClient(ClientNode *np);

/** Force a client to exit.
 * @param np The client to kill.
 */
void KillClient(ClientNode *np);

/** Raise a client to the top of its layer.
 * @param np The client to raise.
 */
void RaiseClient(ClientNode *np);

/** Lower a client to the bottom of its layer.
 * @param np The client to lower.
 */
void LowerClient(ClientNode *np);

/** Restack the clients.
 * This is used when a client is mapped so that the stacking order
 * remains consistent.
 */
void RestackClients();

/** Set the layer of a client.
 * @param np The client whose layer to set.
 * @param layer the layer to assign to the client.
 */
void SetClientLayer(ClientNode *np, unsigned int layer);

/** Set the desktop for a client.
 * @param np The client.
 * @param desktop The desktop to be assigned to the client.
 */
void SetClientDesktop(ClientNode *np, unsigned int desktop);

/** Set the sticky status of a client.
 * A sticky client will appear on all desktops.
 * @param np The client.
 * @param isSticky 1 to make the client sticky, 0 to make it not sticky.
 */
void SetClientSticky(ClientNode *np, int isSticky);

/** Hide a client.
 * This is used for changing desktops.
 * @param np The client to hide.
 */
void HideClient(ClientNode *np);

/** Show a client.
 * This is used for changing desktops.
 * @param np The client to show.
 */
void ShowClient(ClientNode *np);

/** Update a client's colormap.
 * @param np The client.
 */
void UpdateClientColormap(ClientNode *np);

/** Update the shape of a client using the shape extension.
 * @param np The client to update.
 */
void SetShape(ClientNode *np);

/** Send a configure event to a client.
 * This will send updated location and size information to a client.
 * @param np The client to get the event.
 */
void SendConfigureEvent(ClientNode *np);

/** Send a message to a client.
 * @param w The client window.
 * @param type The type of message to send.
 * @param message The message to send.
 */
void SendClientMessage(Window w, AtomType type, AtomType message);

/** Set the opacity for active clients.
 * @param str The opacity to use.
 */
void SetActiveClientOpacity(const char *str);

/** Set the opacity range for inactive clients.
 * @param str The opacity range to use.
 */
void SetInactiveClientOpacity(const char *str);

#endif /* CLIENT_H */

