/***************************************************************************
 * Key input functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ***************************************************************************/

#include "jwm.h"
#include "key.h"
#include "main.h"
#include "client.h"
#include "root.h"
#include "error.h"
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

typedef struct KeyNode {

	/* These are filled in when the configuration file is parsed */
	int key;
	unsigned int mask;
	unsigned int code;
	char *command;
	struct KeyNode *next;

	/* These are filled in by InitializeKeys */
	unsigned int state;

} KeyNode;

typedef struct LockNode {
	unsigned int key;
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
static KeySym ParseKeyString(const char *str);
static int ShouldGrab(KeyType key);
static void GrabKey(KeyNode *np, int index);

/***************************************************************************
 ***************************************************************************/
void InitializeKeys() {

	bindings = NULL;
	modifierMask = 0;

}

/***************************************************************************
 ***************************************************************************/
void StartupKeys() {

	KeyNode *np;
	int x;

	modmap = JXGetModifierMapping(display);

	for(x = 0; x < sizeof(mods) / sizeof(mods[0]); x++) {
		mods[x].mask = GetModifierMask(mods[x].key);
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
		np->code = JXKeysymToKeycode(display, np->code);

		if(ShouldGrab(np->key)) {
			GrabKey(np, 0);
		}

	}

	JXFreeModifiermap(modmap);

}

/***************************************************************************
 ***************************************************************************/
void ShutdownKeys() {

	JXUngrabKey(display, AnyKey, AnyModifier, rootWindow);

}

/***************************************************************************
 ***************************************************************************/
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

/***************************************************************************
 ***************************************************************************/
void GrabKey(KeyNode *np, int index) {

	TrayType *tp;
	int x;
	unsigned int mask;

	mask = 0;
	for(x = 0; x < sizeof(mods) / sizeof(mods[0]); x++) {
		if(index & (1 << x)) {
			mask |= mods[x].mask;
		}
	}

	mask |= np->state;
	JXGrabKey(display, np->code, mask,
		rootWindow, True, GrabModeSync, GrabModeAsync);
	for(tp = GetTrays(); tp; tp = tp->next) {
		JXGrabKey(display, np->code, mask,
			tp->window, True, GrabModeSync, GrabModeAsync);
	}

	++index;
	if(!(index & (1 << sizeof(mods) / sizeof(mods[0])))) {
		GrabKey(np, index);
	}

}

/***************************************************************************
 ***************************************************************************/
KeyType GetKey(const XKeyEvent *event) {

	KeyNode *np;
	unsigned int state;

	state = event->state & ~modifierMask;
	for(np = bindings; np; np = np->next) {
		if(np->state == state && np->code == event->keycode) {
			return np->key;
		}
	}

	return KEY_NONE;

}

/***************************************************************************
 ***************************************************************************/
void RunKeyCommand(const XKeyEvent *event) {
	KeyNode *np;

	for(np = bindings; np; np = np->next) {
		if(np->state == event->state && np->code == event->keycode) {
			RunCommand(np->command);
			return;
		}
	}
}

/***************************************************************************
 ***************************************************************************/
int ShouldGrab(KeyType key) {
	switch(key & 0xFF) {
	case KEY_NEXT:
	case KEY_NEXT_STACKED:
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

/***************************************************************************
 ***************************************************************************/
void GrabKeys(ClientNode *np) {
	KeyNode *kp;

	for(kp = bindings; kp; kp = kp->next) {
		if(ShouldGrab(kp->key)) {
			JXGrabKey(display, kp->code, kp->state,
				np->window, True, GrabModeSync, GrabModeAsync);
		}
	}

}

/***************************************************************************
 ***************************************************************************/
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

/***************************************************************************
 ***************************************************************************/
unsigned int ParseModifierString(const char *str) {
	unsigned int mask;
	int x;

	if(!str) {
		return MASK_NONE;
	}

	mask = MASK_NONE;
	for(x = 0; str[x]; x++) {
		switch(str[x]) {
		case 'A':
			mask |= MASK_ALT;
			break;
		case 'C':
			mask |= MASK_CTRL;
			break;
		case 'S':
			mask |= MASK_SHIFT;
			break;
		case 'H':
			mask |= MASK_HYPER;
			break;
		case 'M':
			mask |= MASK_META;
			break;
		case 'P':
			mask |= MASK_SUPER;
			break;
		default:
			Warning("invalid modifier: \"%c\"", str[x]);
			break;
		}
	}

	return mask;

}

/***************************************************************************
 ***************************************************************************/
KeySym ParseKeyString(const char *str) {
	KeySym symbol;

	symbol = JXStringToKeysym(str);
	if(symbol == NoSymbol) {
		Warning("invalid key: \"%s\"", str);
	}

	return symbol;

}

/***************************************************************************
 ***************************************************************************/
void InsertBinding(KeyType key, const char *modifiers,
	const char *stroke, const char *code, const char *command) {

	KeyNode *np;
	unsigned int modifierMask;
	char *temp;
	int offset;
	KeySym sym;

	modifierMask = ParseModifierString(modifiers);

	if(stroke && strlen(stroke) > 0) {

		for(offset = 0; stroke[offset]; offset++) {
			if(stroke[offset] == '#') {

				temp = Allocate(strlen(stroke) + 1);
				strcpy(temp, stroke);

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
					np->mask = modifierMask;
					np->code = sym;
					np->command = NULL;

				}

				Release(temp);

				return;
			}
		}

		sym = ParseKeyString(stroke);
		if(sym == NoSymbol) {
			Warning("keycode not found for symbol: %s", stroke);
			return;
		}
		np = Allocate(sizeof(KeyNode));
		np->next = bindings;
		bindings = np;

		np->key = key;
		np->mask = modifierMask;
		np->code = sym;
		if(command) {
			np->command = Allocate(strlen(command) + 1);
			strcpy(np->command, command);
		} else {
			np->command = NULL;
		}

	} else if(code && strlen(code) > 0) {

		np = Allocate(sizeof(KeyNode));
		np->next = bindings;
		bindings = np;

		np->key = key;
		np->mask = modifierMask;
		np->code = atoi(code);
		if(command) {
			np->command = Allocate(strlen(command) + 1);
			strcpy(np->command, command);
		} else {
			np->command = NULL;
		}

	} else {

		Warning("neither key nor keycode specified for Key");

	}

}

