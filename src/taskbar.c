/***************************************************************************
 * TODO: Support vertical taskbars.
 ***************************************************************************/

#include "jwm.h"

typedef enum {
	INSERT_LEFT,
	INSERT_RIGHT
} InsertModeType;

typedef struct TaskBarType {

	TrayComponentType *cp;

	int itemHeight;
	LayoutType layout;

	Pixmap buffer;
	GC bufferGC;

	struct TaskBarType *next;

} TaskBarType;

typedef struct Node {
	ClientNode *client;
	struct Node *next;
} Node;

static char minimized_bitmap[] = {
	0x06, 0x0F,
	0x0F, 0x06
};

static const int TASK_SPACER = 2;

static Pixmap minimizedPixmap;
static unsigned int maxItemWidth;
static InsertModeType insertMode;

static TaskBarType *bars;
static Node *taskBarNodes;
static Node *taskBarNodesTail;

static Node *GetNode(TaskBarType *bar, int x);
static unsigned int GetItemCount();
static int ShouldShowItem(const ClientNode *np);
static int ShouldFocusItem(const ClientNode *np);
static unsigned int GetItemWidth(unsigned int width, unsigned int itemCount);
static void Render(const TaskBarType *bp);
static void ShowTaskWindowMenu(TaskBarType *bar, Node *np);

static void Create(TrayComponentType *cp);
static void Destroy(TrayComponentType *cp);
static void ProcessTaskButtonEvent(TrayComponentType *cp,
	int x, int y, int mask);

/***************************************************************************
 ***************************************************************************/
void InitializeTaskBar() {
	bars = NULL;
	taskBarNodes = NULL;
	taskBarNodesTail = NULL;
	maxItemWidth = 0;
	insertMode = INSERT_RIGHT;
}

/***************************************************************************
 ***************************************************************************/
void StartupTaskBar() {
	minimizedPixmap = JXCreatePixmapFromBitmapData(display, rootWindow,
		minimized_bitmap, 4, 4, colors[COLOR_TASK_FG],
		colors[COLOR_TASK_BG], rootDepth);
}

/***************************************************************************
 ***************************************************************************/
void ShutdownTaskBar() {

	TaskBarType *bp;

	while(bars) {
		bp = bars->next;

		JXFreeGC(display, bars->bufferGC);
		JXFreePixmap(display, bars->buffer);

		Release(bars);
		bars = bp;
	}

	JXFreePixmap(display, minimizedPixmap);
}

/***************************************************************************
 ***************************************************************************/
void DestroyTaskBar() {
}

/***************************************************************************
 ***************************************************************************/
TrayComponentType *CreateTaskBar() {

	TrayComponentType *cp;
	TaskBarType *tp;

	tp = Allocate(sizeof(TaskBarType));
	tp->next = bars;
	bars = tp;
	tp->itemHeight = 0;
	tp->layout = LAYOUT_HORIZONTAL;

	cp = CreateTrayComponent();
	cp->object = tp;
	tp->cp = cp;

	cp->width = 0;
	cp->height = 0;

	cp->Create = Create;
	cp->Destroy = Destroy;
	cp->ProcessButtonEvent = ProcessTaskButtonEvent;

	return cp;

}

/***************************************************************************
 ***************************************************************************/
void Create(TrayComponentType *cp) {

	TaskBarType *tp;

	Assert(cp);

	tp = (TaskBarType*)cp->object;

	Assert(tp);

	if(cp->width > cp->height) {
		tp->layout = LAYOUT_HORIZONTAL;
		tp->itemHeight = cp->height;
	} else {
		tp->layout = LAYOUT_VERTICAL;
		tp->itemHeight = GetStringHeight(FONT_TASK) + 12;
	}

	Assert(cp->width > 0);
	Assert(cp->height > 0);

	cp->pixmap = JXCreatePixmap(display, rootWindow, cp->width, cp->height,
		rootDepth);
	tp->bufferGC = JXCreateGC(display, cp->pixmap, 0, NULL);
	tp->buffer = cp->pixmap;

	JXSetForeground(display, tp->bufferGC, colors[COLOR_TRAY_BG]);
	JXFillRectangle(display, cp->pixmap, tp->bufferGC,
		0, 0, cp->width, cp->height);

}

/***************************************************************************
 ***************************************************************************/
void Destroy(TrayComponentType *cp) {

}

/***************************************************************************
 ***************************************************************************/
void ProcessTaskButtonEvent(TrayComponentType *cp, int x, int y, int mask) {

	TaskBarType *bar = (TaskBarType*)cp->object;
	Node *np;

	Assert(bar);

	if(bar->layout == LAYOUT_HORIZONTAL) {
		np = GetNode(bar, x);
	} else {
		np = GetNode(bar, y);
	}

	if(np) {
		switch(mask) {
		case Button1:
			if(np->client->statusFlags & STAT_ACTIVE
				&& np->client == nodes[np->client->layer]) {
				MinimizeClient(np->client);
			} else {
				RestoreClient(np->client);
				FocusClient(np->client);
			}
			break;
		case Button3:
			ShowTaskWindowMenu(bar, np);
			break;
		default:
			break;
		}
	}

}

/***************************************************************************
 ***************************************************************************/
void ShowTaskWindowMenu(TaskBarType *bar, Node *np) {
	int x, y;
	GetMousePosition(&x, &y);
	ShowWindowMenu(np->client, x, y);
}

/***************************************************************************
 ***************************************************************************/
void AddClientToTaskBar(ClientNode *np) {

	Node *tp;

	Assert(np);

	tp = Allocate(sizeof(Node));
	tp->client = np;

	if(insertMode == INSERT_RIGHT) {
		tp->next = NULL;
		if(taskBarNodesTail) {
			taskBarNodesTail->next = tp;
		} else {
			taskBarNodes = tp;
		}
		taskBarNodesTail = tp;
	} else {
		tp->next = taskBarNodes;
		taskBarNodes = tp;
		if(!taskBarNodesTail) {
			taskBarNodesTail = tp;
		}
	}

	UpdateTaskBar();

}

/***************************************************************************
 ***************************************************************************/
void RemoveClientFromTaskBar(ClientNode *np) {

	Node *tp;
	Node *lp;

	Assert(np);

	lp = NULL;
	for(tp = taskBarNodes; tp; tp = tp->next) {
		if(tp->client == np) {
			if(tp == taskBarNodesTail) {
				taskBarNodesTail = lp;
			}
			if(lp) {
				lp->next = tp->next;
			} else {
				taskBarNodes = tp->next;
			}
			Release(tp);
			break;
		}
		lp = tp;
	}

	UpdateTaskBar();

}

/***************************************************************************
 ***************************************************************************/
void UpdateTaskBar() {

	TaskBarType *bp;

	for(bp = bars; bp; bp = bp->next) {
		Render(bp);
	}

}

/***************************************************************************
 ***************************************************************************/
void Render(const TaskBarType *bp) {

	Node *tp;
	ButtonType buttonType;
	int x, y;
	int remainder;
	int itemWidth, itemCount;
	int width, height;
	int iconSize;
	Pixmap buffer;
	GC gc;

	if(shouldExit) {
		return;
	}

	Assert(bp);
	Assert(bp->cp);

	width = bp->cp->width;
	height = bp->cp->height;
	buffer = bp->cp->pixmap;
	gc = bp->bufferGC;

	Assert(buffer != None);
	Assert(gc != None);

	x = TASK_SPACER;
	width -= x;
	y = TASK_SPACER;
	height -= y;

	JXSetForeground(display, gc, colors[COLOR_TRAY_BG]);
	JXFillRectangle(display, buffer, gc, 0, 0, width, height);

	itemCount = GetItemCount();
	if(!itemCount) {
		UpdateSpecificTray(bp->cp->tray, bp->cp);
		return;
	}
	if(bp->layout == LAYOUT_HORIZONTAL) {
		itemWidth = GetItemWidth(width, itemCount);
		remainder = width - itemWidth * itemCount;
	} else {
		itemWidth = width;
		remainder = 0;
	}

	iconSize = bp->itemHeight - 2 * TASK_SPACER - 6;

	SetButtonDrawable(buffer, gc);
	SetButtonFont(FONT_TASK);
	SetButtonAlignment(ALIGN_LEFT);
	SetButtonTextOffset(iconSize + 3);

	for(tp = taskBarNodes; tp; tp = tp->next) {
		if(ShouldShowItem(tp->client)) {

			if(tp->client->statusFlags & STAT_ACTIVE) {
				buttonType = BUTTON_TASK_ACTIVE;
			} else {
				buttonType = BUTTON_TASK;
			}

			if(remainder) {
				SetButtonSize(itemWidth - TASK_SPACER,
					bp->itemHeight - 2 * TASK_SPACER + 1);
			} else {
				SetButtonSize(itemWidth - TASK_SPACER - 1,
					bp->itemHeight - 2 * TASK_SPACER + 1);
			}

			DrawButton(x, y, buttonType, tp->client->name);

			if(tp->client->icon) {
				PutIcon(tp->client->icon, buffer, gc,
					x + TASK_SPACER + 2, y + TASK_SPACER + 2, iconSize);
			}

			if(tp->client->statusFlags & STAT_MINIMIZED) {
				JXCopyArea(display, minimizedPixmap, buffer, gc,
					0, 0, 4, 4, x + 3, y + bp->itemHeight - 8);
			}

			if(bp->layout == LAYOUT_HORIZONTAL) {
				x += itemWidth;
				if(remainder) {
					++x;
					--remainder;
				}
			} else {
				y += bp->itemHeight;
				if(remainder) {
					++y;
					--remainder;
				}
			}

		}
	}

	UpdateSpecificTray(bp->cp->tray, bp->cp);

}

/***************************************************************************
 ***************************************************************************/
void FocusNext() {

	Node *tp;

	for(tp = taskBarNodes; tp; tp = tp->next) {
		if(ShouldFocusItem(tp->client)) {
			if(tp->client->statusFlags & STAT_ACTIVE) {
				tp = tp->next;
				break;
			}
		}
	}

	if(!tp) {
		tp = taskBarNodes;
	}

	while(tp && !ShouldFocusItem(tp->client)) {
		tp = tp->next;
	}

	if(!tp) {
		tp = taskBarNodes;
		while(tp && !ShouldFocusItem(tp->client)) {
			tp = tp->next;
		}
	}

	if(tp) {
		RestoreClient(tp->client);
		FocusClient(tp->client);
	}

}

/***************************************************************************
 ***************************************************************************/
Node *GetNode(TaskBarType *bar, int x)
{

	Node *tp;
	int remainder;
	int itemCount;
	int itemWidth;
	int index, stop;
	int width;

	index = TASK_SPACER;

	itemCount = GetItemCount();

	if(bar->layout == LAYOUT_HORIZONTAL) {

		width = bar->cp->width - index; 
		itemWidth = GetItemWidth(width, itemCount);
		remainder = width - itemWidth * itemCount;

		for(tp = taskBarNodes; tp; tp = tp->next) {
			if(ShouldShowItem(tp->client)) {
				if(remainder) {
					stop = index + itemWidth + 1;
					--remainder;
				} else {
					stop = index + itemWidth;
				}
				if(x >= index && x < stop) {
					return tp;
				}
				index = stop;
			}
		}

	} else {

		for(tp = taskBarNodes; tp; tp = tp->next) {
			if(ShouldShowItem(tp->client)) {
				stop = index + bar->itemHeight;
				if(x >= index && x < stop) {
					return tp;
				}
				index = stop;
			}
		}

	}

	return NULL;

}

/***************************************************************************
 ***************************************************************************/
unsigned int GetItemCount() {

	Node *tp;
	unsigned int count;

	count = 0;
	for(tp = taskBarNodes; tp; tp = tp->next) {
		if(ShouldShowItem(tp->client)) {
			++count;
		}
	}

	return count;

}

/***************************************************************************
 ***************************************************************************/
int ShouldShowItem(const ClientNode *np) {

	if(np->desktop != currentDesktop && !(np->statusFlags & STAT_STICKY)) {
		return 0;
	}

	if(np->statusFlags & (STAT_NOLIST | STAT_WITHDRAWN)) {
		return 0;
	}

	if(np->owner != None) {
		return 0;
	}

	if(!(np->statusFlags & STAT_MAPPED)
		&& !(np->statusFlags & (STAT_MINIMIZED | STAT_SHADED))) {
		return 0;
	}

	return 1;

}

/***************************************************************************
 ***************************************************************************/
int ShouldFocusItem(const ClientNode *np) {

	if(np->desktop != currentDesktop && !(np->statusFlags & STAT_STICKY)) {
		return 0;
	}

	if(np->statusFlags & (STAT_NOLIST | STAT_WITHDRAWN)) {
		return 0;
	}

	if(!(np->statusFlags & STAT_MAPPED)) {
		return 0;
	}

	if(np->owner != None) {
		return 0;
	}

	return 1;

}

/***************************************************************************
 ***************************************************************************/
unsigned int GetItemWidth(unsigned int width, unsigned int itemCount) {

	unsigned int itemWidth;

	itemWidth = width;

	if(!itemCount) {
		return itemWidth;
	}

	itemWidth /= itemCount;
	if(!itemWidth) {
		itemWidth = 1;
	}

	if(maxItemWidth > 0 && itemWidth > maxItemWidth) {
		itemWidth = maxItemWidth;
	}

	return itemWidth;

}

/***************************************************************************
 ***************************************************************************/
void SetMaxTaskBarItemWidth(unsigned int w) {
	maxItemWidth = w;
}

/***************************************************************************
 ***************************************************************************/
void SetTaskBarInsertMode(const char *mode) {

	Assert(mode);

	if(!strcmp(mode, "right")) {
		insertMode = INSERT_RIGHT;
	} else if(!strcmp(mode, "left")) {
		insertMode = INSERT_LEFT;
	} else {
		Warning("invalid insert mode: \"%s\"", mode);
		insertMode = INSERT_RIGHT;
	}

}

