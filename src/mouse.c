
#include "jwm.h"
#include "mouse.h"
#include "key.h"
#include "misc.h"
#include "binding.h"
#include "client.h"
#include "settings.h"

typedef struct MouseNode {
   int button;
   ContextType context;
   ActionType  action;
   char *command;
   unsigned int state;
   struct MouseNode *next;
} MouseNode;

static MouseNode *bindings[CONTEXT_COUNT];

void InitializeMouse()
{
   int i;
   for(i = 0; i < CONTEXT_COUNT; i++) {
      bindings[i] = NULL;
   }
}

void DestroyMouse()
{
   MouseNode *mp;
   int i;
   for(i = 0; i < CONTEXT_COUNT; i++) {
      while(bindings[i]) {
         mp = bindings[i]->next;
         if(bindings[i]->command) {
            Release(bindings[i]->command);
         }
         Release(bindings[i]);
         bindings[i] = mp;
      }
   }
}

void RunMouseCommand(const XButtonEvent *event, ContextType context)
{

   static int lastX = 0, lastY = 0;
   static Time lastClickTime;
   static char doubleClickActive = 0;
   const unsigned int state = event->state & lockMask;
   int button;
   ClientNode *np;
   MouseNode *mp;

   button = event->button;
   if(event->type == ButtonPress) {
      if(doubleClickActive &&
         abs(event->time - lastClickTime) > 0 &&
         abs(event->time - lastClickTime) <= settings.doubleClickSpeed &&
         abs(event->x_root - lastX) <= settings.doubleClickDelta &&
         abs(event->y_root - lastY) <= settings.doubleClickDelta) {
         doubleClickActive = 0;
         button *= 11;
      } else {
         doubleClickActive = 1;
         lastX = event->x_root;
         lastY = event->y_root;
         lastClickTime = event->time;
      }
   }

   np = FindClient(event->window);
   mp = bindings[context];
   while(mp) {
      if(button == mp->button && state == mp->state) {
         RunAction(np, event->x_root, event->y_root, mp->action, mp->command);
         break;
      }
      mp = mp->next;
   }

}

void InsertMouseBinding(ContextType context,
                        ActionType action,
                        int button,
                        const char *modifiers,
                        const char *command)
{

   MouseNode *mp;

   mp = Allocate(sizeof(MouseNode));
   mp->next = bindings[context];
   bindings[context] = mp;

   mp->context = context;
   mp->button  = button;
   mp->action  = action;
   mp->state   = ParseModifierString(modifiers);
   mp->command = CopyString(command);

   switch(mp->action) {
   case ACTION_ROOT:
   case ACTION_WIN:
   case ACTION_MOVE:
   case ACTION_RESIZE:
      break;
   default:
      switch(mp->button) {
      case Button1:     mp->state |= Button1Mask;  break;
      case Button2:     mp->state |= Button2Mask;  break;
      case Button3:     mp->state |= Button3Mask;  break;
      case Button4:     break;
      case Button5:     break;
      default:          break;
      }
      break;
   }

}
