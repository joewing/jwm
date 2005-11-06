/****************************************************************************
 * Header for client window functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef CLIENT_H
#define CLIENT_H

#include "border.h"
#include "hint.h"

typedef enum {
	PROT_NONE       = 0,
	PROT_DELETE     = 1,
	PROT_TAKE_FOCUS = 2
} ClientProtocolType;

typedef enum {
	BORDER_NONE    = 0,
	BORDER_OUTLINE = 1,
	BORDER_TITLE   = 2,
	BORDER_MIN     = 4,
	BORDER_MAX     = 8,
	BORDER_CLOSE   = 16,
	BORDER_RESIZE  = 32,
	BORDER_MOVE    = 64
} BorderFlags;

#define BORDER_DEFAULT ( \
		  BORDER_OUTLINE \
		| BORDER_TITLE   \
		| BORDER_MIN     \
		| BORDER_MAX     \
		| BORDER_CLOSE   \
		| BORDER_RESIZE  \
		| BORDER_MOVE    )

typedef enum {
	STAT_NONE      = 0,
	STAT_ACTIVE    = 1 << 0,
	STAT_MAPPED    = 1 << 1,
	STAT_MAXIMIZED = 1 << 2,
	STAT_HIDDEN    = 1 << 3,
	STAT_STICKY    = 1 << 4,
	STAT_NOLIST    = 1 << 5,
	STAT_WITHDRAWN = 1 << 6,
	STAT_MINIMIZED = 1 << 7,
	STAT_SHADED    = 1 << 8,
	STAT_USESHAPE  = 1 << 9,
	STAT_WMDIALOG  = 1 << 10
} StatusFlags;

typedef struct ColormapNode {
	Window window;
	struct ColormapNode *next;
} ColormapNode;

typedef struct AspectRatio {
	int minx, maxx;
	int miny, maxy;
} AspectRatio;

typedef struct ClientNode {

	Window window;
	Window parent;
	GC parentGC;

	Window owner;

	int x, y, width, height;
	int oldx, oldy, oldWidth, oldHeight;

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

	ClientProtocolType protocols;

	struct IconNode *icon;

	void (*controller)(int wasDestroyed);

	struct ClientNode *prev;
	struct ClientNode *next;

} ClientNode;

extern ClientNode *nodes[LAYER_COUNT];
extern ClientNode *nodeTail[LAYER_COUNT];

ClientNode *FindClientByWindow(Window w);
ClientNode *FindClientByParent(Window p);
ClientNode *GetActiveClient();

void InitializeClients();
void StartupClients();
void ShutdownClients();
void DestroyClients();

ClientNode *AddClientWindow(Window w, int alreadyMapped, int notOwner);
void RemoveClient(ClientNode *np);

void MinimizeClient(ClientNode *np);
void ShadeClient(ClientNode *np);
void UnshadeClient(ClientNode *np);
void SetClientWithdrawn(ClientNode *np, int isWithdrawn);
void WithdrawClient(ClientNode *np);
void RestoreClient(ClientNode *np);
void MaximizeClient(ClientNode *np);
void FocusClient(ClientNode *np);
void FocusNextStacked(ClientNode *np);
void RefocusClient();
void DeleteClient(ClientNode *np);
void KillClient(ClientNode *np);
void RaiseClient(ClientNode *np);
void RestackClients();
void SetClientLayer(ClientNode *np, int layer);
void SetClientDesktop(ClientNode *np, int desktop);
void SetClientSticky(ClientNode *np, int isSticky);

void HideClient(ClientNode *np);
void ShowClient(ClientNode *np);

void UpdateClientColormap(ClientNode *np);

void SetShape(ClientNode *np);

void SendConfigureEvent(ClientNode *np);

#endif

