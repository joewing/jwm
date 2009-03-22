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

#define BUTTON_SIZE 4

typedef struct TrayButtonType {

   TrayComponentType *cp;

   char *label;
   char *popup;
   char *iconName;
   IconNode *icon;

   char *action;

   int mousex;
   int mousey;
   TimeType mouseTime;

   struct TrayButtonType *next;

} TrayButtonType;

static TrayButtonType *buttons;

static void CheckedCreate(TrayComponentType *cp);
static void Create(TrayComponentType *cp);
static void Destroy(TrayComponentType *cp);
static void SetSize(TrayComponentType *cp, int width, int height);
static void Resize(TrayComponentType *cp);
static void Draw(TrayComponentType *cp, int active);

static void ProcessButtonPress(TrayComponentType *cp,
   int x, int y, int mask);
static void ProcessButtonRelease(TrayComponentType *cp,
   int x, int y, int mask);
static void ProcessMotionEvent(TrayComponentType *cp,
   int x, int y, int mask);

/** Initialize tray button data. */
void InitializeTrayButtons() {
   buttons = NULL;
}

/** Startup tray buttons. */
void StartupTrayButtons() {

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
         bp->icon = LoadNamedIcon(bp->iconName);
         if(bp->icon) {
            bp->cp->requestedWidth += bp->icon->image->width;
            bp->cp->requestedHeight += bp->icon->image->height;
         } else {
            Warning("could not load tray icon: \"%s\"", bp->iconName);
         }
      }
      bp->cp->requestedWidth += 2 * BUTTON_SIZE;
      bp->cp->requestedHeight += 2 * BUTTON_SIZE;
   }

}

/** Shutdown tray buttons. */
void ShutdownTrayButtons() {

}

/** Release tray button data. */
void DestroyTrayButtons() {

   TrayButtonType *bp;

   while(buttons) {
      bp = buttons->next;
      if(buttons->label) {
         Release(buttons->label);
      }
      if(buttons->iconName) {
         Release(buttons->iconName);
      }
      if(buttons->action) {
         Release(buttons->action);
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
   const char *label, const char *action,
   const char *popup, int width, int height) {

   TrayButtonType *bp;
   TrayComponentType *cp;

   if((label == NULL || strlen(label) == 0)
      && (iconName == NULL || strlen(iconName) == 0)) {
      Warning("no icon or label for TrayButton");
      return NULL;
   }

   if(width < 0) {
      Warning("invalid TrayButton width: %d", width);
      width = 0;
   }
   if(height < 0) {
      Warning("invalid TrayButton height: %d", height);
      height = 0;
   }

   bp = Allocate(sizeof(TrayButtonType));
   bp->next = buttons;
   buttons = bp;

   bp->icon = NULL;
   bp->iconName = CopyString(iconName);
   bp->label = CopyString(label);
   bp->action = CopyString(action);
   bp->popup = CopyString(popup);

   cp = CreateTrayComponent();
   cp->object = bp;
   bp->cp = cp;
   cp->requestedWidth = width;
   cp->requestedHeight = height;

   bp->mousex = 0;
   bp->mousey = 0;

   cp->Create = CheckedCreate;
   cp->Destroy = Destroy;
   cp->SetSize = SetSize;
   cp->Resize = Resize;

   cp->ProcessButtonPress = ProcessButtonPress;
   cp->ProcessButtonRelease = ProcessButtonRelease;
   if(popup || label) {
      cp->ProcessMotionEvent = ProcessMotionEvent;
   }

   return cp;

}

/** Set the size of a button tray component. */
void SetSize(TrayComponentType *cp, int width, int height) {

   TrayButtonType *bp;
   int labelWidth, labelHeight;
   int iconWidth, iconHeight;
   double ratio;

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
      ratio = (double)iconWidth / iconHeight;

      if(width > 0) {

         /* Compute height from width. */
         iconWidth = width - labelWidth - 2 * BUTTON_SIZE;
         iconHeight = iconWidth / ratio;
         height = Max(iconHeight, labelHeight) + 2 * BUTTON_SIZE;

      } else if(height > 0) {

         /* Compute width from height. */
         iconHeight = height - 2 * BUTTON_SIZE;
         iconWidth = iconHeight * ratio;
         width = iconWidth + labelWidth + 2 * BUTTON_SIZE;

      }

      cp->width = width;
      cp->height = height;

   }

}

/** Initialize a button tray component (display errors). */
void CheckedCreate(TrayComponentType *cp) {

   TrayButtonType *bp;

   bp = (TrayButtonType*)cp->object;

   /* Validate the action for this tray button. */
   if(bp->action && strlen(bp->action) > 0) {
      if(!strncmp(bp->action, "exec:", 5)) {
         /* Valid. */
      } else if(!strncmp(bp->action, "root:", 5)) {
         /* Valid. However, the specified root menu may not exist.
          * This case is handled in ValidateTrayButtons.
          */
      } else if(!strcmp(bp->action, "showdesktop")) {
         /* Valid. */
      } else {
         Warning("invalid TrayButton action: \"%s\"", bp->action);
      }
   } else {
      /* Valid. However, root menu 1 may not exist.
       * This case is handled in ValidateTrayButtons.
       */
   }

   Create(cp);

}

/** Initialize a button tray component. */
void Create(TrayComponentType *cp) {

   cp->pixmap = JXCreatePixmap(display, rootWindow,
      cp->width, cp->height, rootDepth);

   Draw(cp, 0);

}

/** Resize a button tray component. */
void Resize(TrayComponentType *cp) {

   Destroy(cp);
   Create(cp);

}

/** Destroy a button tray component. */
void Destroy(TrayComponentType *cp) {
   if(cp->pixmap != None) {
      JXFreePixmap(display, cp->pixmap);
   }
}

/** Draw a tray button. */
void Draw(TrayComponentType *cp, int active) {

   ButtonNode button;
   TrayButtonType *bp;
   int labelx;

   bp = (TrayButtonType*)cp->object;

   JXSetForeground(display, rootGC, colors[COLOR_TRAYBUTTON_BG]);
   JXFillRectangle(display, cp->pixmap, rootGC, 0, 0, cp->width, cp->height);

   ResetButton(&button, cp->pixmap, rootGC);
   if(active) {
      button.type = BUTTON_TASK_ACTIVE;
   } else {
      button.type = BUTTON_TASK;
   }
   button.width = cp->width - 3;
   button.height = cp->height - 3;
   button.x = 1;
   button.y = 1;
   DrawButton(&button);

   /* Compute the offset of the text. */
   if(bp->label) {
      if(!bp->icon) {
         labelx = 2 + cp->width / 2;
         labelx -= GetStringWidth(FONT_TRAYBUTTON, bp->label) / 2;
      } else {
         labelx = cp->width;
         labelx -= GetStringWidth(FONT_TRAYBUTTON, bp->label) + 4;
      }
   } else {
      labelx = cp->width;
   }
   labelx -= BUTTON_SIZE;

   if(bp->icon) {
      PutIcon(bp->icon, cp->pixmap, BUTTON_SIZE, BUTTON_SIZE,
         labelx - BUTTON_SIZE, cp->height - BUTTON_SIZE * 2);
   }

   if(bp->label) {
      RenderString(cp->pixmap, FONT_TRAYBUTTON, COLOR_TRAYBUTTON_FG,
         labelx + 2, cp->height / 2 - GetStringHeight(FONT_TRAYBUTTON) / 2,
         cp->width - labelx, NULL, bp->label);
   }

}

/** Process a button press. */
void ProcessButtonPress(TrayComponentType *cp, int x, int y, int mask) {

   const ScreenType *sp;
   int mwidth, mheight;
   int button;

   TrayButtonType *bp = (TrayButtonType*)cp->object;

   Assert(bp);

   if(bp->action && strlen(bp->action) > 0) {
      if(strncmp(bp->action, "root:", 5)) {
         GrabMouse(cp->tray->window);
         cp->grabbed = 1;
         Draw(cp, 1);
         UpdateSpecificTray(cp->tray, cp);
         return;
      } else {
         button = atoi(bp->action + 5);
      }
   } else {
      button = 1;
   }

   GetRootMenuSize(button, &mwidth, &mheight);

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
   ShowRootMenu(button, x, y);
   Draw(cp, 0);
   UpdateSpecificTray(cp->tray, cp);

}

/** Process a button release. */
void ProcessButtonRelease(TrayComponentType *cp, int x, int y, int mask) {

   TrayButtonType *bp = (TrayButtonType*)cp->object;

   Assert(bp);

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
   if(bp->action && strlen(bp->action) > 0) {
      if(!strncmp(bp->action, "exec:", 5)) {
         RunCommand(bp->action + 5);
         return;
      } else if(!strcmp(bp->action, "showdesktop")) {
         ShowDesktop();
         return;
      }
   }

}

/** Process a motion event. */
void ProcessMotionEvent(TrayComponentType *cp, int x, int y, int mask) {

   TrayButtonType *bp = (TrayButtonType*)cp->object;

   bp->mousex = cp->screenx + x;
   bp->mousey = cp->screeny + y;
   GetCurrentTime(&bp->mouseTime);

}

/** Signal (needed for popups). */
void SignalTrayButton(const TimeType *now, int x, int y) {

   TrayButtonType *bp;
   const char *popup;

   for(bp = buttons; bp; bp = bp->next) {
      if(bp->popup) {
         popup = bp->popup;
      } else if(bp->label) {
         popup = bp->label;
      } else {
         continue;
      }
      if(abs(bp->mousex - x) < POPUP_DELTA
         && abs(bp->mousey - y) < POPUP_DELTA) {
         if(GetTimeDifference(now, &bp->mouseTime) >= popupDelay) {
            ShowPopup(x, y, popup);
         }
      }
   }

}

/** Validate tray buttons. */
void ValidateTrayButtons() {

   TrayButtonType *bp;
   int bindex;

   for(bp = buttons; bp; bp = bp->next) {
      if(bp->action && !strncmp(bp->action, "root:", 5)) {
         bindex = atoi(bp->action + 5);
         if(!IsRootMenuDefined(bindex)) {
            Warning("tray button: root menu %d not defined", bindex);
         }
      }
   }

}

