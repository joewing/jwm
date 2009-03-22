/**
 * @file confirm.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Confirm dialog functions.
 *
 */

#include "jwm.h"
#include "confirm.h"
#include "client.h"
#include "main.h"
#include "font.h"
#include "button.h"
#include "screen.h"
#include "color.h"
#include "misc.h"

#ifndef DISABLE_CONFIRM

typedef enum {
   DBS_NORMAL   = 0,
   DBS_OK      = 1,
   DBS_CANCEL   = 2
} DialogButtonState;

typedef struct DialogType {

   int x, y;
   int width, height;
   int lineHeight;

   int okx;
   int cancelx;
   int buttony;
   int buttonWidth, buttonHeight;
   DialogButtonState buttonState;

   int lineCount;
   char **message;

   ClientNode *node;

   void (*action)(ClientNode*);
   ClientNode *client;

   struct DialogType *prev;
   struct DialogType *next;

} DialogType;

static const char *OK_STRING = "Ok";
static const char *CANCEL_STRING = "Cancel";

static DialogType *dialogList = NULL;

static int minWidth = 0;

static void DrawConfirmDialog(DialogType *d);
static void DestroyConfirmDialog(DialogType *d);
static void ComputeDimensions(DialogType *d);
static void DrawMessage(DialogType *d);
static void DrawButtons(DialogType *d);
static DialogType *FindDialogByWindow(Window w);
static int HandleDialogExpose(const XExposeEvent *event); 
static int HandleDialogButtonPress(const XButtonEvent *event);
static int HandleDialogButtonRelease(const XButtonEvent *event);

/** Initialize the dialog processing data. */
void InitializeDialogs() {
}

/** Startup dialog processing. */
void StartupDialogs() {
}

/** Stop dialog processing. */
void ShutdownDialogs() {

   while(dialogList) {
      DestroyConfirmDialog(dialogList);
   }

}

/** Destroy dialog processing data. */
void DestroyDialogs() {
}

/** Handle an event on a dialog window. */
int ProcessDialogEvent(const XEvent *event) {

   int handled = 0;

   Assert(event);

   switch(event->type) {
   case Expose:
      return HandleDialogExpose(&event->xexpose);
   case ButtonPress:
      return HandleDialogButtonPress(&event->xbutton);
   case ButtonRelease:
      return HandleDialogButtonRelease(&event->xbutton);
   default:
      break;
   }

   return handled;

}

/** Handle an expose event. */
int HandleDialogExpose(const XExposeEvent *event) {

   DialogType *dp;

   Assert(event);

   dp = FindDialogByWindow(event->window);
   if(dp) {
      DrawConfirmDialog(dp);
      return 1;
   } else {
      return 0;
   }
}

/** Handle a mouse button release event. */
int HandleDialogButtonRelease(const XButtonEvent *event) {

   DialogType *dp;
   int x, y;
   int cancelPressed, okPressed;

   Assert(event);

   dp = FindDialogByWindow(event->window);
   if(dp) {
      cancelPressed = 0;
      okPressed = 0;
      y = event->y;
      if(y >= dp->buttony && y < dp->buttony + dp->buttonHeight) {
         x = event->x;
         if(x >= dp->okx && x < dp->okx + dp->buttonWidth) {
            okPressed = 1;
         } else if(x >= dp->cancelx && x < dp->cancelx + dp->buttonWidth) {
            cancelPressed = 1;
         }
      }

      if(okPressed) {
         (dp->action)(dp->client);
      }

      if(cancelPressed || okPressed) {
         DestroyConfirmDialog(dp);
      } else {
         dp->buttonState = DBS_NORMAL;
         DrawButtons(dp);
      }

      return 1;
   } else {

      for(dp = dialogList; dp; dp = dp->next) {
         if(dp->buttonState != DBS_NORMAL) {
            dp->buttonState = DBS_NORMAL;
            DrawButtons(dp);
         }
      }

      return 0;

   }

}

/** Handle a mouse button release event. */
int HandleDialogButtonPress(const XButtonEvent *event) {

   DialogType *dp;
   int cancelPressed;
   int okPressed;
   int x, y;

   Assert(event);

   /* Find the dialog on which the press occured (if any). */
   dp = FindDialogByWindow(event->window);
   if(dp) {

      /* Determine which button was pressed (if any). */
      cancelPressed = 0;
      okPressed = 0;
      y = event->y;
      if(y >= dp->buttony && y < dp->buttony + dp->buttonHeight) {
         x = event->x;
         if(x >= dp->okx && x < dp->okx + dp->buttonWidth) {
            okPressed = 1;
         } else if(x >= dp->cancelx && x < dp->cancelx + dp->buttonWidth) {
            cancelPressed = 1;
         }
      }

      dp->buttonState = DBS_NORMAL;
      if(cancelPressed) {
         dp->buttonState = DBS_CANCEL;
      }

      if(okPressed) {
         dp->buttonState = DBS_OK;
      }

      /* Draw the buttons. */
      DrawButtons(dp);

      return 1;

   } else {

      /* This event doesn't affect us. */
      return 0;

   }

}

/** Find a dialog by window or frame. */
DialogType *FindDialogByWindow(Window w) {

   DialogType *dp;

   for(dp = dialogList; dp; dp = dp->next) {
      if(dp->node->window == w) {
         return dp;
      }
   }

   return NULL;

}

/** Show a confirm dialog. */
void ShowConfirmDialog(ClientNode *np, void (*action)(ClientNode*), ...) {

   va_list ap;
   DialogType *dp;
   XSetWindowAttributes attrs;
   XSizeHints shints;
   Window window;
   char *str;
   int x;

   Assert(action);

   dp = Allocate(sizeof(DialogType));
   dp->client = np;
   dp->action = action;
   dp->buttonState = DBS_NORMAL;

   dp->prev = NULL;
   dp->next = dialogList;
   if(dialogList) {
      dialogList->prev = dp;
   }
   dialogList = dp;

   /* Get the number of lines. */
   va_start(ap, action);
   for(dp->lineCount = 0; va_arg(ap, char*); dp->lineCount++);
   va_end(ap);

   dp->message = Allocate(dp->lineCount * sizeof(char*));
   va_start(ap, action);
   for(x = 0; x < dp->lineCount; x++) {
      str = va_arg(ap, char*);
      dp->message[x] = CopyString(str);
   }
   va_end(ap);

   ComputeDimensions(dp);

   attrs.background_pixel = colors[COLOR_TRAY_BG];
   attrs.event_mask = ButtonReleaseMask | ExposureMask;

   window = JXCreateWindow(display, rootWindow,
      dp->x, dp->y, dp->width, dp->height, 0,
      CopyFromParent, InputOutput, CopyFromParent,
      CWBackPixel | CWEventMask, &attrs);

   shints.x = dp->x;
   shints.y = dp->y;
   shints.flags = PPosition;
   JXSetWMNormalHints(display, window, &shints);

   JXStoreName(display, window, "Confirm");

   dp->node = AddClientWindow(window, 0, 0);
   Assert(dp->node);
   if(np) {
      dp->node->owner = np->window;
   }
   dp->node->state.status |= STAT_WMDIALOG;
   FocusClient(dp->node);

   DrawConfirmDialog(dp);

   JXGrabButton(display, AnyButton, AnyModifier, window,
      True, ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);

}

/** Draw a confirm dialog. */
void DrawConfirmDialog(DialogType *dp) {

   Assert(dp);

   DrawMessage(dp);
   DrawButtons(dp);

}

/** Destroy a confirm dialog. */
void DestroyConfirmDialog(DialogType *dp) {

   int x;

   Assert(dp);

   /* This will take care of destroying the dialog window since
    * its parent will be destroyed. */
   RemoveClient(dp->node);

   for(x = 0; x < dp->lineCount; x++) {
      Release(dp->message[x]);
   }
   Release(dp->message);

   if(dp->next) {
      dp->next->prev = dp->prev;
   }
   if(dp->prev) {
      dp->prev->next = dp->next;
   } else {
      dialogList = dp->next;
   }
   Release(dp);

}

/** Compute the size of a dialog window. */
void ComputeDimensions(DialogType *dp) {

   const ScreenType *sp;
   int width;
   int x;

   Assert(dp);

   /* Get the min width from the size of the buttons. */
   if(!minWidth) {
      minWidth = GetStringWidth(FONT_MENU, CANCEL_STRING) * 3;
      width = GetStringWidth(FONT_MENU, OK_STRING) * 3;
      if(width > minWidth) {
         minWidth = width;
      }
      minWidth += 16 * 3;
   }
   dp->width = minWidth;

   /* Take into account the size of the message. */
   for(x = 0; x < dp->lineCount; x++) {
      width = GetStringWidth(FONT_MENU, dp->message[x]);
      if(width > dp->width) {
         dp->width = width;
      }
   }
   dp->lineHeight = GetStringHeight(FONT_MENU);
   dp->width += 8;
   dp->height = (dp->lineCount + 2) * dp->lineHeight;

   if(dp->client) {

      dp->x = dp->client->x + dp->client->width / 2 - dp->width / 2;
      dp->y = dp->client->y + dp->client->height / 2 - dp->height / 2;

      if(dp->x < 0) {
         dp->x = 0;
      }
      if(dp->y < 0) {
         dp->y = 0;
      }
      if(dp->x + dp->width >= rootWidth) {
         dp->x = rootWidth - dp->width - (borderWidth * 2);
      }
      if(dp->y + dp->height >= rootHeight) {
         dp->y = rootHeight - dp->height - (borderWidth * 2 + titleHeight);
      }

   } else {

      sp = GetMouseScreen();

      dp->x = sp->width / 2 - dp->width / 2 + sp->x;
      dp->y = sp->height / 2 - dp->height / 2 + sp->y;

   }

}

/** Display the message on the dialog window. */
void DrawMessage(DialogType *dp) {

   int yoffset;
   int x;

   Assert(dp);

   yoffset = 4;
   for(x = 0; x < dp->lineCount; x++) {
      RenderString(dp->node->window, FONT_MENU, COLOR_TRAY_FG,
         4, yoffset, dp->width, NULL, dp->message[x]);
      yoffset += dp->lineHeight;
   }

}

/** Draw the buttons on the dialog window. */
void DrawButtons(DialogType *dp) {

   ButtonNode button;
   int temp;

   Assert(dp);

   dp->buttonWidth = GetStringWidth(FONT_MENU, CANCEL_STRING);
   temp = GetStringWidth(FONT_MENU, OK_STRING);
   if(temp > dp->buttonWidth) {
      dp->buttonWidth = temp;
   }
   dp->buttonWidth += 16;
   dp->buttonHeight = dp->lineHeight + 4;

   ResetButton(&button, dp->node->window, rootGC);
   button.font = FONT_MENU;
   button.width = dp->buttonWidth;
   button.height = dp->buttonHeight;
   button.alignment = ALIGN_CENTER;

   dp->okx = dp->width / 3 - dp->buttonWidth / 2;
   dp->cancelx = 2 * dp->width / 3 - dp->buttonWidth / 2;
   dp->buttony = dp->height - dp->lineHeight - dp->lineHeight / 2;

   if(dp->buttonState == DBS_OK) {
      button.type = BUTTON_TASK_ACTIVE;
   } else {
      button.type = BUTTON_TASK;
   }
   button.text = OK_STRING;
   button.x = dp->okx;
   button.y = dp->buttony;
   DrawButton(&button);

   if(dp->buttonState == DBS_CANCEL) {
      button.type = BUTTON_TASK_ACTIVE;
   } else {
      button.type = BUTTON_TASK;
   }
   button.text = CANCEL_STRING;
   button.x = dp->cancelx;
   button.y = dp->buttony;
   DrawButton(&button);

}

#else /* DISABLE_CONFIRM */

/** Process an event on a dialog window. */
int ProcessDialogEvent(const XEvent *event) {
   return 0;
}

/** Show a confirm dialog. */
void ShowConfirmDialog(ClientNode *np, void (*action)(ClientNode*), ...) {

   Assert(action);

   (action)(np);

}

#endif /* DISABLE_CONFIRM */

