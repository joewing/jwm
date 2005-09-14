/***************************************************************************
 * Functions to handle the tray.
 * Copyright (C) 2004 Joe Wingbermuehle
 ***************************************************************************/

#include "jwm.h"

typedef enum {
	TRAY_ALIGN_LEFT,
	TRAY_ALIGN_CENTER,
	TRAY_ALIGN_RIGHT
} TrayAlignmentType;

typedef enum {
	TRAY_INSERT_LEFT,
	TRAY_INSERT_RIGHT
} TrayInsertModeType;

typedef struct TrayNode {
	ClientNode *client;
	struct TrayNode *next;
} TrayNode;

#define TRAY_BEVEL 2

int trayHeight = DEFAULT_TRAY_HEIGHT - TRAY_BEVEL;
int trayWidth = 0;
int trayX = 0;
int trayY = 0;

static TrayAlignmentType trayAlignment = TRAY_ALIGN_CENTER;
static TrayInsertModeType trayInsertMode = TRAY_INSERT_LEFT;

static int trayStart;

int trayIsHidden = 0;

int autoHideTray = 0;
static int maxTrayItemWidth = 0;
static char *menuTitle = NULL;
static char *menuIconName = NULL;
static IconNode *menuIcon = NULL;

static char minimized_bitmap[] = {
	0x06, 0x0F,
	0x0F, 0x06
};

Window trayWindow;
static GC trayGC;
static Pixmap buffer; 
static GC bufferGC;
static Pixmap minimizedPixmap;
static unsigned int trayStop;
static unsigned int trayTextOffset;

static char *clockProgram = NULL;
static char *loadProgram = NULL;

static TimeType popupTime = ZERO_TIME;
static int popupx = 0;
static int popupy = 0;
static int overTray = 0;

#ifdef SHOW_LOAD
static TimeType loadTime = ZERO_TIME;
#endif

static TrayNode *trayNodes;
static TrayNode *trayNodesTail;

static void RenderTray();
static unsigned int GetItemCount();
static unsigned int GetItemWidth();
static int ShouldShowItemInTray(const ClientNode *np);
static int ShouldFocusItem(const ClientNode *np);
static TrayNode *GetTrayNode(int x);
static int IsOverMenuButton(const int x);
static int IsOverPager(const int x);
static int IsOverLoad(const int x);
static int IsOverClock(const int x);
static char *GetLoadString();

static int HandleTrayExpose(const XExposeEvent *event);
static int HandleTrayEnterNotify(const XCrossingEvent *event);
static int HandleTrayLeaveNotify(const XCrossingEvent *event);
static int HandleTrayButtonPress(const XButtonEvent *event);
static int HandleTrayMotionNotify(const XMotionEvent *event);

/***************************************************************************
 ***************************************************************************/
void InitializeTray() {
}

/***************************************************************************
 ***************************************************************************/
void StartupTray() {

	XSetWindowAttributes attr;
	int x;
	char *temp;

	if(trayWidth == 0) {
		trayWidth = rootWidth;
	}
	if(trayWidth < rootWidth) {
		switch(trayAlignment) {
		case TRAY_ALIGN_LEFT:
			trayX = 0;
			break;
		case TRAY_ALIGN_RIGHT:
			trayX = rootWidth - trayWidth;
			break;
		case TRAY_ALIGN_CENTER:
			trayX = rootWidth / 2 - trayWidth / 2;
			break;
		default:
			Debug("invalid tray alignment: %d", trayAlignment);
			Assert(0);
			break;
		}
	} else {
		trayX = 0;
	}
	trayY = rootHeight - trayHeight - TRAY_BEVEL;

	attr.override_redirect = True;
	attr.event_mask = ButtonPressMask | ExposureMask | KeyPressMask;
	attr.background_pixel = colors[COLOR_TRAY_BG];

	trayWindow = JXCreateWindow(display, rootWindow, trayX, trayY,
		trayWidth, trayHeight + TRAY_BEVEL, 0, rootDepth, InputOutput,
		rootVisual, CWOverrideRedirect | CWBackPixel | CWEventMask, &attr);
	JXMapRaised(display, trayWindow);

	JXSelectInput(display, trayWindow, EnterWindowMask
		| LeaveWindowMask | ExposureMask | ButtonPressMask
		| PointerMotionMask);

	SetDefaultCursor(trayWindow);

	trayGC = JXCreateGC(display, trayWindow, 0, NULL);

	buffer = JXCreatePixmap(display, rootWindow,
		trayWidth - 1, trayHeight, rootDepth);
	bufferGC = JXCreateGC(display, buffer, 0, 0);
	JXSetForeground(display, bufferGC, colors[COLOR_TRAY_BG]);
	JXFillRectangle(display, buffer, bufferGC, 0, 0,
		trayWidth - 1, trayHeight);


	if(!menuTitle) {
		menuTitle = Allocate(strlen(DEFAULT_MENU_TITLE) + 1);
		strcpy(menuTitle, DEFAULT_MENU_TITLE);
	}

	if(menuIconName) {
		menuIcon = LoadNamedIcon(menuIconName, 1);
	} else {
		menuIcon = NULL;
	}

	x = strlen(menuTitle);
	if(x) {
		trayStart = XTextWidth(fonts[FONT_TRAY], menuTitle, x);
		trayStart += 20;
		if(menuIcon) {
			trayStart += iconSize;
		}
	} else if(menuIcon) {
		trayStart = trayHeight;
	} else {
		trayStart = 0;
	}

	/* Compute the size of the clock. */
	temp = Allocate(MAX_CLOCK_LENGTH + 1 + 2);
	strcpy(temp, GetShortTimeString());
	strcat(temp, "MM");
	trayStop = JXTextWidth(fonts[FONT_TRAY], temp, strlen(temp));
	trayStop += loadWidth + 5;
	Release(temp);

	trayTextOffset = (trayHeight / 2)
		- (fonts[FONT_TRAY]->ascent + fonts[FONT_TRAY]->descent) / 2;

	trayNodes = NULL;
	trayNodesTail = NULL;
	trayIsHidden = 0;

	minimizedPixmap = JXCreatePixmapFromBitmapData(display, rootWindow,
		minimized_bitmap, 4, 4, colors[COLOR_TRAY_FG],
		colors[COLOR_TRAY_BG], rootDepth);

	DrawTray();

}

/***************************************************************************
 ***************************************************************************/
void ShutdownTray() {

	JXFreeGC(display, trayGC);
	JXFreeGC(display, bufferGC);
	JXFreePixmap(display, buffer);

	JXFreePixmap(display, minimizedPixmap);

	JXDestroyWindow(display, trayWindow);

}


/***************************************************************************
 ***************************************************************************/
void DestroyTray() {

	if(clockProgram) {
		Release(clockProgram);
		clockProgram = NULL;
	}
	if(loadProgram) {
		Release(loadProgram);
		loadProgram = NULL;
	}

	if(menuTitle) {
		Release(menuTitle);
		menuTitle = NULL;
	}

	if(menuIconName) {
		Release(menuIconName);
		menuIconName = NULL;
	}
}

/***************************************************************************
 ***************************************************************************/
void ShowTray() {
	if(trayIsHidden) {
		JXMoveWindow(display, trayWindow, 0,
			rootHeight - trayHeight - TRAY_BEVEL);
		trayIsHidden = 0;
	}
}

/***************************************************************************
 ***************************************************************************/
void HideTray() {

	if(autoHideTray && !trayIsHidden) {
		trayIsHidden = 1;
		JXMoveWindow(display, trayWindow, 0, rootHeight - 1);
	}

}

/***************************************************************************
 ***************************************************************************/
void AddClientToTray(ClientNode *np) {
	TrayNode *tp;

	tp = Allocate(sizeof(TrayNode));
	tp->client = np;

	switch(trayInsertMode) {
	case TRAY_INSERT_RIGHT:
		tp->next = NULL;
		if(trayNodesTail) {
			trayNodesTail->next = tp;
		} else {
			trayNodes = tp;
		}
		trayNodesTail = tp;
		break;
	default:
		tp->next = trayNodes;
		trayNodes = tp;
		if(!trayNodesTail) {
			trayNodesTail = tp;
		}
		break;
	}

	DrawTray();
}

/***************************************************************************
 ***************************************************************************/
void RemoveClientFromTray(ClientNode *np) {
	TrayNode *tp, *lp;

	lp = NULL;
	for(tp = trayNodes; tp; tp = tp->next) {
		if(tp->client == np) {
			if(tp == trayNodesTail) {
				trayNodesTail = lp;
			}
			if(lp) {
				lp->next = tp->next;
			} else {
				trayNodes = tp->next;
			}
			Release(tp);
			break;
		}
		lp = tp;
	}

	DrawTray();
}

/***************************************************************************
 ***************************************************************************/
void FocusNext() {
	TrayNode *tp;

	for(tp = trayNodes; tp; tp = tp->next) {
		if(!ShouldFocusItem(tp->client)) {
			continue;
		}
		if(tp->client->statusFlags & STAT_ACTIVE) {
			if(tp->next) {
				tp = tp->next;
			} else {
				tp = trayNodes;
			}
			break;
		}
	}
	if(!tp) {
		tp = trayNodes;
	}

	while(tp && !ShouldFocusItem(tp->client)) {
		tp = tp->next;
	}
	if(!tp) {
		tp = trayNodes;
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
int ProcessTrayEvent(const XEvent *event) {

	switch(event->type) {
	case Expose:
		return HandleTrayExpose(&event->xexpose);
	case EnterNotify:
		return HandleTrayEnterNotify(&event->xcrossing);
	case LeaveNotify:
		return HandleTrayLeaveNotify(&event->xcrossing);
	case ButtonPress:
		return HandleTrayButtonPress(&event->xbutton);
	case MotionNotify:
		return HandleTrayMotionNotify(&event->xmotion);
	default:
		return 0;
	}

}

/***************************************************************************
 ***************************************************************************/
int HandleTrayExpose(const XExposeEvent *event) {
	if(event->window == trayWindow) {
		DrawTray();
		return 1;
	} else {
		return 0;
	}
}

/***************************************************************************
 ***************************************************************************/
int HandleTrayEnterNotify(const XCrossingEvent *event) {
	if(event->window == trayWindow) {
		overTray = 1;
		ShowTray();
		return 1;
	} else {
		return 0;
	}
}

/***************************************************************************
 ***************************************************************************/
int HandleTrayLeaveNotify(const XCrossingEvent *event) {

	if(event->window == trayWindow) {
		overTray = 0;
		return 1;
	} else {
		return 0;
	}

}

/***************************************************************************
 ***************************************************************************/
int HandleTrayButtonPress(const XButtonEvent *event) {
	TrayNode *tp;
	int x, y;

	if(event->window != trayWindow) {
		return 0;
	}

	popupx = event->x;
	popupy = event->y;
	GetCurrentTime(&popupTime);

	x = event->x;
	y = event->y;

	if(y <= TRAY_BEVEL + 1 || y >= trayHeight) {
		return 1;
	}

	if(IsOverClock(x)) {
		if(clockProgram) {
			RunCommand(clockProgram);
		}
		return 1;
	}

	if(IsOverLoad(x)) {
		if(loadProgram) {
			RunCommand(loadProgram);
		}
		return 1;
	}

	if(IsOverMenuButton(x)) {
		ShowRootMenu(trayX, trayY);
		return 1;
	}

	if(IsOverPager(x)) {
		switch(event->button) {
		case Button1:
		case Button3:
			ChangeDesktop((x - trayStart) / (pagerWidth / desktopCount));
			break;
		case Button4:
			NextDesktop();
			break;
		case Button5:
			PreviousDesktop();
			break;
		default:
			break;
		}
		return 1;
	}

	tp = GetTrayNode(x);
	if(tp) {
		switch(event->button) {
		case Button1:
			if(tp->client->statusFlags & STAT_ACTIVE
				&& tp->client == nodes[tp->client->layer]) {
				MinimizeClient(tp->client);
			} else {
				RestoreClient(tp->client);
				FocusClient(tp->client);
			}
			break;
		case Button3:
			ShowWindowMenu(tp->client, x + trayX, y + trayY);
			break;
		default:
			break;
		}
	}

	return 1;

}

/***************************************************************************
 ***************************************************************************/
int HandleTrayMotionNotify(const XMotionEvent *event) {

	if(event->window == trayWindow) {

		GetCurrentTime(&popupTime);
		popupx = event->x;
		popupy = event->y;

		return 1;
	} else {
		return 0;
	}

}

/***************************************************************************
 ***************************************************************************/
int IsOverMenuButton(const int x) {
	return x < trayStart;
}

/***************************************************************************
 ***************************************************************************/
int IsOverPager(const int x) {
	if(x < trayStart + pagerWidth && x >= trayStart) {
		return 1;
	} else {
		return 0;
	}
}

/***************************************************************************
 ***************************************************************************/
int IsOverLoad(const int x) {
	if(x > trayWidth - trayStop
		&& x <= trayWidth - trayStop + loadWidth + 5) {
		return 1;
	} else {
		return 0;
	}
}

/***************************************************************************
 ***************************************************************************/
int IsOverClock(const int x) {
	return x > trayWidth - trayStop + loadWidth + 5;
}

/***************************************************************************
 ***************************************************************************/
TrayNode *GetTrayNode(int x) {
	TrayNode *tp;
	unsigned int remainder, itemCount, itemWidth;
	int index, stop;

	x -= trayStart + pagerWidth + 1;
	if(x < 0) {
		return NULL;
	}

	itemCount = GetItemCount();
	itemWidth = GetItemWidth(itemCount);

	remainder = trayWidth - trayStart - pagerWidth - 2
		- trayStop - (itemWidth * itemCount);

	index = 0;
	for(tp = trayNodes; tp; tp = tp->next) {
		if(ShouldShowItemInTray(tp->client)) {
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

	return NULL;

}

/***************************************************************************
 ***************************************************************************/
void NextDesktop() {
	ChangeDesktop((currentDesktop + 1) % desktopCount);
}

/***************************************************************************
 ***************************************************************************/
void PreviousDesktop() {
	if(currentDesktop > 0) {
		ChangeDesktop(currentDesktop - 1);
	} else {
		ChangeDesktop(desktopCount - 1);
	}
}

/***************************************************************************
 ***************************************************************************/
void ChangeDesktop(int desktop) {
	ClientNode *np;
	int x;

	if(desktop >= desktopCount || desktop < 0) {
		return;
	}

	if(currentDesktop == desktop && !initializing) {
		return;
	}

	for(x = 0; x < LAYER_COUNT; x++) {
		for(np = nodes[x]; np; np = np->next) {
			if(np->statusFlags & STAT_STICKY) {
				continue;
			}
			if(np->desktop == desktop) {
				ShowClient(np);
			} else if(np->desktop == currentDesktop) {
				HideClient(np);
			}
		}
	}

	currentDesktop = desktop;

	SetCardinalAtom(rootWindow, ATOM_NET_CURRENT_DESKTOP, currentDesktop);
	SetCardinalAtom(rootWindow, ATOM_WIN_WORKSPACE, currentDesktop);

	RestackClients();

	DrawTray();

}

/***************************************************************************
 ***************************************************************************/
void DrawTray() {
	TrayNode *tp;
	ButtonType buttonType;
	unsigned int width, x;
	unsigned int remainder;
	unsigned int itemCount;

	if(shouldExit) {
		return;
	}

	/* Clear tray. */
	JXSetForeground(display, bufferGC, colors[COLOR_TRAY_BG]);
	JXFillRectangle(display, buffer, bufferGC, 0, 0,
		trayWidth - trayStop - 1, trayHeight);

	/* Draw top bevel (this is done directly on the tray). */
	JXSetForeground(display, trayGC, colors[COLOR_TRAY_DOWN]);
	JXFillRectangle(display, trayWindow, trayGC, 0, 0,
		trayWidth, TRAY_BEVEL);

	SetButtonDrawable(buffer, bufferGC);
	SetButtonFont(FONT_TRAY);

	if(menuIcon || menuTitle[0]) {

		SetButtonSize(trayStart - 4, trayHeight - 3);
		if(menuIcon) {
			SetButtonAlignment(ALIGN_LEFT);
			SetButtonTextOffset(iconSize);
		} else {
			SetButtonAlignment(ALIGN_CENTER);
		}
		DrawButton(1, 1, BUTTON_TRAY, menuTitle);
		if(menuIcon) {
			PutIcon(menuIcon, buffer, bufferGC, 5,
				trayHeight / 2 - iconSize / 2);
		}

	}

	itemCount = GetItemCount();

	if(!itemCount) {
		UpdatePager();
		if(!UpdateTime()) {
			RenderTray();
		}
		return;
	}

	width = GetItemWidth(itemCount);
	remainder = trayWidth - trayStart - pagerWidth - 2
		- trayStop - (width * itemCount);

	SetButtonAlignment(ALIGN_LEFT);
	SetButtonTextOffset(iconSize);

	x = trayStart + pagerWidth + 2;
	for(tp = trayNodes; tp; tp = tp->next) {

		if(!ShouldShowItemInTray(tp->client)) {
			continue;
		}

		if(tp->client->statusFlags & STAT_ACTIVE) {
			buttonType = BUTTON_TRAY_ACTIVE;
		} else {
			buttonType = BUTTON_TRAY;
		}

		if(remainder) {
			SetButtonSize(width - 2, trayHeight - 3);
		} else {
			SetButtonSize(width - 3, trayHeight - 3);
		}

		DrawButton(x, 1, buttonType, tp->client->name);

		if(tp->client->icon) {
			PutIcon(tp->client->icon, buffer, bufferGC, x + 3,
				trayHeight / 2 - iconSize / 2);
		}

		if(tp->client->statusFlags & STAT_MINIMIZED) {
			JXCopyArea(display, minimizedPixmap, buffer, bufferGC, 0, 0,
				4, 4, x + 3, trayHeight - 8);
		}

		if(remainder) {
			x += width + 1;
			--remainder;
		} else {
			x += width;
		}

	}

	UpdatePager();
	if(!UpdateTime()) {
		RenderTray();
	}

}

/***************************************************************************
 ***************************************************************************/
void UpdatePager() {
	DrawPager(buffer, bufferGC, trayStart);
	JXCopyArea(display, buffer, trayWindow, trayGC,
		trayStart, 0, pagerWidth, trayHeight, trayStart, TRAY_BEVEL);
}

/***************************************************************************
 ***************************************************************************/
void RenderTray() {
	JXCopyArea(display, buffer, trayWindow, trayGC,
		0, 0, trayWidth - 1, trayHeight, 0, TRAY_BEVEL);
}

/***************************************************************************
 * Not reentrant
 ***************************************************************************/
char *GetLoadString() {
	static char str[80];
	FILE *fd;
	size_t len;

	fd = popen("uptime", "r");
	if(fd) {
		len = fread(str, sizeof(char), sizeof(str), fd);
		pclose(fd);
		if(len > 0) {
			return str;
		}
	}
	return "Error running command \"uptime\"";

}

/***************************************************************************
 ***************************************************************************/
int UpdateTime() {
	static TimeType lastTime = ZERO_TIME;
	TimeType currentTime;
	char *tstring;
	TrayNode *tp;
	unsigned int x;
	int rx, ry, wx, wy;
	Window win1, win2;

	GetCurrentTime(&currentTime);
	if(GetTimeDifference(&currentTime, &lastTime) < 1000) {
		return 0;
	}
	lastTime = currentTime;

	if(autoHideTray) {
		if(trayIsHidden) {
#ifdef SHOW_LOAD
			if(GetTimeDifference(&currentTime, &loadTime) >= 5000) {
				loadTime = currentTime;
				UpdateLoadDisplay(buffer, bufferGC, trayWidth - trayStop + 1);
			}
#endif
			return 1;
		} else if(!menuShown) {
			JXQueryPointer(display, rootWindow, &win1, &win2, &rx, &ry,
				&wx, &wy, &x);
			if(win2 != trayWindow) {
				HideTray();
				return 1;
			}
		}
	}

	if(overTray && GetTimeDifference(&currentTime, &popupTime) >= 1000) {

		if(IsOverClock(popupx)) {
			ShowPopup(popupx + trayX, rootHeight, GetLongTimeString());
		}

		if(IsOverLoad(popupx)) {
			ShowPopup(popupx + trayX, rootHeight, GetLoadString());
		}

		tp = GetTrayNode(popupx);
		if(tp && tp->client && tp->client->name) {
			ShowPopup(popupx + trayX, rootHeight, tp->client->name);
		}

	}

	tstring = GetShortTimeString();
	x = trayWidth - 4 
		- JXTextWidth(fonts[FONT_TRAY], tstring, strlen(tstring));

	JXSetForeground(display, bufferGC, colors[COLOR_TRAY_BG]);
	JXFillRectangle(display, buffer, bufferGC,
		trayWidth - trayStop + loadWidth + 2, 0,
		trayStop - loadWidth - 2, trayHeight);

	RenderString(buffer, bufferGC, FONT_TRAY, RAMP_TRAY,
		x, trayTextOffset, trayWidth, tstring);

#ifdef SHOW_LOAD
	if(GetTimeDifference(&currentTime, &loadTime) >= 5000) {
		loadTime = currentTime;
		UpdateLoadDisplay(buffer, bufferGC, trayWidth - trayStop + 1);
	}
#endif

	RenderTray();
	return 1;

}

/***************************************************************************
 ***************************************************************************/
unsigned int GetItemCount() {
	TrayNode *tp;
	unsigned int count;

	count = 0;
	for(tp = trayNodes; tp; tp = tp->next) {
		if(ShouldShowItemInTray(tp->client)) {
			++count;
		}
	}

	return count;
}

/***************************************************************************
 ***************************************************************************/
int ShouldShowItemInTray(const ClientNode *np) {

	if(np->desktop != currentDesktop
		&& !(np->statusFlags & STAT_STICKY)) {
		return 0;
	}

	if(np->owner != None || (np->statusFlags & STAT_NOLIST)
		|| (np->statusFlags & STAT_WITHDRAWN)
		|| (!(np->statusFlags & STAT_MAPPED)
		&& !(np->statusFlags & (STAT_MINIMIZED | STAT_SHADED)))) {
		return 0;
	}

	return 1;
}

/***************************************************************************
 ***************************************************************************/
int ShouldFocusItem(const ClientNode *np) {

	if(np->desktop != currentDesktop
		&& !(np->statusFlags & STAT_STICKY)) {
		return 0;
	}

	if(np->owner != None || (np->statusFlags & STAT_NOLIST)
		|| (np->statusFlags & STAT_WITHDRAWN)
		|| !(np->statusFlags & STAT_MAPPED)) {
		return 0;
	}

	return 1;
}

/***************************************************************************
 ***************************************************************************/
unsigned int GetItemWidth(unsigned int itemCount) {
	unsigned int width;

	width = trayWidth - trayStart - pagerWidth - 2 - trayStop;

	if(!itemCount) {
		return width;
	}

	width /= itemCount;
	if(!width) {
		width = 1;
	}
	if(maxTrayItemWidth > 0 && width > maxTrayItemWidth) {
		width = maxTrayItemWidth;
	}

	return width;
}

/***************************************************************************
 ***************************************************************************/
void SetTrayHeight(const char *str) {

	int height;

	Assert(str);

	height = atoi(str);

	if(height < MIN_TRAY_HEIGHT || height > MAX_TRAY_HEIGHT) {
		Warning("invalid tray height: %d", height);
		trayHeight = DEFAULT_TRAY_HEIGHT;
	} else {
		trayHeight = height;
	}

	height -= TRAY_BEVEL;

}

/***************************************************************************
 ***************************************************************************/
void SetTrayWidth(const char *str) {

	int width;

	Assert(str);

	width = atoi(str);

	if(width == 0) {
		trayWidth = 0;
	} else if(width < MIN_TRAY_WIDTH) {
		Warning("invalid tray width: %d", width);
		trayWidth = 0;
	} else {
		trayWidth = width;
	}

}

/***************************************************************************
 ***************************************************************************/
void SetTrayAlignment(const char *str) {

	Assert(str);

	if(!strcmp(str, "left")) {
		trayAlignment = TRAY_ALIGN_LEFT;
	} else if(!strcmp(str, "center")) {
		trayAlignment = TRAY_ALIGN_CENTER;
	} else if(!strcmp(str, "right")) {
		trayAlignment = TRAY_ALIGN_RIGHT;
	} else {
		Warning("invalid tray alignment: \"%s\"", str);
		trayAlignment = TRAY_ALIGN_CENTER;
	}

}

/***************************************************************************
 ***************************************************************************/
void SetTrayInsertMode(const char *str) {

	Assert(str);

	if(!strcmp(str, "left")) {
		trayInsertMode = TRAY_INSERT_LEFT;
	} else if(!strcmp(str, "right")) {
		trayInsertMode = TRAY_INSERT_RIGHT;
	} else {
		Warning("invalid tray insert mode: \"%s\"", str);
		trayInsertMode = TRAY_INSERT_LEFT;
	}

}

/***************************************************************************
 ***************************************************************************/
void SetClockProgram(const char *command) {
	if(clockProgram) {
		Release(clockProgram);
	}
	if(command) {
		clockProgram = Allocate(strlen(command) + 1);
		strcpy(clockProgram, command);
	} else {
		clockProgram = NULL;
	}
}

/***************************************************************************
 ***************************************************************************/
void SetLoadProgram(const char *command) {
	if(loadProgram) {
		Release(loadProgram);
	}
	if(command) {
		loadProgram = Allocate(strlen(command) + 1);
		strcpy(loadProgram, command);
	} else {
		loadProgram = NULL;
	}
}

/***************************************************************************
 ***************************************************************************/
void SetMenuTitle(const char *title) {

	Assert(title);

	if(menuTitle) {
		Release(menuTitle);
	}

	menuTitle = Allocate(strlen(title) + 1);
	strcpy(menuTitle, title);

}

/***************************************************************************
 ***************************************************************************/
void SetMenuIcon(const char *name) {

	Assert(name);

	if(menuIconName) {
		Release(menuIconName);
	}

	menuIconName = Allocate(strlen(name) + 1);
	strcpy(menuIconName, name);

}

/***************************************************************************
 ***************************************************************************/
void SetAutoHideTray(int v) {
	autoHideTray = v;
}

/***************************************************************************
 ***************************************************************************/
void SetMaxTrayItemWidth(const char *str) {

	int temp;

	Assert(str);

	temp = atoi(str);
	if(!temp || temp >= MIN_MAX_TRAY_ITEM_WIDTH) {
		maxTrayItemWidth = temp;
	} else {
		Warning("invalid max tray item width: %d", temp);
	}

}


