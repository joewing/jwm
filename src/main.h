/**
 * @file main.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the main functions.
 *
 */

#ifndef MAIN_H
#define MAIN_H

/** Enumeration of focus models. */
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
extern GC rootGC;
extern int colormapCount;

extern char *exitCommand;

extern unsigned int desktopWidth;
extern unsigned int desktopHeight;
extern unsigned int desktopCount;
extern unsigned int currentDesktop;

extern int shouldExit;
extern int shouldRestart;
extern int isRestarting;

extern int initializing;

extern int borderWidth;
extern int titleHeight;

extern unsigned int doubleClickSpeed;
extern unsigned int doubleClickDelta;

extern FocusModelType focusModel;

extern XContext clientContext;
extern XContext frameContext;

#ifdef USE_SHAPE
extern int haveShape;
extern int shapeEvent;
#endif
#ifdef USE_XRENDER
extern int haveRender;
#endif

#endif /* MAIN_H */

