/**
 * @file popup.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions for displaying popup windows.
 *
 */

#include "jwm.h"
#include "popup.h"
#include "main.h"
#include "color.h"
#include "font.h"
#include "screen.h"
#include "cursor.h"
#include "error.h"
#include "timing.h"
#include "misc.h"
#include "settings.h"

typedef struct PopupType {
   int x, y;   /* The coordinates of the upper-left corner of the popup. */
   int mx, my; /* The mouse position when the popup was created. */
   int width, height;
   char *text;
   Window window;
} PopupType;

static PopupType popup;

static void DrawPopup();

/** Initialize popup data. */
void InitializePopup()
{
}

/** Startup popups. */
void StartupPopup()
{
   popup.text = NULL;
   popup.window = None;
}

/** Shutdown popups. */
void ShutdownPopup()
{
   if(popup.text) {
      Release(popup.text);
      popup.text = NULL;
   }
   if(popup.window != None) {
      JXDestroyWindow(display, popup.window);
      popup.window = None;
   }
}

/** Destroy popup data. */
void DestroyPopup()
{
}

/** Show a popup window. */
void ShowPopup(int x, int y, const char *text)
{

   unsigned long attrMask;
   XSetWindowAttributes attr;
   const ScreenType *sp;

   Assert(text);

   if(!settings.popupEnabled) {
      return;
   }

   if(popup.text) {
      if(x == popup.mx && y == popup.my && !strcmp(popup.text, text)) {
         // This popup is already shown.
         return;
      }
      Release(popup.text);
      popup.text = NULL;
   }

   if(text[0] == 0) {
      return;
   }

   popup.text = CopyString(text);
   popup.height = GetStringHeight(FONT_POPUP) + 2;
   popup.width = GetStringWidth(FONT_POPUP, popup.text) + 9;

   sp = GetCurrentScreen(x, y);

   if(popup.width > sp->width) {
      popup.width = sp->width;
   }

   popup.x = x;
   popup.y = y - popup.height - 2;

   if(popup.width + popup.x >= sp->width) {
      popup.x = sp->width - popup.width - 2;
   }
   if(popup.height + popup.y >= sp->height) {
      popup.y = sp->height - popup.height - 2;
   }
   if (popup.x < 2) {
      popup.x = 2;
   }
   if (popup.y < 2) {
      popup.y = 2;
   }

   if(popup.window == None) {

      attrMask = 0;

      attrMask |= CWEventMask;
      attr.event_mask
         = ExposureMask
         | PointerMotionMask | PointerMotionHintMask;

      attrMask |= CWSaveUnder;
      attr.save_under = True;

      attrMask |= CWBackPixel;
      attr.background_pixel = colors[COLOR_POPUP_BG];

      attrMask |= CWBorderPixel;
      attr.border_pixel = colors[COLOR_POPUP_OUTLINE];

      attrMask |= CWDontPropagate;
      attr.do_not_propagate_mask
         = PointerMotionMask
         | ButtonPressMask
         | ButtonReleaseMask;

      popup.window = JXCreateWindow(display, rootWindow, popup.x, popup.y,
                                    popup.width, popup.height, 1,
                                    CopyFromParent, InputOutput,
                                    CopyFromParent, attrMask, &attr);
      JXMapRaised(display, popup.window);

   } else {

      JXMoveResizeWindow(display, popup.window, popup.x, popup.y,
                         popup.width, popup.height);
      DrawPopup();

   }

   popup.mx = x;
   popup.my = y;

}

/** Signal popup (this is used to hide popups after awhile). */
void SignalPopup(const TimeType *now, int x, int y)
{
   if(popup.window != None) {
      if(abs(popup.mx - x) > 2 || abs(popup.my - y) > 2) {
         JXDestroyWindow(display, popup.window);
         popup.window = None;
      }
   }
}

/** Process an event on a popup window. */
char ProcessPopupEvent(const XEvent *event)
{
   if(event->xany.window == popup.window) {
      if(event->type == Expose) {
         DrawPopup();
         return 1;
      } else if(event->type == MotionNotify) {
         JXDestroyWindow(display, popup.window);
         popup.window = None;
         return 1;
      }
   }
   return 0;
}

/** Draw the popup window. */
void DrawPopup()
{
   JXClearWindow(display, popup.window);
   RenderString(popup.window, FONT_POPUP, COLOR_POPUP_FG, 4, 1,
                popup.width, popup.text);
}

