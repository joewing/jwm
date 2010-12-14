/**
 * @file clientlist.c
 * @author Joe Wingbermuehle
 * @date 2007
 *
 * @brief Functions to manage lists of clients.
 *
 */

#include "jwm.h"
#include "clientlist.h"
#include "client.h"
#include "main.h"
#include "key.h"

ClientNode *nodes[LAYER_COUNT];
ClientNode *nodeTail[LAYER_COUNT];

static Window *windowStack = NULL;  /**< Image of the window stack. */
static int windowStackSize = 0;     /**< Size of the image. */
static int windowStackCurrent = 0;  /**< Current location in the image. */

/** Determine if a client is allowed focus. */
int ShouldFocus(const ClientNode *np) {

   if(np->state.desktop != currentDesktop
      && !(np->state.status & STAT_STICKY)) {
      return 0;
   }

   if(np->state.status & STAT_NOLIST) {
      return 0;
   }

   if(np->owner != None) {
      return 0;
   }

   return 1;

}

/** Start walking the window stack. */
void StartWindowStackWalk() {

   /* Get an image of the window stack.
    * Here we get the Window IDs rather than client pointers so
    * clients can be added/removed without disrupting the stack walk.
    */

   ClientNode *np;
   int layer;
   int count;

   /* If we are already walking the stack, just return. */
   if(windowStack != NULL) {
      return;
   }

   /* First determine how much space to allocate for windows. */
   count = 0;
   for(layer = LAYER_TOP; layer >= LAYER_BOTTOM; layer--) {
      for(np = nodes[layer]; np; np = np->next) {
         if(ShouldFocus(np)) {
            ++count;
         }
      }
   }

   /* If there were no windows to walk, don't even start. */
   if(count == 0) {
      return;
   }

   /* Allocate space for the windows. */
   windowStack = Allocate(sizeof(Window) * count);

   /* Copy windows into the array. */
   windowStackSize = 0;
   for(layer = LAYER_TOP; layer >= LAYER_BOTTOM; layer--) {
      for(np = nodes[layer]; np; np = np->next) {
         if(ShouldFocus(np)) {
            windowStack[windowStackSize++] = np->window;
         }
      }
   }

   Assert(windowStackSize == count);

   windowStackCurrent = 0;

}

/** Move to the next window in the window stack. */
void WalkWindowStack(int forward) {

   ClientNode *np;
   int x;

   if(windowStack != NULL) {

      /* Loop until we either raise a window or go through them all. */
      for(x = 0; x < windowStackSize; x++) {

         /* Move to the next/previous window (wrap if needed). */
         if(forward) {
             windowStackCurrent = (windowStackCurrent + 1) % windowStackSize;
         } else {
             if(windowStackCurrent == 0) {
                 windowStackCurrent = windowStackSize;
             }
             --windowStackCurrent;
         }

         /* Look up the window. */
         np = FindClientByWindow(windowStack[windowStackCurrent]);

         /* Skip this window if it no longer exists or is currently in
          * a state that doesn't allow focus.
          */
         if(np == NULL || !ShouldFocus(np)) {
            continue;
         }

         /* Focus the window. We only raise the client when the
          * stack walk completes. */
         FocusClient(np);
         break;

      }

   }

}

/** Stop walking the window stack. */
void StopWindowStackWalk() {

   ClientNode *np;

   /* Raise the selected window and free the window array. */
   if(windowStack != NULL) {

      /* Look up the current window. */
      np = FindClientByWindow(windowStack[windowStackCurrent]);
      if(np) {
         if(np->state.status & STAT_MINIMIZED) {
            RestoreClient(np, 1);
         } else {
            RaiseClient(np);
         }
      }

      Release(windowStack);
      windowStack = NULL;

      windowStackSize = 0;
      windowStackCurrent = 0;

   }

}

/** Focus the next client in the stacking order. */
void FocusNextStacked(ClientNode *np) {

   int x;
   ClientNode *tp;

   Assert(np);

   for(tp = np->next; tp; tp = tp->next) {
      if((tp->state.status & (STAT_MAPPED | STAT_SHADED))
         && !(tp->state.status & STAT_HIDDEN)) {
         FocusClient(tp);
         return;
      }
   }
   for(x = np->state.layer - 1; x >= LAYER_BOTTOM; x--) {
      for(tp = nodes[x]; tp; tp = tp->next) {
         if((tp->state.status & (STAT_MAPPED | STAT_SHADED))
            && !(tp->state.status & STAT_HIDDEN)) {
            FocusClient(tp);
            return;
         }
      }
   }

}

