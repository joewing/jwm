/****************************************************************************
 * Global variables.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"

Display *display = NULL;
Window rootWindow;
unsigned int rootWidth, rootHeight;
unsigned int rootDepth;
unsigned int rootScreen;
Colormap rootColormap;
Visual *rootVisual;
unsigned int colormapCount;

int shouldExit = 0;
int shouldRestart = 0;
int initializing = 0;

int desktopCount = 4;
int currentDesktop = 0;

char *exitCommand = NULL;

int borderWidth = DEFAULT_BORDER_WIDTH;
int titleHeight = DEFAULT_TITLE_HEIGHT;
int titleSize = DEFAULT_TITLE_HEIGHT + DEFAULT_BORDER_WIDTH;

int doubleClickSpeed = DEFAULT_DOUBLE_CLICK_SPEED;
int doubleClickDelta = DEFAULT_DOUBLE_CLICK_DELTA;

FocusModelType focusModel = FOCUS_SLOPPY;

XContext clientContext;
XContext frameContext;

#ifdef USE_SHAPE
int haveShape;
int shapeEvent;
#endif

