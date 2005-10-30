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
	int code;
	char *command;
	struct KeyNode *next;

	/* These are filled in by InitializeKeys */
	unsigned int state;

} KeyNode;

static XModifierKeymap *modmap;

static KeyNode *bindings = NULL;

static unsigned int GetModifierMask(KeySym key);
static unsigned int ParseModifierString(const char *str);
static KeySym ParseKeyString(const char *str);
static int ShouldGrab(KeyType key);

/***************************************************************************
 ***************************************************************************/
void InitializeKeys() {
	bindings = NULL;
}

/***************************************************************************
 ***************************************************************************/
void StartupKeys() {
	KeyNode *np;

	modmap = JXGetModifierMapping(display);

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
			JXGrabKey(display, np->code, np->state,
				rootWindow, True, GrabModeSync, GrabModeAsync);
/*TODO: grab for all trays.
			JXGrabKey(display, np->code, np->state,
				trayWindow, True, GrabModeSync, GrabModeAsync);
*/
		}

	}

	JXFreeModifiermap(modmap);

}

/***************************************************************************
 ***************************************************************************/
void ShutdownKeys() {
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
KeyType GetKey(const XKeyEvent *event) {
	KeyNode *np;

	for(np = bindings; np; np = np->next) {
		if(np->state == event->state && np->code == event->keycode) {
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
	const char *stroke, const char *command) {

	KeyNode *np;
	unsigned int modifierMask;
	char *temp;
	int offset;
	KeySym code;

	Assert(stroke);

	modifierMask = ParseModifierString(modifiers);

	for(offset = 0; stroke[offset]; offset++) {
		if(stroke[offset] == '#') {

			temp = Allocate(strlen(stroke) + 1);
			strcpy(temp, stroke);

			for(temp[offset] = '1'; temp[offset] <= '9'; temp[offset]++) {
				code = ParseKeyString(temp);
				if(code == NoSymbol) {
					Release(temp);
					return;
				}

				np = Allocate(sizeof(KeyNode));
				np->next = bindings;
				bindings = np;

				np->key = key | ((temp[offset] - '1' + 1) << 8);
				np->mask = modifierMask;
				np->code = code;
				np->command = NULL;

			}

			Release(temp);

			return;
		}
	}

	code = ParseKeyString(stroke);
	if(code == NoSymbol) {
		return;
	}

	np = Allocate(sizeof(KeyNode));
	np->next = bindings;
	bindings = np;

	np->key = key;
	np->mask = modifierMask;
	np->code = code;
	if(command) {
		np->command = Allocate(strlen(command) + 1);
		strcpy(np->command, command);
	} else {
		np->command = NULL;
	}

}

