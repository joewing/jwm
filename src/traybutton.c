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

typedef struct TrayButtonActionType {
   char *action;
   int mask;
   struct TrayButtonActionType *next;
} TrayButtonActionType;

typedef struct TrayButtonType {

   TrayComponentType *cp;

   char *label;
   char *popup;
   char *iconName;
   IconNode *icon;

   int mousex;
   int mousey;
   TimeType mouseTime;

   struct TrayButtonActionType *actions;
   struct TrayButtonType *next;

} TrayButtonType;

static TrayButtonType *buttons = NULL;

static void Create(TrayComponentType *cp);
static void Destroy(TrayComponentType *cp);
static void SetSize(TrayComponentType *cp, int width, int height);
static void Resize(TrayComponentType *cp);
static void Draw(TrayComponentType *cp, int active);

static void ProcessButtonPress(TrayComponentType *cp,
                               int x, int y, int button);
static void ProcessButtonRelease(TrayComponentType *cp,
                                 int x, int y, int button);
static void ProcessMotionEvent(TrayComponentType *cp,
                               int x, int y, int mask);
static void SignalTrayButton(const TimeType *now,
                             int x, int y, Window w, void *data);

/** Startup tray buttons. */
void StartupTrayButtons(void)
{
   TrayButtonType *bp;
   for(bp = buttons; bp; bp = bp->next) {
      if(bp->label) {
         bp->cp->requestedWidth
            = GetStringWidth(FONT_TRAYBUTTON, bp->label) + 4;
         bp->cp->requestedHeight = GetStringHeight(FONT_TRAYBUTTON);
      } else {
         bp->cp->requestedWidth = 0;
         bp->cp->requestedHeight = 0;
      }
      if(bp->iconName) {
         bp->icon = LoadNamedIcon(bp->iconName, 1, 1);
         if(JLIKELY(bp->icon)) {
            bp->cp->requestedWidth += bp->icon->image->width + 4;
            if(bp->label) {
               bp->cp->requestedWidth -= 2;
            }
            bp->cp->requestedHeight
               = Max(bp->icon->image->height + 4, bp->cp->requestedHeight);
         } else {
            Warning(_("could not load tray icon: \"%s\""), bp->iconName);
         }
      }
   }
}

/** Release tray button data. */
void DestroyTrayButtons(void)
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
      while(buttons->actions) {
         TrayButtonActionType *ap = buttons->actions->next;
         Release(buttons->actions->action);
         Release(buttons->actions);
         buttons->actions = ap;
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
                                    unsigned int height)
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
   bp->actions = NULL;
   bp->popup = CopyString(popup);

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

   cp->ProcessButtonPress = ProcessButtonPress;
   cp->ProcessButtonRelease = ProcessButtonRelease;
   if(popup || label) {
      cp->ProcessMotionEvent = ProcessMotionEvent;
   }

   RegisterCallback(settings.popupDelay / 2, SignalTrayButton, bp);

   return cp;

}

/** Add a action to a tray button. */
void AddTrayButtonAction(TrayComponentType *cp,
                         const char *action,
                         int mask)
{
   TrayButtonType *bp = (TrayButtonType*)cp->object;
   TrayButtonActionType *ap
      = (TrayButtonActionType*)Allocate(sizeof(TrayButtonActionType));
   ap->action = CopyString(action);
   ap->mask = mask;
   ap->next = bp->actions;
   bp->actions = ap;
}

/** Set the size of a button tray component. */
void SetSize(TrayComponentType *cp, int width, int height)
{

   TrayButtonType *bp;

   bp = (TrayButtonType*)cp->object;

   if(bp->icon) {

      int labelWidth, labelHeight;
      int iconWidth, iconHeight;
      int ratio;

      if(bp->label) {
         labelWidth = GetStringWidth(FONT_TRAYBUTTON, bp->label) + 6;
         labelHeight = GetStringHeight(FONT_TRAYBUTTON) + 6;
      } else {
         labelWidth = 4;
         labelHeight = 4;
      }

      iconWidth = bp->icon->image->width;
      iconHeight = bp->icon->image->height;

      /* Fixed point with 16 bit fraction. */
      ratio = (iconWidth << 16) / iconHeight;

      if(width > 0) {

         /* Compute height from width. */
         iconWidth = width - labelWidth;
         iconHeight = (iconWidth << 16) / ratio;
         height = labelHeight;

      } else if(height > 0) {

         /* Compute width from height. */
         iconHeight = height - 4;
         iconWidth = (iconHeight * ratio) >> 16;
         width = iconWidth + labelWidth;

      }

      cp->width = width;
      cp->height = height;

   }

}

/** Initialize a button tray component. */
void Create(TrayComponentType *cp)
{

   TrayButtonType *bp;
   TrayButtonActionType *ap;

   bp = (TrayButtonType*)cp->object;

   /* Validate the action(s) for this tray button. */
   for(ap = bp->actions; ap; ap = ap->next) {
      if(ap->action && strlen(ap->action) > 0) {
         if(!strncmp(ap->action, "exec:", 5)) {
            /* Valid. */
         } else if(!strncmp(ap->action, "root:", 5)) {
            /* Valid. However, the specified root menu may not exist.
             * This case is handled in ValidateTrayButtons.
             */
         } else if(!strcmp(ap->action, "showdesktop")) {
            /* Valid. */
         } else {
            Warning(_("invalid TrayButton action: \"%s\""), ap->action);
         }
      } else {
         /* Valid. However, root menu 1 may not exist.
          * This case is handled in ValidateTrayButtons.
          */
      }
   }

   cp->pixmap = JXCreatePixmap(display, rootWindow,
                               cp->width, cp->height, rootVisual.depth);

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
   ResetButton(&button, cp->pixmap, &rootVisual);
   if(active) {
      button.type = BUTTON_TRAY_ACTIVE;
   } else {
      button.type = BUTTON_TRAY;
   }
   button.width = cp->width;
   button.height = cp->height;
   button.x = 0;
   button.y = 0;
   button.font = FONT_TRAYBUTTON;
   button.text = bp->label;
   button.icon = bp->icon;
   DrawButton(&button);

}

/** Process a button press. */
void ProcessButtonPress(TrayComponentType *cp, int x, int y, int button)
{

   const TrayButtonType *bp = (TrayButtonType*)cp->object;
   const ScreenType *sp;
   const TrayButtonActionType *ap;
   const int mask = 1 << button;
   int mwidth, mheight;
   int menu;

   menu = -1;
   for(ap = bp->actions; ap; ap = ap->next) {
      if(ap->mask & mask) {
         if(ap->action && ap->action[0]) {
            if(strncmp(ap->action, "root:", 5)) {
               GrabMouse(cp->tray->window);
               cp->grabbed = 1;
               Draw(cp, 1);
               UpdateSpecificTray(cp->tray, cp);
               return;
            } else {
               menu = atoi(ap->action + 5);
            }
         } else {
            menu = 1;
         }
         break;
      }
   }
   if(menu < 0) {
      return;
   }

   GetRootMenuSize(menu, &mwidth, &mheight);
   sp = GetCurrentScreen(cp->screenx, cp->screeny);
   if(cp->tray->layout == LAYOUT_HORIZONTAL) {
      x = cp->screenx;
      if(cp->screeny + cp->height / 2 < sp->y + sp->height / 2) {
         y = cp->screeny + cp->height;
      } else {
         y = cp->screeny - mheight;
      }
   } else {
      y = cp->screeny;
      if(cp->screenx + cp->width / 2 < sp->x + sp->width / 2) {
         x = cp->screenx + cp->width;
      } else {
         x = cp->screenx - mwidth;
      }
   }

   Draw(cp, 1);
   UpdateSpecificTray(cp->tray, cp);
   ShowRootMenu(menu, x, y);
   Draw(cp, 0);
   UpdateSpecificTray(cp->tray, cp);

}

/** Process a button release. */
void ProcessButtonRelease(TrayComponentType *cp, int x, int y, int button)
{

   const TrayButtonType *bp = (TrayButtonType*)cp->object;
   const TrayButtonActionType *ap;
   const int mask = 1 << button;

   Draw(cp, 0);
   UpdateSpecificTray(cp->tray, cp);

   // Since we grab the mouse, make sure the mouse is actually
   // over the button.
   if(x < 0 || x >= cp->width) {
      return;
   }
   if(y < 0 || y >= cp->height) {
      return;
   }

   // Run the tray button action (if any).
   for(ap = bp->actions; ap; ap = ap->next) {
      if(ap->mask & mask) {
         if(ap->action && strlen(ap->action) > 0) {
            if(!strncmp(ap->action, "exec:", 5)) {
               RunCommand(ap->action + 5);
            } else if(!strcmp(ap->action, "showdesktop")) {
               ShowDesktop();
            }
         }
         return;
      }
   }
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
void SignalTrayButton(const TimeType *now, int x, int y, Window w, void *data)
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
   if(bp->cp->tray->window == w &&
      abs(bp->mousex - x) < settings.doubleClickDelta &&
      abs(bp->mousey - y) < settings.doubleClickDelta) {
      if(GetTimeDifference(now, &bp->mouseTime) >= settings.popupDelay) {
         ShowPopup(x, y, popup);
      }
   }
}

/** Validate tray buttons. */
void ValidateTrayButtons(void)
{
   const TrayButtonType *bp;
   for(bp = buttons; bp; bp = bp->next) {
      const TrayButtonActionType *ap;
      for(ap = bp->actions; ap; ap = ap->next) {
         if(ap->action && !strncmp(ap->action, "root:", 5)) {
            const int bindex = atoi(ap->action + 5);
            if(JUNLIKELY(!IsRootMenuDefined(bindex))) {
               Warning(_("tray button: root menu %d not defined"), bindex);
            }
         }
      }
   }
}

