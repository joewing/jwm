/**
 * @file binding.c
 * @author Joe Wingbermuehle
 * @date 2013
 *
 * @brief Binding functions that are shared between key and mouse bindings.
 *
 */

#include "jwm.h"
#include "binding.h"
#include "client.h"
#include "clientlist.h"
#include "root.h"
#include "command.h"
#include "main.h"
#include "desktop.h"
#include "tray.h"
#include "taskbar.h"
#include "winmenu.h"

/** Initialize action context. */
void InitActionContext(ActionContext *ac)
{
   ac->client = NULL;
   ac->data = NULL;
   ac->desktop = currentDesktop;
   ac->x = 0;
   ac->y = 0;
   ac->MoveFunc = NULL;
   ac->ResizeFunc = NULL;
}

/** Run an action. */
char RunAction(const ActionContext *context,
               const ActionNode *action)
{
   char value;
   switch(action->type) {
   case ACTION_EXEC:
      RunCommand(action->arg);
      return 0;
   case ACTION_DESKTOP:
      if(action->arg) {
         ChangeDesktop((unsigned int)atoi(action->arg));
      } else {
         ChangeDesktop(context->desktop);
      }
      return 0;
   case ACTION_RDESKTOP:
      RightDesktop();
      return 0;
   case ACTION_LDESKTOP:
      LeftDesktop();
      return 0;
   case ACTION_UDESKTOP:
      AboveDesktop();
      return 0;
   case ACTION_DDESKTOP:
      BelowDesktop();
      return 0;
   case ACTION_SHOWDESK:
      ShowDesktop();
      return 0;
   case ACTION_SHOWTRAY:
      ShowAllTrays();
      return 0;
   case ACTION_NEXT:
      FocusNext();
      return 0;
   case ACTION_NEXTSTACK:
      StartWindowStackWalk();
      WalkWindowStack(1);
      return 0;
   case ACTION_PREV:
      FocusPrevious();
      return 0;
   case ACTION_PREVSTACK:
      StartWindowStackWalk();
      WalkWindowStack(0);
      return 0;
   case ACTION_CLOSE:
      if(context->client) {
         DeleteClient(context->client);
      }
      return 0;
   case ACTION_KILL:
      if(context->client) {
         KillClient(context->client);
      }
      return 0;
   case ACTION_SHADE:
      if(context->client) {
         if(action->arg) {
            value = action->arg[0] == '1';
         } else {
            value = !(context->client->state.status & STAT_SHADED);
         }
         if(value) {
            ShadeClient(context->client);
         } else {
            UnshadeClient(context->client);
         }
      }
      return 0;
   case ACTION_STICK:
      if(context->client) {
         if(action->arg) {
            value = action->arg[0] == '1';
         } else {
            value = !(context->client->state.status & STAT_STICKY);
         }
         SetClientSticky(context->client, value);
      }
      return 0;
   case ACTION_MOVE:
      if(context->client && context->MoveFunc) {
         (context->MoveFunc)(context, context->x - context->client->x,
                             context->y - context->client->y, 1);
      }
      return 1;
   case ACTION_RESIZE:
      if(context->client && context->ResizeFunc) {
         (context->ResizeFunc)(context, context->x, context->y);
      }
      return 1;
   case ACTION_MIN:
      if(context->client) {
         if(action->arg) {
            value = action->arg[0] == '1';
         } else {
            value = !(context->client->state.status & STAT_MINIMIZED);
         }
         MinimizeClient(context->client, value);
      }
      return 0;
   case ACTION_MAX:
      if(context->client) {
         MaximizeClientDefault(context->client);
      }
      return 0;
   case ACTION_ROOT:
      if(JLIKELY(action->arg)) {
         ShowRootMenu((unsigned int)atoi(action->arg),
                      context->x, context->y);
         return 1;
      } else {
         return 0;
      }
   case ACTION_WIN:
      if(context->client) {
         ShowWindowMenu(context->client, context->x, context->y);
         return 1;
      } else {
         return 0;
      }
   case ACTION_RESTART:
      Restart();
      return 0;
   case ACTION_EXIT:
      Exit();
      return 0;
   case ACTION_FULLSCREEN:
      if(context->client) {
         if(action->arg) {
            value = action->arg[0] == '1';
         } else {
            value = !(context->client->state.status & STAT_FULLSCREEN);
         }
         SetClientFullScreen(context->client, value);
      }
      return 0;
   case ACTION_SENDTO:
      if(context->client && action->arg) {
         SetClientDesktop(context->client, atoi(action->arg));
      }
      return 0;
   case ACTION_SENDLEFT:
      if(context->client) {
         SetClientDesktop(context->client, GetLeftDesktop());
      }
      return 0;
   case ACTION_SENDRIGHT:
      if(context->client) {
         SetClientDesktop(context->client, GetRightDesktop());
      }
      return 0;
   case ACTION_SENDUP:
      if(context->client) {
         SetClientDesktop(context->client, GetAboveDesktop());
      }
      return 0;
   case ACTION_SENDDOWN:
      if(context->client) {
         SetClientDesktop(context->client, GetBelowDesktop());
      }
      return 0;
   case ACTION_LAYER:
      if(context->client && action->arg) {
         SetClientLayer(context->client, atoi(action->arg));
      }
      return 0;
   default:
      return 0;
   }
}

