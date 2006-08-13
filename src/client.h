/**
 * @file client.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
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
	BORDER_OUTLINE = 1,  /**< Window has a border. */
	BORDER_TITLE   = 2,  /**< Window has a title bar. */
	BORDER_MIN     = 4,  /**< Window supports minimize. */
	BORDER_MAX     = 8,  /**< Window supports maximize. */
	BORDER_CLOSE   = 16, /**< Window supports close. */
	BORDER_RESIZE  = 32, /**< Window supports resizing. */
	BORDER_MOVE    = 64  /**< Window supports moving. */
} BorderFlags;

/** The default border flags. */
#define BORDER_DEFAULT ( \
		  BORDER_OUTLINE \
		| BORDER_TITLE   \
		| BORDER_MIN     \
		| BORDER_MAX     \
		| BORDER_CLOSE   \
		| BORDER_RESIZE  \
		| BORDER_MOVE    )

/** Window status flags. */
typedef enum {
	STAT_NONE      = 0,
	STAT_ACTIVE    = 1 << 0,  /**< This client has focus. */
	STAT_MAPPED    = 1 << 1,  /**< This client is shown (on some desktop). */
	STAT_MAXIMIZED = 1 << 2,  /**< This client is maximized. */
	STAT_HIDDEN    = 1 << 3,  /**< This client is not on the current desktop. */
	STAT_STICKY    = 1 << 4,  /**< This client is on all desktops. */
	STAT_NOLIST    = 1 << 5,  /**< Skip this client in the task list. */
	STAT_MINIMIZED = 1 << 6,  /**< This client is minimized. */
	STAT_SHADED    = 1 << 7,  /**< This client is shaded. */
	STAT_WMDIALOG  = 1 << 8,  /**< This is a JWM dialog window. */
	STAT_PIGNORE   = 1 << 9,  /**< Ignore the program-specified position. */
	STAT_SHAPE     = 1 << 10, /**< This client uses the shape extension. */
	STAT_SDESKTOP  = 1 << 11  /**< This client was minimized to show desktop. */
} StatusFlags;

/** Colormap window linked list. */
typedef struct ColormapNode {
	Window window;             /**< A window containing a colormap. */
	struct ColormapNode *next; /**< Next value in the linked list. */
} ColormapNode;

/** The aspect ratio of a window. */
typedef struct AspectRatio {
	int minx, miny;  /**< The minimum aspect ratio. */
	int maxx, maxy;  /**< The maximum aspect ratio. */
} AspectRatio;

/** Struture to store information about a client window. */
typedef struct ClientNode {

	Window window;           /**< The client window. */
	Window parent;           /**< The frame window. */

	Window owner;            /**< The owner window (for transients). */

	int x, y;                /**< The location of the window. */
	int width, height;       /**< The size of the window. */
	int oldx, oldy;          /**< The old location (for maximize). */
	int oldWidth, oldHeight; /**< The old size (for maximize). */

	long sizeFlags;
	int baseWidth, baseHeight;
	int minWidth, minHeight;
	int maxWidth, maxHeight;
	int xinc, yinc;
	AspectRatio aspect;
	int gravity;

	Colormap cmap;
	ColormapNode *colormaps;

	char *name;
	char *instanceName;
	char *className;

	ClientState state;

	BorderActionType borderAction;

	struct IconNode *icon;

	void (*controller)(int wasDestroyed);

	struct ClientNode *prev;  /**< The previous client in this layer. */
	struct ClientNode *next;  /**< The next client in this layer. */

} ClientNode;

/** Client windows in linked lists for each layer. */
extern ClientNode *nodes[LAYER_COUNT];

/** Client windows in linked lists for each layer (pointer to the tail). */
extern ClientNode *nodeTail[LAYER_COUNT];

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
 */
void MaximizeClient(ClientNode *np);

/** Set the keyboard focus to a client.
 * @param np The client to focus.
 */
void FocusClient(ClientNode *np);

/** Set the keyboard focus to the next client.
 * This is used to focus the next client in the stacking order.
 * @param np The client before the client to focus.
 */
void FocusNextStacked(ClientNode *np);

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
 * @parma desktop The desktop to be assigned to the client.
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

#endif

