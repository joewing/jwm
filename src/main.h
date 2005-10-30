/****************************************************************************
 * Header for the main functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef MAIN_H
#define MAIN_H

typedef enum {
	FOCUS_SLOPPY              = 0,
	FOCUS_CLICK               = 1
} FocusModelType;

extern Display *display;
extern Window rootWindow;
extern int rootWidth, rootHeight;
extern int rootDepth;
extern int rootScreen;
extern Colormap rootColormap;
extern Visual *rootVisual;
extern int colormapCount;

extern char *exitCommand;

extern int desktopCount;
extern int currentDesktop;

extern int shouldExit;
extern int shouldRestart;

extern int initializing;

extern int borderWidth;
extern int titleSize, titleHeight;

extern int doubleClickSpeed;
extern int doubleClickDelta;

extern FocusModelType focusModel;

extern XContext clientContext;
extern XContext frameContext;

#ifdef USE_SHAPE
extern int haveShape;
extern int shapeEvent;
#endif

#endif

