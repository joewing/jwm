
#ifndef KEY_H
#define KEY_H

struct ClientNode;

typedef enum {
	KEY_NONE    = 0,
	KEY_UP      = 1,
	KEY_DOWN    = 2,
	KEY_RIGHT   = 3,
	KEY_LEFT    = 4,
	KEY_ESC     = 5,
	KEY_ENTER   = 6,
	KEY_NEXT    = 7,
	KEY_CLOSE   = 8,
	KEY_MIN     = 9,
	KEY_MAX     = 10,
	KEY_SHADE   = 11,
	KEY_MOVE    = 12,
	KEY_RESIZE  = 13,
	KEY_ROOT    = 14,
	KEY_WIN     = 15,
	KEY_DESKTOP = 16,
	KEY_EXEC    = 17,
	KEY_RESTART = 18,
	KEY_EXIT    = 19
} KeyType;

void InitializeKeys();
void StartupKeys();
void ShutdownKeys();
void DestroyKeys();

KeyType GetKey(const XKeyEvent *event);
void GrabKeys(struct ClientNode *np);

void InsertBinding(KeyType key, const char *modifiers,
	const char *stroke, const char *command);

void RunKeyCommand(const XKeyEvent *event);

#endif

