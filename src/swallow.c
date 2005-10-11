/****************************************************************************
 * Functions to handle swallowing client windows in the tray.
 ****************************************************************************/

#include "jwm.h"

#define FIND_RETRY_COUNT 10
#define FIND_USLEEP 500

typedef struct SwallowNode {

	char *name;
	char *command;
	Window window;
	int width;

	struct SwallowNode *prev;
	struct SwallowNode *next;

} SwallowNode;

static SwallowNode *leftHead;
static SwallowNode *leftTail;
static Window leftWindow;

static SwallowNode *rightHead;
static SwallowNode *rightTail;
static Window rightWindow;

static void DestroySwallowList(SwallowNode *np);
static void AppendSwallowList(SwallowNode **head, SwallowNode **tail,
		const char *name, const char *command);
static int StartSwallowedClient(SwallowNode *np);
static Window FindSwallowedClient(const char *name);

/****************************************************************************
 ****************************************************************************/
void InitializeSwallow()
{

	leftHead = NULL;
	leftTail = NULL;
	leftWindow = None;

	rightHead = NULL;
	rightTail = NULL;
	rightWindow = None;

}

/****************************************************************************
 ****************************************************************************/
void StartupSwallow()
{

	SwallowNode *np;
	unsigned int width;

	width = 0;
	for(np = leftHead; np; np = np->next) {
		width += StartSwallowedClient(np);
	}
/*TODO*/

	width = 0;
	for(np = rightHead; np; np = np->next) {
		width += StartSwallowedClient(np);
	}

	if(width > 0) {
	}

}

/****************************************************************************
 ****************************************************************************/
void ShutdownSwallow()
{
}

/****************************************************************************
 ****************************************************************************/
void DestroySwallow()
{

	DestroySwallowList(leftHead);
	leftHead = NULL;

	DestroySwallowList(rightHead);
	rightHead = NULL;

}

/****************************************************************************
 ****************************************************************************/
void DestroySwallowList(SwallowNode *np)
{

	SwallowNode *tp;

	while(np) {
		tp = np->next;
		Release(np->name);
		if(np->command) {
			Release(np->command);
		}
		Release(np);
		np = tp;
	}

}

/****************************************************************************
 ****************************************************************************/
void AppendSwallowList(SwallowNode **head, SwallowNode **tail,
		const char *name, const char *command)
{

	SwallowNode *node;

	Assert(name);

	node = Allocate(sizeof(SwallowNode));

	node->window = None;
	node->width = 0;

	node->name = Allocate(strlen(name) + 1);
	strcpy(node->name, name);

	if(command) {
		node->command = Allocate(strlen(command) + 1);
		strcpy(node->command, command);
	} else {
		node->command = NULL;
	}

	node->prev = *tail;
	node->next = NULL;

	if(*tail) {
		(*tail)->next = node;
	} else {
		*head = node;
	}
	*tail = node;

}

/****************************************************************************
 ****************************************************************************/
void Swallow(const char *name, const char *command,
		SwallowLocationType location)
{

	if(name == NULL) {
		Warning("cannot swallow a client with no name");
		return;
	}

	if(location == SWALLOW_LEFT) {
		AppendSwallowList(&leftHead, &leftTail, name, command);
	} else {
		AppendSwallowList(&rightHead, &rightTail, name, command);
	}

}

/****************************************************************************
 ****************************************************************************/
int StartSwallowedClient(SwallowNode *np)
{

	XWindowAttributes attributes;
	int x;

	Assert(np);

	np->width = 0;
	np->window = FindSwallowedClient(np->name);
	if(np->window == None) {

		if(!np->command) {
			Warning("client to be swallowed (%s) not found and no command given",
					np->name);
			return 0;
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
			return 0;
		}
	}

	if(!JXGetWindowAttributes(display, np->window, &attributes)) {
		attributes.width = 0;
	}
	np->width = attributes.width;

	return np->width;

}

/****************************************************************************
 ****************************************************************************/
Window FindSwallowedClient(const char *name)
{

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
				break;
			}
			JXFree(hint.res_name);
			JXFree(hint.res_class);
		}
	}

	JXFree(childrenReturn);

	return result;

}


