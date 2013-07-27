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

/** Run an action. */
char RunAction(ActionType action,
               const char *arg,
               const ActionDataType *data)
{
   char value;
   switch(action) {
   case ACTION_EXEC:
      RunCommand(arg);
      return 0;
   case ACTION_DESKTOP:
      if(arg) {
         ChangeDesktop((unsigned int)atoi(arg));
      } else {
         ChangeDesktop(data->desktop);
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
      if(data->client) {
         DeleteClient(data->client);
      }
      return 0;
   case ACTION_SHADE:
      if(data->client) {
         if(arg) {
            value = arg[0] == '1';
         } else {
            value = !(data->client->state.status & STAT_SHADED);
         }
         if(value) {
            ShadeClient(data->client);
         } else {
            UnshadeClient(data->client);
         }
      }
      return 0;
   case ACTION_STICK:
      if(data->client) {
         if(arg) {
            value = arg[0] == '1';
         } else {
            value = !(data->client->state.status & STAT_STICKY);
         }
         SetClientSticky(data->client, value);
      }
      return 0;
   case ACTION_MOVE:
      if(data->client && data->MoveFunc) {
         (data->MoveFunc)(data, data->x - data->client->x,
                          data->y - data->client->y, 1);
      }
      return 1;
   case ACTION_RESIZE:
      if(data->client && data->ResizeFunc) {
         (data->ResizeFunc)(data, data->x, data->y);
      }
      return 1;
   case ACTION_MIN:
      if(data->client) {
         if(arg) {
            value = arg[0] == '1';
         } else {
            value = !(data->client->state.status & STAT_MINIMIZED);
         }
         MinimizeClient(data->client, value);
      }
      return 0;
   case ACTION_MAX:
      if(data->client) {
         MaximizeClientDefault(data->client);
      }
      return 0;
   case ACTION_ROOT:
      if(JLIKELY(arg)) {
         ShowRootMenu((unsigned int)atoi(arg), data->x, data->y);
         return 1;
      } else {
         return 0;
      }
   case ACTION_WIN:
      if(data->client) {
         ShowWindowMenu(data->client, data->x, data->y);
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
      if(data->client) {
         if(arg) {
            value = arg[0] == '1';
         } else {
            value = !(data->client->state.status & STAT_FULLSCREEN);
         }
         SetClientFullScreen(data->client, value);
      }
      return 0;
   case ACTION_SENDTO:
      if(data->client && arg) {
         SetClientDesktop(data->client, atoi(arg));
      }
      return 0;
   case ACTION_SENDLEFT:
      if(data->client) {
         SetClientDesktop(data->client, GetLeftDesktop());
      }
      return 0;
   case ACTION_SENDRIGHT:
      if(data->client) {
         SetClientDesktop(data->client, GetRightDesktop());
      }
      return 0;
   case ACTION_SENDUP:
      if(data->client) {
         SetClientDesktop(data->client, GetAboveDesktop());
      }
      return 0;
   case ACTION_SENDDOWN:
      if(data->client) {
         SetClientDesktop(data->client, GetBelowDesktop());
      }
      return 0;
   default:
      return 0;
   }
}

