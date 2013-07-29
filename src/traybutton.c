/**
 * @file traybutton.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Button tray component.
 *
 */

#include "jwm.h"
#include "traybutton.h"
#include "tray.h"
#include "icon.h"
#include "image.h"
#include "error.h"
#include "root.h"
#include "main.h"
#include "color.h"
#include "font.h"
#include "button.h"
#include "misc.h"
#include "screen.h"
#include "desktop.h"
#include "popup.h"
#include "timing.h"
#include "command.h"
#include "cursor.h"
#include "settings.h"
#include "event.h"
#include "mouse.h"
#include "binding.h"
#include "move.h"
#include "resize.h"

#define BUTTON_SIZE 4

typedef struct TrayButtonType {

   TrayComponentType *cp;

   char *label;
   char *popup;
   char *iconName;
   IconNode *icon;
   char border;
   ContextType context;

   int mousex;
   int mousey;
   TimeType mouseTime;

   struct TrayButtonType *next;

} TrayButtonType;

static TrayButtonType *buttons = NULL;

static void Create(TrayComponentType *cp);
static void Destroy(TrayComponentType *cp);
static void SetSize(TrayComponentType *cp, int width, int height);
static void Resize(TrayComponentType *cp);
static void Draw(TrayComponentType *cp, int active);

static void ProcessButtonEvent(TrayComponentType *cp,
                               const XButtonEvent *event,
                               int x, int y);
static void ProcessButtonRelease(const XButtonEvent *event, void *arg);
static void ProcessMotionEvent(TrayComponentType *cp,
                               int x, int y, int mask);
static void SignalTrayButton(const TimeType *now,
                             int x, int y, void *data);

/** Startup tray buttons. */
void StartupTrayButtons()
{
   TrayButtonType *bp;
   for(bp = buttons; bp; bp = bp->next) {
      if(bp->label) {
         bp->cp->requestedWidth
            = GetStringWidth(FONT_TRAYBUTTON, bp->label) + 4;
         bp->cp->requestedHeight
            = GetStringHeight(FONT_TRAYBUTTON);
      } else {
         bp->cp->requestedWidth = 0;
         bp->cp->requestedHeight = 0;
      }
      if(bp->iconName) {
         bp->icon = LoadNamedIcon(bp->iconName, 1);
         if(JLIKELY(bp->icon)) {
            bp->cp->requestedWidth += bp->icon->image->width;
            bp->cp->requestedHeight += bp->icon->image->height;
         } else {
            Warning(_("could not load tray icon: \"%s\""), bp->iconName);
         }
      }
      bp->cp->requestedWidth += 2 * BUTTON_SIZE;
      bp->cp->requestedHeight += 2 * BUTTON_SIZE;
   }
}

/** Release tray button data. */
void DestroyTrayButtons()
{
   TrayButtonType *bp;
   while(buttons) {
      bp = buttons->next;
      UnregisterCallback(SignalTrayButton, buttons);
      if(buttons->label) {
         Release(buttons->label);
      }
      if(buttons->iconName) {
         Release(buttons->iconName);
      }
      if(buttons->popup) {
         Release(buttons->popup);
      }
      Release(buttons);
      buttons = bp;
   }
}

/** Create a button tray component. */
TrayComponentType *CreateTrayButton(const char *iconName,
                                    const char *label,
                                    const char *popup,
                                    unsigned int width,
                                    unsigned int height,
                                    char border)
{

   TrayButtonType *bp;
   TrayComponentType *cp;

   if(JUNLIKELY((label == NULL || strlen(label) == 0)
      && (iconName == NULL || strlen(iconName) == 0))) {
      Warning(_("no icon or label for TrayButton"));
      return NULL;
   }

   bp = Allocate(sizeof(TrayButtonType));
   bp->next = buttons;
   buttons = bp;

   bp->icon = NULL;
   bp->iconName = CopyString(iconName);
   bp->label = CopyString(label);
   bp->popup = CopyString(popup);
   bp->border = border;
   bp->context = CreateMouseContext();

   cp = CreateTrayComponent();
   cp->object = bp;
   bp->cp = cp;
   cp->requestedWidth = width;
   cp->requestedHeight = height;

   bp->mousex = -settings.doubleClickDelta;
   bp->mousey = -settings.doubleClickDelta;

   cp->Create = Create;
   cp->Destroy = Destroy;
   cp->SetSize = SetSize;
   cp->Resize = Resize;

   cp->ProcessButtonEvent = ProcessButtonEvent;
   if(popup || label) {
      cp->ProcessMotionEvent = ProcessMotionEvent;
   }

   RegisterCallback(settings.popupDelay / 2, SignalTrayButton, bp);

   return cp;

}

/** Add an action to a tray button. */
void AddTrayButtonAction(const TrayComponentType *cp,
                         int button,
                         const char *mask,
                         const ActionNode *action)
{
   const TrayButtonType *bp = (const TrayButtonType*)cp->object;
   InsertMouseBinding(bp->context, button, mask, action);
}

/** Set the size of a button tray component. */
void SetSize(TrayComponentType *cp, int width, int height)
{

   TrayButtonType *bp;
   int labelWidth, labelHeight;
   int iconWidth, iconHeight;
   int ratio;

   bp = (TrayButtonType*)cp->object;

   if(bp->icon) {

      if(bp->label) {
         labelWidth = GetStringWidth(FONT_TRAYBUTTON, bp->label) + 4;
         labelHeight = GetStringHeight(FONT_TRAYBUTTON);
      } else {
         labelWidth = 0;
         labelHeight = 0;
      }

      iconWidth = bp->icon->image->width;
      iconHeight = bp->icon->image->height;

      /* Fixed point with 16 bit fraction. */
      ratio = (iconWidth << 16) / iconHeight;

      if(width > 0) {

         /* Compute height from width. */
         iconWidth = width - labelWidth - 2 * BUTTON_SIZE;
         iconHeight = (iconWidth << 16) / ratio;
         height = Max(iconHeight, labelHeight) + 2 * BUTTON_SIZE;

      } else if(height > 0) {

         /* Compute width from height. */
         iconHeight = height - 2 * BUTTON_SIZE;
         iconWidth = (iconHeight * ratio) >> 16;
         width = iconWidth + labelWidth + 2 * BUTTON_SIZE;

      }

      cp->width = width;
      cp->height = height;

   }

}

/** Initialize a button tray component. */
void Create(TrayComponentType *cp)
{
   cp->pixmap = JXCreatePixmap(display, rootWindow,
                               cp->width, cp->height, rootDepth);
   Draw(cp, 0);
}

/** Resize a button tray component. */
void Resize(TrayComponentType *cp)
{
   Destroy(cp);
   Create(cp);
}

/** Destroy a button tray component. */
void Destroy(TrayComponentType *cp)
{
   if(cp->pixmap != None) {
      JXFreePixmap(display, cp->pixmap);
   }
}

/** Draw a tray button. */
void Draw(TrayComponentType *cp, int active)
{

   ButtonNode button;
   TrayButtonType *bp;

   bp = (TrayButtonType*)cp->object;

   ClearTrayDrawable(cp);
   ResetButton(&button, cp->pixmap, rootGC);
   if(active) {
      button.type = BUTTON_TRAY_ACTIVE;
   } else {
      button.border = bp->border;
      button.type = BUTTON_TRAY;
   }
   button.width = cp->width - 3;
   button.height = cp->height - 3;
   button.x = 1;
   button.y = 1;
   button.font = FONT_TRAYBUTTON;
   button.text = bp->label;
   button.icon = bp->icon;
   DrawButton(&button);

}

/** Process a button event. */
void ProcessButtonEvent(TrayComponentType *cp,
                        const XButtonEvent *event,
                        int x, int y)
{

   TrayButtonType *bp = (TrayButtonType*)cp->object;
   ActionContext context;

   InitActionContext(&context);
   context.data = bp;
   context.x = event->x_root;
   context.y = event->y_root;
   context.MoveFunc = MoveClientKeyboard;
   context.ResizeFunc = ResizeClientKeyboard;

   Draw(cp, 1);
   UpdateSpecificTray(cp->tray, cp);

   if(RunMouseCommand(event, bp->context, &context)) {
      Draw(cp, 0);
      UpdateSpecificTray(cp->tray, cp);
   } else {
      GrabMouse(cp->tray->window);
      SetButtonReleaseCallback(ProcessButtonRelease, bp);
   }

}

/** Process a button release. */
void ProcessButtonRelease(const XButtonEvent *event, void *arg)
{

   const TrayButtonType *bp = (TrayButtonType*)arg;
   TrayComponentType *cp = bp->cp;
   ActionContext context;

   InitActionContext(&context);
   context.data = arg;
   context.x = event->x_root;
   context.y = event->y_root;
   context.MoveFunc = MoveClientKeyboard;
   context.ResizeFunc = ResizeClientKeyboard;
   RunMouseCommand(event, bp->context, &context);

   Draw(cp, 0);
   UpdateSpecificTray(cp->tray, cp);
}

/** Process a motion event. */
void ProcessMotionEvent(TrayComponentType *cp, int x, int y, int mask)
{
   TrayButtonType *bp = (TrayButtonType*)cp->object;
   bp->mousex = cp->screenx + x;
   bp->mousey = cp->screeny + y;
   GetCurrentTime(&bp->mouseTime);
}

/** Signal (needed for popups). */
void SignalTrayButton(const TimeType *now, int x, int y, void *data)
{
   TrayButtonType *bp = (TrayButtonType*)data;
   const char *popup;

   if(bp->popup) {
      popup = bp->popup;
   } else if(bp->label) {
      popup = bp->label;
   } else {
      return;
   }
   if(abs(bp->mousex - x) < settings.doubleClickDelta
      && abs(bp->mousey - y) < settings.doubleClickDelta) {
      if(GetTimeDifference(now, &bp->mouseTime) >= settings.popupDelay) {
         ShowPopup(x, y, popup);
      }
   }
}

