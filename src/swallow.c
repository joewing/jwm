/****************************************************************************
 * Functions to handle swallowing client windows in the tray.
 ****************************************************************************/

#include "jwm.h"

/* Spend 5 seconds looking. */
#define FIND_RETRY_COUNT 10
#define FIND_USLEEP ((5 * 1000 * 1000) / FIND_RETRY_COUNT)

typedef struct SwallowNode {

	char *name;
	char *command;
	int started;

	Window window;
	int width, height;
	int border;

} SwallowNode;

static void StartSwallowedClient(SwallowNode *np);
static Window FindSwallowedClient(const char *name);

static void Create(void *object, void *owner, void (*Update)(void *owner),
	int width, int height);
static void Destroy(void *object);
static int GetWidth(void *object);
static int GetHeight(void *object);
static Window GetWindow(void *object);

/****************************************************************************
 ****************************************************************************/
void InitializeSwallow() {
}

/****************************************************************************
 ****************************************************************************/
void StartupSwallow() {
}

/****************************************************************************
 ****************************************************************************/
void ShutdownSwallow() {
}

/****************************************************************************
 ****************************************************************************/
void DestroySwallow() {
}

/****************************************************************************
 ****************************************************************************/
TrayComponentType *CreateSwallow(const char *name, const char *command) {

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

	np->window = None;
	np->width = 0;
	np->height = 0;
	np->started = 0;

	cp = Allocate(sizeof(TrayComponentType));

	cp->object = np;

	cp->Create = Create;
	cp->Destroy = Destroy;
	cp->GetWidth = GetWidth;
	cp->GetHeight = GetHeight;
	cp->SetSize = NULL;
	cp->GetWindow = GetWindow;
	cp->GetPixmap = NULL;

	cp->ProcessButtonEvent = NULL;

	cp->next = NULL;

	return cp;

}

/****************************************************************************
 ****************************************************************************/
void Create(void *object, void *owner, void (*Update)(void *owner),
	int width, int height) {

	SwallowNode *np = (SwallowNode*)object;

	Assert(np);

	StartSwallowedClient(np);

	np->width = width - np->border * 2;
	np->height = height - np->border * 2;

	if(np->window != None) {
		JXResizeWindow(display, np->window, np->width, np->height);
	}

}

/****************************************************************************
 ****************************************************************************/
void Destroy(void *object) {

	SwallowNode *np = (SwallowNode*)object;

	Assert(np);

	Assert(np->name);
	Release(np->name);

	if(np->command) {
		Release(np->command);
	}

	if(np->window != None) {
		JXReparentWindow(display, np->window, rootWindow, 0, 0);
		JXRemoveFromSaveSet(display, np->window);
	}

	Release(np);

}

/****************************************************************************
 ****************************************************************************/
int GetWidth(void *object) {

	SwallowNode *np = (SwallowNode*)object;

	Assert(np);

	if(!np->started) {
		StartSwallowedClient(np);
	}

	return np->width + 2 * np->border;

}

/****************************************************************************
 ****************************************************************************/
int GetHeight(void *object) {

	SwallowNode *np = (SwallowNode*)object;

	Assert(np);

	if(!np->started) {
		StartSwallowedClient(np);
	}

	return np->height + 2 * np->border;

}

/****************************************************************************
 ****************************************************************************/
Window GetWindow(void *object) {

	SwallowNode *np = (SwallowNode*)object;

	Assert(np);

	return np->window;

}

/****************************************************************************
 ****************************************************************************/
void StartSwallowedClient(SwallowNode *np) {

	XWindowAttributes attributes;
	int x;

	Assert(np);

	if(np->started) {
		return;
	}

	np->width = 0;
	np->height = 0;
	np->border = 0;
	np->started = 1;
	np->window = FindSwallowedClient(np->name);
	if(np->window == None) {

		if(!np->command) {
			Warning("client to be swallowed (%s) not found and no command given",
					np->name);
			return;
		}

		RunCommand(np->command);

		for(x = 0; x < FIND_RETRY_COUNT; x++) {
			np->window = FindSwallowedClient(np->name);
			if(np->window != None) {
				break;
			}
			usleep(FIND_USLEEP);
		}

		if(np->window == None) {
			Warning("%s not found after running %s", np->name, np->command);
			return;
		}
	}

	if(!JXGetWindowAttributes(display, np->window, &attributes)) {
		attributes.width = 0;
		attributes.height = 0;
		attributes.border_width = 0;
	}
	np->width = attributes.width;
	np->height = attributes.height;
	np->border = attributes.border_width;

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
				break;
			}
			JXFree(hint.res_name);
			JXFree(hint.res_class);
		}
	}

	JXFree(childrenReturn);

	return result;

}


