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
#include "timing.h"
#include "misc.h"
#include "settings.h"
#include "event.h"

typedef struct PopupType {
   int x, y;   /* The coordinates of the upper-left corner of the popup. */
   int mx, my; /* The mouse position when the popup was created. */
   Window mw;
   int width, height;
   char *text;
   Window window;
   Pixmap pmap;
} PopupType;

static PopupType popup;

static void SignalPopup(const TimeType *now, int x, int y, Window w,
                        void *data);

/** Startup popups. */
void StartupPopup(void)
{
   popup.text = NULL;
   popup.window = None;
   RegisterCallback(100, SignalPopup, NULL);
}

/** Shutdown popups. */
void ShutdownPopup(void)
{
   UnregisterCallback(SignalPopup, NULL);
   if(popup.text) {
      Release(popup.text);
      popup.text = NULL;
   }
   if(popup.window != None) {
      JXDestroyWindow(display, popup.window);
      JXFreePixmap(display, popup.pmap);
      popup.window = None;
   }
}

/** Show a popup window. */
void ShowPopup(int x, int y, const char *text,
               const PopupMaskType context)
{

   const ScreenType *sp;

   Assert(text);

   if(!(settings.popupMask & context)) {
      return;
   }

   if(popup.text) {
      if(x == popup.x && y == popup.y && !strcmp(popup.text, text)) {
         // This popup is already shown.
         return;
      }
      Release(popup.text);
      popup.text = NULL;
   }

   if(text[0] == 0) {
      return;
   }

   GetMousePosition(&popup.mx, &popup.my, &popup.mw);
   popup.text = CopyString(text);
   popup.height = GetStringHeight(FONT_POPUP) + 2;
   popup.width = GetStringWidth(FONT_POPUP, popup.text) + 9;

   sp = GetCurrentScreen(x, y);

   if(popup.width > sp->width) {
      popup.width = sp->width;
   }

   popup.x = x;
   if(y + 2 * popup.height + 2 >= sp->height) {
      popup.y = y - popup.height - 2;
   } else {
      popup.y = y + popup.height + 2;
   }

   if(popup.width + popup.x > sp->x + sp->width) {
      popup.x = sp->x + sp->width - popup.width - 2;
   }
   if(popup.height + popup.y > sp->y + sp->height) {
      popup.y = sp->y + sp->height - popup.height - 2;
   }
   if(popup.x < 2) {
      popup.x = 2;
   }
   if(popup.y < 2) {
      popup.y = 2;
   }

   if(popup.window == None) {

      XSetWindowAttributes attr;
      unsigned long attrMask = 0;

      attrMask |= CWEventMask;
      attr.event_mask = ExposureMask
                      | PointerMotionMask | PointerMotionHintMask;

      attrMask |= CWSaveUnder;
      attr.save_under = True;

      attrMask |= CWDontPropagate;
      attr.do_not_propagate_mask = PointerMotionMask
                                 | ButtonPressMask
                                 | ButtonReleaseMask;

      popup.window = JXCreateWindow(display, rootWindow, popup.x, popup.y,
                                    popup.width, popup.height, 0,
                                    CopyFromParent, InputOutput,
                                    CopyFromParent, attrMask, &attr);
      JXMapRaised(display, popup.window);

   } else {

      JXMoveResizeWindow(display, popup.window, popup.x, popup.y,
                         popup.width, popup.height);
      JXFreePixmap(display, popup.pmap);

   }

   popup.pmap = JXCreatePixmap(display, popup.window,
                               popup.width, popup.height,
                               rootDepth);

   JXSetForeground(display, rootGC, colors[COLOR_POPUP_BG]);
   JXFillRectangle(display, popup.pmap, rootGC, 0, 0,
                   popup.width - 1, popup.height - 1);
   JXSetForeground(display, rootGC, colors[COLOR_POPUP_OUTLINE]);
   JXDrawRectangle(display, popup.pmap, rootGC, 0, 0,
                   popup.width - 1, popup.height - 1);
   RenderString(popup.pmap, FONT_POPUP, COLOR_POPUP_FG, 4, 1,
                popup.width, popup.text);
   JXCopyArea(display, popup.pmap, popup.window, rootGC,
              0, 0, popup.width, popup.height, 0, 0);

}

/** Signal popup (this is used to hide popups after awhile). */
void SignalPopup(const TimeType *now, int x, int y, Window w, void *data)
{
   if(popup.window != None) {
      if(popup.mw != w ||
         abs(popup.mx - x) > 0 || abs(popup.my - y) > 0) {
         JXDestroyWindow(display, popup.window);
         JXFreePixmap(display, popup.pmap);
         popup.window = None;
      }
   }
}

/** Process an event on a popup window. */
char ProcessPopupEvent(const XEvent *event)
{
   if(popup.window != None && event->xany.window == popup.window) {
      if(event->type == Expose && event->xexpose.count == 0) {
         JXCopyArea(display, popup.pmap, popup.window, rootGC,
                    0, 0, popup.width, popup.height, 0, 0);
      } else if(event->type == MotionNotify) {
         JXDestroyWindow(display, popup.window);
         JXFreePixmap(display, popup.pmap);
         popup.window = None;
      }
      return 1;
   }
   return 0;
}

