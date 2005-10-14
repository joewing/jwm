/****************************************************************************
 * Functions to handle swallowing client windows in the tray.
 ****************************************************************************/

#include "jwm.h"

/* Spend 5 seconds looking. */
#define FIND_RETRY_COUNT 10
#define FIND_USLEEP ((5 * 1000 * 1000) / FIND_RETRY_COUNT)

typedef struct SwallowNode {

	TrayComponentType *cp;

	char *name;
	char *command;
	int started;
	int border;

	struct SwallowNode *next;

} SwallowNode;

static SwallowNode *swallowNodes;

static void StartSwallowedClient(TrayComponentType *cp);
static Window FindSwallowedClient(const char *name);

static void Create(TrayComponentType *cp);
static void Destroy(TrayComponentType *cp);

/****************************************************************************
 ****************************************************************************/
void InitializeSwallow() {
	swallowNodes = NULL;
}

/****************************************************************************
 ****************************************************************************/
void StartupSwallow() {

	SwallowNode *np;

	for(np = swallowNodes; np; np = np->next) {
		StartSwallowedClient(np->cp);
	}

}

/****************************************************************************
 ****************************************************************************/
void ShutdownSwallow() {

	SwallowNode *np;

	while(swallowNodes) {

		np = swallowNodes->next;

		Assert(swallowNodes->name);
		Release(swallowNodes->name);

		if(swallowNodes->command) {
			Release(swallowNodes->command);
		}

		Release(swallowNodes);
		swallowNodes = np;

	}

}

/****************************************************************************
 ****************************************************************************/
void DestroySwallow() {
}

/****************************************************************************
 ****************************************************************************/
TrayComponentType *CreateSwallow(const char *name, const char *command,
	int width, int height) {

	TrayComponentType *cp;
	SwallowNode *np;

	if(!name) {
		Warning("cannot swallow a client with no name");
		return NULL;
	}

	np = Allocate(sizeof(SwallowNode));
	np->name = Allocate(strlen(name) + 1);
	strcpy(np->name, name);
	if(command) {
		np->command = Allocate(strlen(command) + 1);
		strcpy(np->command, command);
	} else {
		np->command = NULL;
	}
	np->started = 0;

	np->next = swallowNodes;
	swallowNodes = np;

	cp = CreateTrayComponent();
	np->cp = cp;
	cp->object = np;
	cp->Create = Create;
	cp->Destroy = Destroy;

	cp->width = width;
	cp->height = height;

	return cp;

}

/****************************************************************************
 ****************************************************************************/
int ProcessSwallowEvent(const XEvent *event) {

	Window w;
	SwallowNode *np;

	if(event->type == DestroyNotify) {
		w = event->xdestroywindow.window;
		for(np = swallowNodes; np; np = np->next) {
			if(np->cp->window == w) {
				np->cp->window = None;
				return 1;
			}
		}
	}

	return 0;

}

/****************************************************************************
 ****************************************************************************/
void Create(TrayComponentType *cp) {

	int width, height;

	SwallowNode *np = (SwallowNode*)cp->object;

	if(cp->window != None) {
		width = cp->width - np->border * 2;
		height = cp->height - np->border * 2;
		JXResizeWindow(display, cp->window, width, height);
	}

}

/****************************************************************************
 ****************************************************************************/
void Destroy(TrayComponentType *cp) {

	if(cp->window) {
		JXReparentWindow(display, cp->window, rootWindow, 0, 0);
		JXRemoveFromSaveSet(display, cp->window);
	}

}

/****************************************************************************
 ****************************************************************************/
void StartSwallowedClient(TrayComponentType *cp) {

	XWindowAttributes attributes;
	SwallowNode *np;
	int x;

	Assert(cp);

	np = (SwallowNode*)cp->object;

	Assert(np);

	if(np->started) {
		return;
	}

	Debug("starting %s...", np->name);

	cp->width = 0;
	cp->height = 0;

	np->border = 0;
	np->started = 1;

	cp->window = FindSwallowedClient(np->name);
	if(cp->window == None) {

		if(!np->command) {
			Warning("client to be swallowed (%s) not found and no command given",
					np->name);
			return;
		}

		RunCommand(np->command);

		for(x = 0; x < FIND_RETRY_COUNT; x++) {
			cp->window = FindSwallowedClient(np->name);
			if(cp->window != None) {
				break;
			}
			usleep(FIND_USLEEP);
		}

		if(cp->window == None) {
			Warning("%s not found after running %s", np->name, np->command);
			return;
		}
	}

	if(!JXGetWindowAttributes(display, cp->window, &attributes)) {
		attributes.width = 0;
		attributes.height = 0;
		attributes.border_width = 0;
	}
	np->border = attributes.border_width;

	if(cp->width < 0 || cp->width > rootWidth) {
		Warning("invalid width for swallow: %d", cp->width);
		cp->width = 0;
	}
	if(cp->height < 0 || cp->height > rootHeight) {
		Warning("invalid height for swallow: %d", cp->height);
		cp->height = 0;
	}

	if(cp->width == 0) {
		cp->width = attributes.width + 2 * np->border;
	}
	if(cp->height == 0) {
		cp->height = attributes.height + 2 * np->border;
	}

	Debug("%s started", np->name);

}

/****************************************************************************
 ****************************************************************************/
Window FindSwallowedClient(const char *name) {

	XClassHint hint;
	Window rootReturn, parentReturn, *childrenReturn;
	Window result;
	unsigned int childrenCount;
	unsigned int x;

	Assert(name);

	result = None;

	JXQueryTree(display, rootWindow, &rootReturn, &parentReturn,
			&childrenReturn, &childrenCount);

	for(x = 0; x < childrenCount; x++) {
		if(JXGetClassHint(display, childrenReturn[x], &hint)) {
			if(!strcmp(hint.res_name, name)) {
				result = childrenReturn[x];
				JXAddToSaveSet(display, result);
				JXSetWindowBorder(display, result, colors[COLOR_TRAY_BG]);
				JXMapRaised(display, result);
				JXSelectInput(display, result, StructureNotifyMask);
				break;
			}
			JXFree(hint.res_name);
			JXFree(hint.res_class);
		}
	}

	JXFree(childrenReturn);

	return result;

}


