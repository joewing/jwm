/****************************************************************************
 * The main entry point and related JWM functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"

static const char *CONFIG_FILE = "/.jwmrc";

static void Initialize();
static void Startup();
static void Shutdown();
static void Destroy();

static void OpenConnection();
static void CloseConnection();
static void StartupConnection();
static void ShutdownConnection();
static void EventLoop();
static void HandleExit();
static void DoExit(int code);
static void SendRestart();
static void SendExit();

static char *configPath = NULL;
static char *displayString = NULL;

/****************************************************************************
 ****************************************************************************/
int main(int argc, char *argv[]) {
	char *temp;
	int x;

	StartDebug();

	temp = getenv("HOME");
	if(temp) {
		configPath = Allocate(strlen(temp) + strlen(CONFIG_FILE) + 1);
		strcpy(configPath, temp);
		strcat(configPath, CONFIG_FILE);
	} else {
		configPath = Allocate(strlen(CONFIG_FILE) + 1);
		strcpy(configPath, CONFIG_FILE);
	}

	for(x = 1; x < argc; x++) {
		if(!strcmp(argv[x], "-v")) {
			DisplayAbout();
			DoExit(0);
		} else if(!strcmp(argv[x], "-h")) {
			DisplayHelp();
			DoExit(0);
		} else if(!strcmp(argv[x], "-p")) {
			Initialize();
			ParseConfig(configPath);
			DoExit(0);
		} else if(!strcmp(argv[x], "-restart")) {
			SendRestart();
			DoExit(0);
		} else if(!strcmp(argv[x], "-exit")) {
			SendExit();
			DoExit(0);
		} else if(!strcmp(argv[x], "-display") && x + 1 < argc) {
			displayString = argv[++x];
		} else {
			DisplayUsage();
			DoExit(1);
		}
	}

	do {
		shouldExit = 0;
		shouldRestart = 0;
		Initialize();
		ParseConfig(configPath);
		Startup();
		EventLoop();
		Shutdown();
		Destroy();
	} while(shouldRestart);

	if(exitCommand) {
		execl(SHELL_NAME, SHELL_NAME, "-c", exitCommand, NULL);
		Warning("exec failed: (%s) %s", SHELL_NAME, exitCommand);
		DoExit(1);
	} else {
		DoExit(0);
	}

	/* Control shoud never get here. */
	return -1;

}

/****************************************************************************
 ****************************************************************************/
void DoExit(int code) {
	Destroy();

	if(configPath) {
		Release(configPath);
		configPath = NULL;
	}
	if(exitCommand) {
		Release(exitCommand);
		exitCommand = NULL;
	}

	StopDebug();
	exit(code);
}

/****************************************************************************
 ****************************************************************************/
void EventLoop() {
	XEvent event;

	while(!shouldExit) {
		WaitForEvent(&event);
		ProcessEvent(&event);
	}
}

/****************************************************************************
 ****************************************************************************/
void OpenConnection() {
	display = JXOpenDisplay(displayString);
	if(!display) {
		if(displayString) {
			printf("error: could not open display %s\n", displayString);
		} else {
			printf("error: could not open display\n");
		}
		DoExit(1);
	}
	rootScreen = DefaultScreen(display);
	rootWindow = RootWindow(display, rootScreen);
	rootWidth = DisplayWidth(display, rootScreen);
	rootHeight = DisplayHeight(display, rootScreen);
	rootDepth = DefaultDepth(display, rootScreen);
	rootColormap = DefaultColormap(display, rootScreen);
	rootVisual = DefaultVisual(display, rootScreen);
	colormapCount = MaxCmapsOfScreen(ScreenOfDisplay(display, rootScreen));
}

/****************************************************************************
 ****************************************************************************/
void StartupConnection() {
	XSetWindowAttributes attr;
	int temp;

	initializing = 1;
	OpenConnection();

#if 0
	XSynchronize(display, True);
#endif

	JXSetErrorHandler(ErrorHandler);

	clientContext = XUniqueContext();
	frameContext = XUniqueContext();

	attr.event_mask = SubstructureRedirectMask | SubstructureNotifyMask
		| PropertyChangeMask | ColormapChangeMask | ButtonPressMask
		| ButtonReleaseMask;
	JXChangeWindowAttributes(display, rootWindow, CWEventMask, &attr);

	signal(SIGTERM, HandleExit);
	signal(SIGINT, HandleExit);
	signal(SIGHUP, HandleExit);

#ifdef USE_SHAPE
	haveShape = JXShapeQueryExtension(display, &shapeEvent, &temp);
	if(!haveShape) {
		Debug("No shape extension.");
	}
#endif

	initializing = 0;

}

/****************************************************************************
 ****************************************************************************/
void CloseConnection() {
	JXCloseDisplay(display);
}

/****************************************************************************
 ****************************************************************************/
void ShutdownConnection() {
	CloseConnection();
}

/****************************************************************************
 ****************************************************************************/
void HandleExit() {
	signal(SIGTERM, HandleExit);
	signal(SIGINT, HandleExit);
	signal(SIGHUP, HandleExit);
	shouldExit = 1;
}

/****************************************************************************
 * This is called before the X connection is opened.
 ****************************************************************************/
void Initialize() {
	InitializeBorders();
	InitializeClients();
	InitializeColors();
	InitializeCommands();
	InitializeCursors();
	#ifndef DISABLE_CONFIRM
		InitializeDialogs();
	#endif
	InitializeFonts();
	InitializeGroups();
	InitializeHints();
	InitializeIcons();
	InitializeKeys();
	InitializeLoadDisplay();
	InitializeOutline();
	InitializeOSDependent();
	InitializePager();
	InitializePopup();
	InitializeRootMenu();
	InitializeScreens();
	InitializeTiming();
	InitializeTray();
}

/****************************************************************************
 * This is called after the X connection is opened.
 ****************************************************************************/
void Startup() {

	/* This order is important. */

	StartupConnection();

	StartupCommands();

	StartupScreens();

	StartupGroups();
	StartupColors();
	StartupFonts();
	StartupCursors();
	StartupOutline();

	StartupLoadDisplay();
	StartupPager();
	StartupTray();
	StartupKeys();
	StartupHints();
	StartupBorders();
	StartupClients();

	StartupIcons();
	StartupTiming();
	#ifndef DISABLE_CONFIRM
		StartupDialogs();
	#endif
	StartupPopup();

	StartupRootMenu();

	SetDefaultCursor(rootWindow);
	ReadCurrentDesktop();
	JXFlush(display);

}

/****************************************************************************
 * This is called before the X connection is closed.
 ****************************************************************************/
void Shutdown() {

	/* This order is important. */

	ShutdownOutline();
	#ifndef DISABLE_CONFIRM
		ShutdownDialogs();
	#endif
	ShutdownPopup();
	ShutdownKeys();
	ShutdownPager();
	ShutdownRootMenu();
	ShutdownLoadDisplay();
	ShutdownTray();
	ShutdownBorders();
	ShutdownClients();
	ShutdownIcons();
	ShutdownCursors();
	ShutdownFonts();
	ShutdownColors();
	ShutdownGroups();

	ShutdownHints();
	ShutdownTiming();
	ShutdownScreens();

	ShutdownCommands();

	ShutdownConnection();

}

/****************************************************************************
 * This is called after the X connection is closed.
 * Note that it is possible for this to be called more than once.
 ****************************************************************************/
void Destroy() {
	DestroyBorders();
	DestroyClients();
	DestroyColors();
	DestroyCommands();
	DestroyCursors();
	#ifndef DISABLE_CONFIRM
		DestroyDialogs();
	#endif
	DestroyFonts();
	DestroyGroups();
	DestroyHints();
	DestroyIcons();
	DestroyKeys();
	DestroyLoadDisplay();
	DestroyOutline();
	DestroyOSDependent();
	DestroyPager();
	DestroyPopup();
	DestroyRootMenu();
	DestroyScreens();
	DestroyTiming();
	DestroyTray();
}

/****************************************************************************
 * Send _JWM_RESTART to the root window.
 ****************************************************************************/
void SendRestart() {

	Atom atom;

	OpenConnection();
	atom = JXInternAtom(display, "_JWM_RESTART", False);

	JXChangeProperty(display, rootWindow, atom, XA_CARDINAL, 32,
		PropModeReplace, (unsigned char*)&atom, 1);

	ShutdownHints();
	CloseConnection();
	DestroyHints();
}

/****************************************************************************
 * Send _JWM_EXIT to the root window.
 ****************************************************************************/
void SendExit() {

	Atom atom;

	OpenConnection();
	atom = JXInternAtom(display, "_JWM_EXIT", False);

	JXChangeProperty(display, rootWindow, atom, XA_CARDINAL, 32,
		PropModeReplace, (unsigned char*)&atom, 1);

	CloseConnection();
}

