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
#include "root.h"
#include "tray.h"

typedef enum {
   MASK_NONE  = 0,
   MASK_ALT   = 1,
   MASK_CTRL  = 2,
   MASK_SHIFT = 4,
   MASK_HYPER = 8,
   MASK_META  = 16,
   MASK_SUPER = 32
} MaskType;

typedef struct ModifierNode {
   char name;
   MaskType mask;
   KeySym symbol1;
   KeySym symbol2;
} ModifierNode;

static ModifierNode modifiers[] = {

   { 'A',   MASK_ALT,   XK_Alt_L,      XK_Alt_R       },
   { 'C',   MASK_CTRL,  XK_Control_L,  XK_Control_R   },
   { 'S',   MASK_SHIFT, XK_Shift_L,    XK_Shift_R     },
   { 'H',   MASK_HYPER, XK_Hyper_L,    XK_Hyper_R     },
   { 'M',   MASK_META,  XK_Meta_L,     XK_Meta_R      },
   { 'P',   MASK_SUPER, XK_Super_L,    XK_Super_R     },
   { 0,     MASK_NONE,  XK_Shift_L,    XK_Shift_R     }

};

typedef struct KeyNode {

   /* These are filled in when the configuration file is parsed */
   int key;
   unsigned int mask;
   KeySym symbol;
   char *command;
   struct KeyNode *next;

   /* This is filled in by StartupKeys if it isn't already set. */
   KeyCode code;

   /* This is filled in by StartupKeys. */
   unsigned int state;

} KeyNode;

typedef struct LockNode {
   KeySym symbol;
   unsigned int mask;
} LockNode;

static XModifierKeymap *modmap;

static LockNode mods[] = {
   { XK_Caps_Lock,   0 },
   { XK_Num_Lock,    0 }
};

static KeyNode *bindings;
static unsigned int modifierMask;

static unsigned int GetModifierMask(KeySym key);
static unsigned int ParseModifierString(const char *str);
static ModifierNode *GetKeyFromModifier(const char *str);
static KeySym ParseKeyString(const char *str);
static int ShouldGrab(KeyType key);
static void GrabKey(KeyNode *np);

/** Initialize key data. */
void InitializeKeys() {

   bindings = NULL;
   modifierMask = 0;

}

/** Startup key bindings. */
void StartupKeys() {

   KeyNode *np;
   int x;

   modmap = JXGetModifierMapping(display);

   for(x = 0; x < sizeof(mods) / sizeof(mods[0]); x++) {
      mods[x].mask = GetModifierMask(mods[x].symbol);
      modifierMask |= mods[x].mask;
   }

   for(np = bindings; np; np = np->next) {

      np->state = 0;
      if(np->mask & MASK_ALT) {
         np->state |= GetModifierMask(XK_Alt_L);
      }
      if(np->mask & MASK_CTRL) {
         np->state |= GetModifierMask(XK_Control_L);
      }
      if(np->mask & MASK_SHIFT) {
         np->state |= GetModifierMask(XK_Shift_L);
      }
      if(np->mask & MASK_HYPER) {
         np->state |= GetModifierMask(XK_Hyper_L);
      }
      if(np->mask & MASK_META) {
         np->state |= GetModifierMask(XK_Meta_L);
      }
      if(np->mask & MASK_SUPER) {
         np->state |= GetModifierMask(XK_Super_L);
      }

      if(!np->code) {
         np->code = JXKeysymToKeycode(display, np->symbol);
      }

      if(ShouldGrab(np->key)) {
         GrabKey(np);
      }

   }

   JXFreeModifiermap(modmap);

}

/** Shutdown key bindings. */
void ShutdownKeys() {

   ClientNode *np;
   int layer;

   for(layer = 0; layer < LAYER_COUNT; layer++) {
      for(np = nodes[layer]; np; np = np->next) {
         JXUngrabKey(display, AnyKey, AnyModifier, np->window);
      }
   }

   JXUngrabKey(display, AnyKey, AnyModifier, rootWindow);

}

/** Destroy key data. */
void DestroyKeys() {

   KeyNode *np;

   while(bindings) {
      np = bindings->next;
      if(bindings->command) {
         Release(bindings->command);
      }
      Release(bindings);
      bindings = np;
   }

}

/** Grab a key. */
void GrabKey(KeyNode *np) {

   TrayType *tp;
   int x;
   int index, maxIndex;
   unsigned int mask;

   /* Don't attempt to grab if there is nothing to grab. */
   if(!np->code) {
      return;
   }

   maxIndex = 1 << (sizeof(mods) / sizeof(mods[0]));
   for(index = 0; index < maxIndex; index++) {

      mask = 0;
      for(x = 0; x < sizeof(mods) / sizeof(mods[0]); x++) {
         if(index & (1 << x)) {
            mask |= mods[x].mask;
         }
      }

      mask |= np->state;
      JXGrabKey(display, np->code, mask,
         rootWindow, True, GrabModeAsync, GrabModeAsync);
      for(tp = GetTrays(); tp; tp = tp->next) {
         JXGrabKey(display, np->code, mask,
            tp->window, True, GrabModeAsync, GrabModeAsync);
      }

   }

}

/** Get the key action from an event. */
KeyType GetKey(const XKeyEvent *event) {

   KeyNode *np;
   unsigned int state;

   /* Remove modifiers we don't care about from the state. */
   state = event->state & ~modifierMask;

   /* Loop looking for a matching key binding. */
   for(np = bindings; np; np = np->next) {

      /* We need to treat KeyRelease events on modifiers with a
       * special case since the state and keycode members will have
       * the key set despite the fact we grabbed with no state. */
      if(np->key == KEY_NEXTSTACK_END) {
         if(np->code == event->keycode) {
            return np->key;
         }
      } else {
         if(np->state == state && np->code == event->keycode) {
            return np->key;
         }
      }

   }

   return KEY_NONE;

}

/** Run a command invoked from a key binding. */
void RunKeyCommand(const XKeyEvent *event) {

   KeyNode *np;

   for(np = bindings; np; np = np->next) {
      if(np->state == event->state && np->code == event->keycode) {
         RunCommand(np->command);
         return;
      }
   }

}

/** Show a root menu caused by a key binding. */
void ShowKeyMenu(const XKeyEvent *event) {

   KeyNode *np;
   int button;

   for(np = bindings; np; np = np->next) {
      if(np->state == event->state && np->code == event->keycode) {
         button = atoi(np->command);
         if(button >= 0 && button <= 9) {
            ShowRootMenu(button, 0, 0);
         }
         return;
      }
   }

}

/** Determine if a key should be grabbed on client windows. */
int ShouldGrab(KeyType key) {
   switch(key & 0xFF) {
   case KEY_NEXT:
   case KEY_NEXTSTACK:
   case KEY_NEXTSTACK_END:
   case KEY_CLOSE:
   case KEY_MIN:
   case KEY_MAX:
   case KEY_SHADE:
   case KEY_MOVE:
   case KEY_RESIZE:
   case KEY_ROOT:
   case KEY_WIN:
   case KEY_DESKTOP:
   case KEY_EXEC:
   case KEY_RESTART:
   case KEY_EXIT:
      return 1;
   default:
      return 0;
   }
}

/** Grab keys on a client window. */
void GrabKeys(ClientNode *np) {

   KeyNode *kp;

   for(kp = bindings; kp; kp = kp->next) {
      if(ShouldGrab(kp->key) && kp->code) {
         JXGrabKey(display, kp->code, kp->state,
            np->window, True, GrabModeAsync, GrabModeAsync);
      }
   }

}

/** Get the modifier mask for a key. */
unsigned int GetModifierMask(KeySym key) {

   KeyCode temp;
   int x;

   temp = JXKeysymToKeycode(display, key);
   for(x = 0; x < 8 * modmap->max_keypermod; x++) {
      if(modmap->modifiermap[x] == temp) {
         return 1 << (x / modmap->max_keypermod);
      }
   }

   Warning("modifier not found for keysym 0x%0x", key);

   return 0;

}

/** Parse a modifier mask string. */
unsigned int ParseModifierString(const char *str) {

   unsigned int mask;
   int x, y;
   int found;

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

      if(!found) {
         Warning("invalid modifier: \"%c\"", str[x]);
      }

   }

   return mask;

}

/** Get the key from a modifier character. */
ModifierNode *GetKeyFromModifier(const char *str) {

   int x, y;

   if(str == NULL) {
      return NULL;
   }

   for(x = 0; str[x]; x++) {

      for(y = 0; modifiers[y].name; y++) {
         if(modifiers[y].name == str[x]) {
            return &modifiers[y];
         }
      }

   }

   return NULL;

}

/** Parse a key string. */
KeySym ParseKeyString(const char *str) {

   KeySym symbol;

   symbol = JXStringToKeysym(str);
   if(symbol == NoSymbol) {
      Warning("invalid key symbol: \"%s\"", str);
   }

   return symbol;

}

/** Insert a key binding. */
void InsertBinding(KeyType key, const char *modifiers,
   const char *stroke, const char *code, const char *command) {

   KeyNode *np;
   ModifierNode *mp;
   unsigned int mask;
   char *temp;
   int offset;
   KeySym sym;

   mask = ParseModifierString(modifiers);

   if(stroke && strlen(stroke) > 0) {

      for(offset = 0; stroke[offset]; offset++) {
         if(stroke[offset] == '#') {

            temp = CopyString(stroke);

            for(temp[offset] = '1'; temp[offset] <= '9'; temp[offset]++) {

               sym = ParseKeyString(temp);
               if(sym == NoSymbol) {
                  Release(temp);
                  return;
               }

               np = Allocate(sizeof(KeyNode));
               np->next = bindings;
               bindings = np;

               np->key = key | ((temp[offset] - '1' + 1) << 8);
               np->mask = mask;
               np->symbol = sym;
               np->command = NULL;
               np->code = 0;

            }

            Release(temp);

            return;
         }
      }

      sym = ParseKeyString(stroke);
      if(sym == NoSymbol) {
         return;
      }

      np = Allocate(sizeof(KeyNode));
      np->next = bindings;
      bindings = np;

      np->key = key;
      np->mask = mask;
      np->symbol = sym;
      np->command = CopyString(command);
      np->code = 0;

   } else if(code && strlen(code) > 0) {

      np = Allocate(sizeof(KeyNode));
      np->next = bindings;
      bindings = np;

      np->key = key;
      np->mask = mask;
      np->symbol = NoSymbol;
      np->command = CopyString(command);
      np->code = atoi(code);

   } else {

      Warning("neither key nor keycode specified for Key");
      np = NULL;

   }

   /* KEY_NEXTSTACK is special in that we need to know when the user
    * is finished walking the window stack. To do this we watch for
    * the release of a modifier key used with the KEY_NEXTSTACK binding
    * and report it as KEY_NEXTSTACK_END. If no modifier is specified
    * for KEY_NEXTSTACK we use KEY_NEXT instead and warn the user.
    */
   if(np != NULL && key == KEY_NEXTSTACK) {

      mp = GetKeyFromModifier(modifiers);
      if(!mp) {

         Warning("no valid modifier specified for \"nextstacked\"");
         np->key = KEY_NEXT;

      } else {

         /* Create a node for the first modifier. */
         np = Allocate(sizeof(KeyNode));
         np->next = bindings;
         bindings = np;

         np->key = KEY_NEXTSTACK_END;
         np->mask = 0;
         np->symbol = mp->symbol1;
         np->command = NULL;
         np->code = 0;

         /* Create a node for the second modifier. */
         np = Allocate(sizeof(KeyNode));
         np->next = bindings;
         bindings = np;

         np->key = KEY_NEXTSTACK_END;
         np->mask = 0;
         np->symbol = mp->symbol2;
         np->command = NULL;
         np->code = 0;


      }

   }

}

/** Validate key bindings. */
void ValidateKeys() {

   KeyNode *kp;
   int bindex;

   for(kp = bindings; kp; kp = kp->next) {
      if((kp->key & 0xFF) == KEY_ROOT && kp->command) {
         bindex = atoi(kp->command);
         if(!IsRootMenuDefined(bindex)) {
            Warning("key binding: root menu %d not defined", bindex);
         }
      }
   }

}

