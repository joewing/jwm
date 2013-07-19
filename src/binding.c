
#include "jwm.h"
#include "binding.h"
#include "client.h"
#include "clientlist.h"
#include "root.h"
#include "command.h"
#include "main.h"
#include "desktop.h"

void RunAction(ClientNode *np,
               int x, int y,
               ActionType action,
               const char *arg)
{
   char value;
   char useKeyboard = 0;
   if(np == NULL) {
      useKeyboard = 1;
      np = GetActiveClient();
   }
   switch(action) {
   case ACTION_EXEC:
      RunCommand(arg);
      break;
   case ACTION_DESKTOP:
      if(JLIKELY(arg)) {
         ChangeDesktop(arg[0]);
      }
      break;
   case ACTION_RDESKTOP:
      RightDesktop();
      break;
   case ACTION_LDESKTOP:
      LeftDesktop();
      break;
   case ACTION_UDESKTOP:
      AboveDesktop();
      break;
   case ACTION_DDESKTOP:
      BelowDesktop();
      break;
   case ACTION_SHOWDESK:
      ShowDesktop();
      break;
   case ACTION_SHOWTRAY:
      ShowAllTrays();
      break;
   case ACTION_NEXT:
      FocusNext();
      break;
   case ACTION_NEXTSTACK:
      StartWindowStackWalk();
      WalkWindowStack(1);
      break;
   case ACTION_PREV:
      FocusPrevious();
      break;
   case ACTION_PREVSTACK:
      StartWindowStackWalk();
      WalkWindowStack(0);
      break;
   case ACTION_CLOSE:
      if(np) {
         DeleteClient(np);
      }
      break;
   case ACTION_SHADE:
      if(np) {
         if(arg) {
            value = arg[0] == '1';
         } else {
            value = !(np->state.status & STAT_SHADED);
         }
         if(value) {
            ShadeClient(np);
         } else {
            UnshadeClient(np);
         }
      }
      break;
   case ACTION_STICK:
      if(np) {
         if(arg) {
            value = arg[0] == '1';
         } else {
            value = !(np->state.status & STAT_STICKY);
         }
         SetClientSticky(np, value);
      }
      break;
   case ACTION_MOVE:
      if(np) {
         if(useKeyboard) {
            MoveClientKeyboard(np);
         } else {
            MoveClient(np, x - np->x, y - np->y, 1);
         }
      }
      break;
   case ACTION_RESIZE:
      if(np) {
         if(useKeyboard) {
            ResizeClientKeyboard(np);
         } else {
            ResizeClient(np, x, y);
         }
      }
      break;
   case ACTION_MIN:
      if(np) {
         if(arg) {
            value = arg[0] == '1';
         } else {
            value = !(np->state.status & STAT_MINIMIZED);
         }
         MinimizeClient(np, value);
      }
      break;
   case ACTION_MAX:
      if(np) {
         MaximizeClientDefault(np);
      }
      break;
   case ACTION_ROOT:
      if(JLIKELY(arg)) {
         ShowRootMenu((unsigned int)atoi(arg), x, y);
      }
      break;
   case ACTION_WIN:
      if(np) {
         ShowWindowMenu(np, x, y);
      }
      break;
   case ACTION_RESTART:
      Restart();
      break;
   case ACTION_EXIT:
      Exit();
      break;
   case ACTION_FULLSCREEN:
      if(np) {
         if(arg) {
            value = arg[0] == '1';
         } else {
            value = !(np->state.status & STAT_FULLSCREEN);
         }
         SetClientFullScreen(np, value);
      }
      break;
   case ACTION_SENDTO:
      if(np && arg) {
         SetClientDesktop(np, atoi(arg));
      }
      break;
   case ACTION_SENDLEFT:
      if(np) {
         SetClientDesktop(np, GetLeftDesktop());
      }
      break;
   case ACTION_SENDRIGHT:
      if(np) {
         SetClientDesktop(np, GetRightDesktop());
      }
      break;
   case ACTION_SENDUP:
      if(np) {
         SetClientDesktop(np, GetAboveDesktop());
      }
      break;
   case ACTION_SENDDOWN:
      if(np) {
         SetClientDesktop(np, GetBelowDesktop());
      }
      break;
   default:
      break;
   }
}

