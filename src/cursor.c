/**
 * @file cursor.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Cursor functions.
 *
 */

#include "jwm.h"
#include "cursor.h"
#include "main.h"
#include "error.h"

static Cursor defaultCursor;
static Cursor moveCursor;
static Cursor northCursor;
static Cursor southCursor;
static Cursor eastCursor;
static Cursor westCursor;
static Cursor northEastCursor;
static Cursor northWestCursor;
static Cursor southEastCursor;
static Cursor southWestCursor;
static Cursor chooseCursor;

static Cursor GetResizeCursor(BorderActionType action);
static Cursor CreateCursor(unsigned int shape);

static int mousex;
static int mousey;

/** Initialize cursor data. */
void InitializeCursors()
{
}

/** Startup cursor support. */
void StartupCursors()
{

   Window win1, win2;
   int winx, winy;
   unsigned int mask;

   defaultCursor = CreateCursor(XC_left_ptr);
   moveCursor = CreateCursor(XC_fleur);
   northCursor = CreateCursor(XC_top_side);
   southCursor = CreateCursor(XC_bottom_side);
   eastCursor = CreateCursor(XC_right_side);
   westCursor = CreateCursor(XC_left_side);
   northEastCursor = CreateCursor(XC_top_right_corner);
   northWestCursor = CreateCursor(XC_top_left_corner);
   southEastCursor = CreateCursor(XC_bottom_right_corner);
   southWestCursor = CreateCursor(XC_bottom_left_corner);
   chooseCursor = CreateCursor(XC_tcross);

   JXQueryPointer(display, rootWindow, &win1, &win2,
                  &mousex, &mousey, &winx, &winy, &mask);

}

/** Create a cursor for the specified shape. */
Cursor CreateCursor(unsigned int shape)
{
   return JXCreateFontCursor(display, shape);
}

/** Shutdown cursor support. */
void ShutdownCursors()
{
   JXFreeCursor(display, defaultCursor);
   JXFreeCursor(display, moveCursor);
   JXFreeCursor(display, northCursor);
   JXFreeCursor(display, southCursor);
   JXFreeCursor(display, eastCursor);
   JXFreeCursor(display, westCursor);
   JXFreeCursor(display, northEastCursor);
   JXFreeCursor(display, northWestCursor);
   JXFreeCursor(display, southEastCursor);
   JXFreeCursor(display, southWestCursor);
   JXFreeCursor(display, chooseCursor);
}

/** Destroy cursor data. */
void DestroyCursors()
{
}

/** Get the cursor for the specified location on the frame. */
Cursor GetFrameCursor(BorderActionType action)
{
   switch(action & 0x0F) {
   case BA_RESIZE:
      return GetResizeCursor(action);
   case BA_CLOSE:
      break;
   case BA_MAXIMIZE:
      break;
   case BA_MINIMIZE:
      break;
   case BA_MOVE:
      break;
   default:
      break;
   }
   return defaultCursor;
}

/** Get the cursor for resizing on the specified frame location. */
Cursor GetResizeCursor(BorderActionType action)
{
   if(action & BA_RESIZE_N) {
      if(action & BA_RESIZE_E) {
         return northEastCursor;
      } else if(action & BA_RESIZE_W) {
         return northWestCursor;
      } else {
         return northCursor;
      }
   } else if(action & BA_RESIZE_S) {
      if(action & BA_RESIZE_E) {
         return southEastCursor;
      } else if(action & BA_RESIZE_W) {
         return southWestCursor;
      } else {
         return southCursor;
      }
   } else {
      if(action & BA_RESIZE_E) {
         return eastCursor;
      } else {
         return westCursor;
      }
   }
}

/** Grab the mouse for resizing a window. */
char GrabMouseForResize(BorderActionType action)
{
   Cursor cur;
   int result;
   unsigned int mask;

   cur = GetFrameCursor(action);
   mask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
   result = JXGrabPointer(display, rootWindow, False, mask,
                          GrabModeAsync, GrabModeAsync, None,
                          cur, CurrentTime);
   if(JLIKELY(result == GrabSuccess)) {
      return 1;
   } else {
      return 0;
   }
}

/** Grab the mouse for moving a window. */
char GrabMouseForMove()
{
   int result;
   unsigned int mask;
   mask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
   result = JXGrabPointer(display, rootWindow, False, mask,
                          GrabModeAsync, GrabModeAsync, None,
                          moveCursor, CurrentTime);
   if(JLIKELY(result == GrabSuccess)) {
      return 1;
   } else {
      return 0;
   }
}

/** Grab the mouse. */
char GrabMouse(Window w)
{
   int result;
   unsigned int mask;
   mask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
   result = JXGrabPointer(display, w, False, mask,
                          GrabModeAsync, GrabModeAsync, None,
                          defaultCursor, CurrentTime);
   if(JLIKELY(result == GrabSuccess)) {
      return 1;
   } else {
      return 0;
   }
}

/** Grab the mouse for choosing a window. */
char GrabMouseForChoose()
{
   int result;
   unsigned int mask;
   mask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
   result = JXGrabPointer(display, rootWindow, False, mask,
                          GrabModeAsync, GrabModeAsync, None,
                          chooseCursor, CurrentTime);
   if(JLIKELY(result == GrabSuccess)) {
      return 1;
   } else {
      return 0;
   }
}

/** Set the default cursor for a window. */
void SetDefaultCursor(Window w)
{
   JXDefineCursor(display, w, defaultCursor);
}

/** Move the mouse to the specified coordinates on a window. */
void MoveMouse(Window win, int x, int y)
{
   Window win1, win2;
   int winx, winy;
   unsigned int mask;
   JXWarpPointer(display, None, win, 0, 0, 0, 0, x, y);
   JXQueryPointer(display, rootWindow, &win1, &win2,
                  &mousex, &mousey, &winx, &winy, &mask);
}

/** Set the current mouse position. */
void SetMousePosition(int x, int y)
{
   mousex = x;
   mousey = y;
}

/** Get the current mouse position. */
void GetMousePosition(int *x, int *y)
{
   Assert(x);
   Assert(y);
   *x = mousex;
   *y = mousey;
}

/** Get the current mouse buttons pressed. */
unsigned int GetMouseMask()
{
   Window win1, win2;
   int winx, winy;
   unsigned int mask;
   JXQueryPointer(display, rootWindow, &win1, &win2,
                  &mousex, &mousey, &winx, &winy, &mask);
   return mask;
}

