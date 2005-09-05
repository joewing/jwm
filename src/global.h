/****************************************************************************
 * Header for global variables.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef GLOBAL_H
#define GLOBAL_H

extern Display *display;
extern Window rootWindow;
extern unsigned int rootWidth, rootHeight;
extern unsigned int rootDepth;
extern unsigned int rootScreen;
extern Colormap rootColormap;
extern Visual *rootVisual;
extern unsigned int colormapCount;

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

