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

/** Cursor types. */
typedef enum {
   CURSOR_DEFAULT,
   CURSOR_MOVE,
   CURSOR_NORTH,
   CURSOR_SOUTH,
   CURSOR_EAST,
   CURSOR_WEST,
   CURSOR_NE,
   CURSOR_NW,
   CURSOR_SE,
   CURSOR_SW,
   CURSOR_CHOOSE,
   CURSOR_CLOSE,
   CURSOR_MAXIMIZE,
   CURSOR_MINIMIZE,
   CURSOR_COUNT
} CursorType;

/** Cursors to load for the various cursor types.
 * This must be ordered the same as CursorType.
 */
static const unsigned int cursor_shapes[CURSOR_COUNT] = {
   XC_left_ptr,
   XC_fleur,
   XC_top_side,
   XC_bottom_side,
   XC_right_side,
   XC_left_side,
   XC_top_right_corner,
   XC_top_left_corner,
   XC_bottom_right_corner,
   XC_bottom_left_corner,
   XC_tcross,
   XC_dotbox,
   XC_based_arrow_up,
   XC_based_arrow_down
};

static Cursor cursors[CURSOR_COUNT];

static Cursor GetResizeCursor(BorderActionType action);
static Cursor CreateCursor(unsigned int shape);

static int mousex;
static int mousey;

/** Startup cursor support. */
void StartupCursors()
{

   Window win1, win2;
   int winx, winy;
   unsigned int mask;
   int x;

   for(x = 0; x < CURSOR_COUNT; x++) {
      cursors[x] = CreateCursor(cursor_shapes[x]);
   }

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
   int x;
   for(x = 0; x < CURSOR_COUNT; x++) {
      JXFreeCursor(display, cursors[x]);
   }
}

/** Get the cursor for the specified location on the frame. */
Cursor GetFrameCursor(BorderActionType action)
{
   switch(action & 0x0F) {
   case BA_RESIZE:
      return GetResizeCursor(action);
   case BA_CLOSE:
      return cursors[CURSOR_CLOSE];
   case BA_MAXIMIZE:
      return cursors[CURSOR_MAXIMIZE];
   case BA_MINIMIZE:
      return cursors[CURSOR_MINIMIZE];
   case BA_MOVE:
      break;
   default:
      break;
   }
   return cursors[CURSOR_DEFAULT];
}

/** Get the cursor for resizing on the specified frame location. */
Cursor GetResizeCursor(BorderActionType action)
{
   if(action & BA_RESIZE_N) {
      if(action & BA_RESIZE_E) {
         return cursors[CURSOR_NE];
      } else if(action & BA_RESIZE_W) {
         return cursors[CURSOR_NW];
      } else {
         return cursors[CURSOR_NORTH];
      }
   } else if(action & BA_RESIZE_S) {
      if(action & BA_RESIZE_E) {
         return cursors[CURSOR_SE];
      } else if(action & BA_RESIZE_W) {
         return cursors[CURSOR_SW];
      } else {
         return cursors[CURSOR_SOUTH];
      }
   } else {
      if(action & BA_RESIZE_E) {
         return cursors[CURSOR_EAST];
      } else {
         return cursors[CURSOR_WEST];
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
                          cursors[CURSOR_MOVE], CurrentTime);
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
                          cursors[CURSOR_DEFAULT], CurrentTime);
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
                          cursors[CURSOR_CHOOSE], CurrentTime);
   if(JLIKELY(result == GrabSuccess)) {
      return 1;
   } else {
      return 0;
   }
}

/** Set the default cursor for a window. */
void SetDefaultCursor(Window w)
{
   JXDefineCursor(display, w, cursors[CURSOR_DEFAULT]);
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

