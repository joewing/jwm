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
#include "settings.h"
#include "key.h"

#ifndef DISABLE_CONFIRM

typedef enum {
   DBS_NORMAL   = 0,
   DBS_OK      = 1,
   DBS_CANCEL   = 2
} DialogButtonState;

typedef struct {

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

} DialogType;

static const char *OK_STRING = "Ok";
static const char *CANCEL_STRING = "Cancel";

static DialogType *dialog = NULL;

static int minWidth = 0;

static void DrawConfirmDialog();
static void DestroyConfirmDialog();
static void ComputeDimensions();
static void DrawMessage();
static void DrawButtons();
static char HandleDialogExpose(const XExposeEvent *event); 
static char HandleDialogButtonPress(const XButtonEvent *event);
static char HandleDialogButtonRelease(const XButtonEvent *event);
static char HandleDialogKeyPress(const XKeyEvent *event);

/** Initialize the dialog processing data. */
void InitializeDialogs()
{
}

/** Startup dialog processing. */
void StartupDialogs()
{
}

/** Stop dialog processing. */
void ShutdownDialogs()
{
   if(dialog) {
      DestroyConfirmDialog();
      dialog = NULL;
   }
}

/** Destroy dialog processing data. */
void DestroyDialogs()
{
}

/** Handle an event on a dialog window. */
char ProcessDialogEvent(const XEvent *event)
{

   Assert(event);

   switch(event->type) {
   case Expose:
      return HandleDialogExpose(&event->xexpose);
   case ButtonPress:
      return HandleDialogButtonPress(&event->xbutton);
   case ButtonRelease:
      return HandleDialogButtonRelease(&event->xbutton);
   case KeyPress:
      return HandleDialogKeyPress(&event->xkey);
   default:
      return 0;
   }

}

/** Handle an expose event. */
char HandleDialogExpose(const XExposeEvent *event)
{
   Assert(event);
   if(dialog && dialog->node->window == event->window) {
      DrawConfirmDialog();
      return 1;
   } else {
      return 0;
   }
}

/** Handle a mouse button release event. */
char HandleDialogButtonRelease(const XButtonEvent *event)
{

   int x, y;
   char cancelPressed, okPressed;

   Assert(event);

   if(dialog && event->window == dialog->node->window) {
      cancelPressed = 0;
      okPressed = 0;
      y = event->y;
      if(y >= dialog->buttony && y < dialog->buttony + dialog->buttonHeight) {
         x = event->x;
         if(x >= dialog->okx && x < dialog->okx + dialog->buttonWidth) {
            okPressed = 1;
         } else if(x >= dialog->cancelx
                  && x < dialog->cancelx + dialog->buttonWidth) {
            cancelPressed = 1;
         }
      }

      if(okPressed) {
         (dialog->action)(dialog->client);
      }

      if(cancelPressed || okPressed) {
         DestroyConfirmDialog();
      } else {
         dialog->buttonState = DBS_NORMAL;
         DrawButtons();
      }

      return 1;
   } else {

      if(dialog) {
         if(dialog->buttonState != DBS_NORMAL) {
            dialog->buttonState = DBS_NORMAL;
            DrawButtons();
         }
      }

      return 0;

   }

}

/** Handle a mouse button release event. */
char HandleDialogButtonPress(const XButtonEvent *event)
{

   char cancelPressed;
   char okPressed;
   int x, y;

   Assert(event);

   /* Find the dialog on which the press occured (if any). */
   if(dialog && event->window == dialog->node->window) {

      /* Determine which button was pressed (if any). */
      cancelPressed = 0;
      okPressed = 0;
      y = event->y;
      if(y >= dialog->buttony && y < dialog->buttony + dialog->buttonHeight) {
         x = event->x;
         if(x >= dialog->okx && x < dialog->okx + dialog->buttonWidth) {
            okPressed = 1;
         } else if(x >= dialog->cancelx
                  && x < dialog->cancelx + dialog->buttonWidth) {
            cancelPressed = 1;
         }
      }

      dialog->buttonState = DBS_NORMAL;
      if(cancelPressed) {
         dialog->buttonState = DBS_CANCEL;
      }

      if(okPressed) {
         dialog->buttonState = DBS_OK;
      }

      /* Draw the buttons. */
      DrawButtons();

      return 1;

   } else {

      /* This event doesn't affect us. */
      return 0;

   }

}

/** Handle a key press. */
char HandleDialogKeyPress(const XKeyEvent *event)
{
   if(dialog && event->window == dialog->node->window) {
      KeyType key = GetKey(event);
      switch(key & 0xFF) {
      case KEY_ENTER:
         (dialog->action)(dialog->client);
         DestroyConfirmDialog();
         break;
      case KEY_ESC:
         DestroyConfirmDialog();
         break;
      default:
         break;
      }
      return 1;
   } else {
      return 0;
   }
}

/** Show a confirm dialog. */
void ShowConfirmDialog(ClientNode *np, void (*action)(ClientNode*), ...)
{

   va_list ap;
   XSetWindowAttributes attrs;
   XSizeHints shints;
   Window window;
   char *str;
   int x;

   Assert(action);

   /* Only allow one dialog at a time. */
   if(dialog) {
      DestroyConfirmDialog();
   }

   dialog = Allocate(sizeof(DialogType));
   dialog->client = np;
   dialog->action = action;
   dialog->buttonState = DBS_NORMAL;

   /* Get the number of lines. */
   va_start(ap, action);
   for(dialog->lineCount = 0; va_arg(ap, char*); dialog->lineCount++);
   va_end(ap);

   dialog->message = Allocate(dialog->lineCount * sizeof(char*));
   va_start(ap, action);
   for(x = 0; x < dialog->lineCount; x++) {
      str = va_arg(ap, char*);
      dialog->message[x] = CopyString(str);
   }
   va_end(ap);

   ComputeDimensions();

   attrs.background_pixel = colors[COLOR_MENU_BG];
   attrs.event_mask = ButtonPressMask
                    | ButtonReleaseMask
                    | KeyPressMask
                    | ExposureMask;

   window = JXCreateWindow(display, rootWindow,
                           dialog->x, dialog->y,
                           dialog->width, dialog->height, 0,
                           CopyFromParent, InputOutput, CopyFromParent,
                           CWBackPixel | CWEventMask, &attrs);

   shints.x = dialog->x;
   shints.y = dialog->y;
   shints.flags = PPosition;
   JXSetWMNormalHints(display, window, &shints);

   JXStoreName(display, window, "Confirm");

   dialog->node = AddClientWindow(window, 0, 0);
   Assert(dialog->node);
   if(np) {
      dialog->node->owner = np->window;
   }
   dialog->node->state.status |= STAT_WMDIALOG;
   FocusClient(dialog->node);

   JXGrabButton(display, AnyButton, AnyModifier, window, True,
                ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);

}

/** Draw a confirm dialog. */
void DrawConfirmDialog()
{
   Assert(dialog);
   DrawMessage();
   DrawButtons();
}

/** Destroy a confirm dialog. */
void DestroyConfirmDialog()
{

   int x;

   Assert(dialog);

   /* This will take care of destroying the dialog window since
    * its parent will be destroyed. */
   JXSync(display, True);
   RemoveClient(dialog->node);

   for(x = 0; x < dialog->lineCount; x++) {
      Release(dialog->message[x]);
   }
   Release(dialog->message);

   Release(dialog);
   dialog = NULL;

}

/** Compute the size of a dialog window. */
void ComputeDimensions()
{

   const ScreenType *sp;
   int width;
   int x;

   Assert(dialog);

   /* Get the min width from the size of the buttons. */
   if(!minWidth) {
      minWidth = GetStringWidth(FONT_MENU, CANCEL_STRING) * 3;
      width = GetStringWidth(FONT_MENU, OK_STRING) * 3;
      if(width > minWidth) {
         minWidth = width;
      }
      minWidth += 16 * 3;
   }
   dialog->width = minWidth;

   /* Take into account the size of the message. */
   for(x = 0; x < dialog->lineCount; x++) {
      width = GetStringWidth(FONT_MENU, dialog->message[x]);
      if(width > dialog->width) {
         dialog->width = width;
      }
   }
   dialog->lineHeight = GetStringHeight(FONT_MENU);
   dialog->width += 8;
   dialog->height = (dialog->lineCount + 2) * dialog->lineHeight;

   if(dialog->client) {

      dialog->x = dialog->client->x
                + (dialog->client->width - dialog->width) / 2;
      dialog->y = dialog->client->y
                + (dialog->client->height - dialog->height) / 2;

      if(dialog->x < 0) {
         dialog->x = 0;
      }
      if(dialog->y < 0) {
         dialog->y = 0;
      }
      if(dialog->x + dialog->width >= rootWidth) {
         dialog->x = rootWidth - dialog->width - (settings.borderWidth * 2);
      }
      if(dialog->y + dialog->height >= rootHeight) {
         dialog->y = rootHeight - dialog->height
               - (settings.borderWidth * 2 + settings.titleHeight);
      }

   } else {

      sp = GetMouseScreen();

      dialog->x = (sp->width - dialog->width) / 2 + sp->x;
      dialog->y = (sp->height - dialog->height) / 2 + sp->y;

   }

}

/** Display the message on the dialog window. */
void DrawMessage()
{

   int yoffset;
   int x;

   Assert(dialog);

   yoffset = 4;
   for(x = 0; x < dialog->lineCount; x++) {
      RenderString(dialog->node->window, FONT_MENU, COLOR_MENU_FG,
                   4, yoffset, dialog->width, dialog->message[x]);
      yoffset += dialog->lineHeight;
   }

}

/** Draw the buttons on the dialog window. */
void DrawButtons()
{

   ButtonNode button;
   int temp;

   Assert(dialog);

   dialog->buttonWidth = GetStringWidth(FONT_MENU, CANCEL_STRING);
   temp = GetStringWidth(FONT_MENU, OK_STRING);
   if(temp > dialog->buttonWidth) {
      dialog->buttonWidth = temp;
   }
   dialog->buttonWidth += 16;
   dialog->buttonHeight = dialog->lineHeight + 4;

   ResetButton(&button, dialog->node->window, rootGC);
   button.font = FONT_MENU;
   button.width = dialog->buttonWidth;
   button.height = dialog->buttonHeight;
   button.alignment = ALIGN_CENTER;

   dialog->okx = dialog->width / 3 - dialog->buttonWidth / 2;
   dialog->cancelx = 2 * dialog->width / 3 - dialog->buttonWidth / 2;
   dialog->buttony = dialog->height - dialog->lineHeight
                   - dialog->lineHeight / 2;

   if(dialog->buttonState == DBS_OK) {
      button.type = BUTTON_MENU_ACTIVE;
   } else {
      button.type = BUTTON_MENU;
   }
   button.text = OK_STRING;
   button.x = dialog->okx;
   button.y = dialog->buttony;
   DrawButton(&button);

   if(dialog->buttonState == DBS_CANCEL) {
      button.type = BUTTON_MENU_ACTIVE;
   } else {
      button.type = BUTTON_MENU;
   }
   button.text = CANCEL_STRING;
   button.x = dialog->cancelx;
   button.y = dialog->buttony;
   DrawButton(&button);

}

#else /* DISABLE_CONFIRM */

/** Process an event on a dialog window. */
char ProcessDialogEvent(const XEvent *event)
{
   return 0;
}

/** Show a confirm dialog. */
void ShowConfirmDialog(ClientNode *np, void (*action)(ClientNode*), ...)
{
   Assert(action);
   (action)(np);
}

#endif /* DISABLE_CONFIRM */

