/**
 * @file key.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Key binding functions.
 *
 */

#include "jwm.h"
#include "key.h"

#include "client.h"
#include "clientlist.h"
#include "command.h"
#include "error.h"
#include "main.h"
#include "misc.h"
#include "tray.h"
#include "binding.h"
#include "move.h"
#include "resize.h"
#include "root.h"

#define MASK_NONE    0
#define MASK_SHIFT   (1 << ShiftMapIndex)
#define MASK_LOCK    (1 << LockMapIndex)
#define MASK_CTRL    (1 << ControlMapIndex)
#define MASK_MOD1    (1 << Mod1MapIndex)
#define MASK_MOD2    (1 << Mod2MapIndex)
#define MASK_MOD3    (1 << Mod3MapIndex)
#define MASK_MOD4    (1 << Mod4MapIndex)
#define MASK_MOD5    (1 << Mod5MapIndex)

typedef struct ModifierNode {
   char           name;
   unsigned int   mask;
} ModifierNode;

static ModifierNode modifiers[] = {

   { 'C',   MASK_CTRL      },
   { 'S',   MASK_SHIFT     },
   { 'A',   MASK_MOD1      },
   { '1',   MASK_MOD1      },
   { '2',   MASK_MOD2      },
   { '3',   MASK_MOD3      },
   { '4',   MASK_MOD4      },
   { '5',   MASK_MOD5      },
   { 0,     MASK_NONE      }

};

typedef struct KeyNode {

   /* These are filled in when the configuration file is parsed */
   ActionNode action;
   unsigned int state;
   KeySym symbol;
   struct KeyNode *next;

   /* This is filled in by StartupKeys if it isn't already set. */
   KeyCode code;

} KeyNode;

typedef struct LockNode {
   KeySym symbol;
   unsigned int mask;
} LockNode;

static LockNode lockMods[] = {
   { XK_Caps_Lock,   0 },
   { XK_Num_Lock,    0 }
};

static KeyNode *bindings;
unsigned int lockMask;

static unsigned int GetModifierMask(XModifierKeymap *modmap, KeySym key);
static KeySym ParseKeyString(const char *str);
static char ShouldGrab(ActionType action);
static void GrabKey(KeyNode *np, Window win);

/** Initialize key data. */
void InitializeKeys()
{
   bindings = NULL;
   lockMask = 0;
}

/** Startup key bindings. */
void StartupKeys()
{

   XModifierKeymap *modmap;
   KeyNode *np;
   TrayType *tp;
   int x;

   /* Get the keys that we don't care about (num lock, etc). */
   modmap = JXGetModifierMapping(display);
   for(x = 0; x < sizeof(lockMods) / sizeof(lockMods[0]); x++) {
      lockMods[x].mask = GetModifierMask(modmap, lockMods[x].symbol);
      lockMask |= lockMods[x].mask;
   }
   lockMask = ~lockMask;
   JXFreeModifiermap(modmap);

   /* Look up and grab the keys. */
   for(np = bindings; np; np = np->next) {

      /* Determine the key code. */
      if(!np->code) {
         np->code = JXKeysymToKeycode(display, np->symbol);
      }

      /* Grab the key if needed. */
      if(ShouldGrab(np->action.type)) {

         /* Grab on the root. */
         GrabKey(np, rootWindow);

         /* Grab on the trays. */
         for(tp = GetTrays(); tp; tp = tp->next) {
            GrabKey(np, tp->window);
         }

      }

   }

}

/** Shutdown key bindings. */
void ShutdownKeys()
{

   ClientNode *np;
   TrayType *tp;
   unsigned int layer;

   /* Ungrab keys on client windows. */
   for(layer = 0; layer < LAYER_COUNT; layer++) {
      for(np = nodes[layer]; np; np = np->next) {
         JXUngrabKey(display, AnyKey, AnyModifier, np->window);
      }
   }

   /* Ungrab keys on trays, only really needed if we are restarting. */
   for(tp = GetTrays(); tp; tp = tp->next) {
      JXUngrabKey(display, AnyKey, AnyModifier, tp->window);
   }

   /* Ungrab keys on the root. */
   JXUngrabKey(display, AnyKey, AnyModifier, rootWindow);

}

/** Destroy key data. */
void DestroyKeys()
{
   KeyNode *np;
   while(bindings) {
      np = bindings->next;
      if(bindings->action.arg) {
         Release(bindings->action.arg);
      }
      Release(bindings);
      bindings = np;
   }
}

/** Grab a key. */
void GrabKey(KeyNode *np, Window win)
{
   unsigned int x;
   unsigned int index, maxIndex;
   unsigned int mask;

   /* Don't attempt to grab if there is nothing to grab. */
   if(!np->code) {
      return;
   }

   /* Grab for each lock modifier. */
   maxIndex = 1 << (sizeof(lockMods) / sizeof(lockMods[0]));
   for(index = 0; index < maxIndex; index++) {

      /* Compute the modifier mask. */
      mask = 0;
      for(x = 0; x < sizeof(lockMods) / sizeof(lockMods[0]); x++) {
         if(index & (1 << x)) {
            mask |= lockMods[x].mask;
         }
      }
      mask |= np->state;

      /* Grab the key. */
      JXGrabKey(display, np->code, mask, win,
                True, GrabModeAsync, GrabModeAsync);

   }

}

/** Get the key action from an event. */
ActionType GetKey(const XKeyEvent *event)
{

   KeyNode *np;
   unsigned int state;

   /* Remove modifiers we don't care about from the state. */
   state = event->state & lockMask;

   /* Loop looking for a matching key binding. */
   for(np = bindings; np; np = np->next) {
      if(np->state == state && np->code == event->keycode) {
         return np->action.type;
      }
   }

   return ACTION_NONE;

}

/** Run a command invoked from a key binding. */
void RunKeyCommand(const XKeyEvent *event)
{

   KeyNode *kp;
   ActionContext ac;
   unsigned int state;

   /* Remove the lock key modifiers. */
   state = event->state & lockMask;

   InitActionContext(&ac);
   ac.MoveFunc = MoveClientKeyboard;
   ac.ResizeFunc = ResizeClientKeyboard;
   for(kp = bindings; kp; kp = kp->next) {
      if(kp->state == state && kp->code == event->keycode) {
         if(event->type == KeyPress) {
            RunAction(&ac, &kp->action);
         } else {
            if(kp->action.type != ACTION_NEXTSTACK &&
               kp->action.type != ACTION_PREVSTACK) {
               StopWindowStackWalk();
            }
         }
         return;
      }
   }

}

/** Determine if a key should be grabbed on client windows. */
char ShouldGrab(ActionType action)
{
   switch(action) {
   case ACTION_NONE:
   case ACTION_UP:
   case ACTION_DOWN:
   case ACTION_RIGHT:
   case ACTION_LEFT:
   case ACTION_ESC:
   case ACTION_ENTER:
      return 0;
   default:
      return 1;
   }
}

/** Grab keys on a client window. */
void GrabKeys(ClientNode *np)
{

   KeyNode *kp;

   for(kp = bindings; kp; kp = kp->next) {
      if(ShouldGrab(kp->action.type)) {
         GrabKey(kp, np->window);
      }
   }

}

/** Get the modifier mask for a key. */
unsigned int GetModifierMask(XModifierKeymap *modmap, KeySym key) {

   KeyCode temp;
   int x;

   temp = JXKeysymToKeycode(display, key);
   if(JUNLIKELY(temp == 0)) {
      Warning(_("Specified KeySym is not defined for any KeyCode"));
   }
   for(x = 0; x < 8 * modmap->max_keypermod; x++) {
      if(modmap->modifiermap[x] == temp) {
         return 1 << (x / modmap->max_keypermod);
      }
   }

   Warning(_("modifier not found for keysym 0x%0x"), key);

   return 0;

}

/** Parse a modifier mask string. */
unsigned int ParseModifierString(const char *str)
{
   unsigned int mask;
   unsigned int x, y;
   char found;

   if(!str) {
      return MASK_NONE;
   }

   mask = MASK_NONE;
   for(x = 0; str[x]; x++) {

      found = 0;
      for(y = 0; modifiers[y].name; y++) {
         if(modifiers[y].name == str[x]) {
            mask |= modifiers[y].mask;
            found = 1;
            break;
         }
      }

      if(JUNLIKELY(!found)) {
         Warning(_("invalid modifier: \"%c\""), str[x]);
      }

   }

   return mask;

}

/** Parse a key string. */
KeySym ParseKeyString(const char *str)
{
   KeySym symbol;
   symbol = JXStringToKeysym(str);
   if(JUNLIKELY(symbol == NoSymbol)) {
      Warning(_("invalid key symbol: \"%s\""), str);
   }
   return symbol;
}

/** Insert a key binding. */
void InsertKeyBinding(const char *modifiers,
                      const char *stroke,
                      const char *code,
                      const ActionNode *action)
{

   KeyNode *np;
   unsigned int mask;
   KeySym sym;

   mask = ParseModifierString(modifiers);

   if(stroke && strlen(stroke) > 0) {

      sym = ParseKeyString(stroke);
      if(sym == NoSymbol) {
         return;
      }

      np = Allocate(sizeof(KeyNode));
      np->next = bindings;
      bindings = np;

      np->action = *action;
      np->state = mask;
      np->symbol = sym;
      np->code = 0;

   } else if(code && strlen(code) > 0) {

      np = Allocate(sizeof(KeyNode));
      np->next = bindings;
      bindings = np;

      np->action = *action;
      np->state = mask;
      np->symbol = NoSymbol;
      np->code = atoi(code);

   } else {

      Warning(_("neither key nor keycode specified for Key"));
      np = NULL;

   }

}

