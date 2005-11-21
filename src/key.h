/****************************************************************************
 * Header for key binding functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef KEY_H
#define KEY_H

struct ClientNode;

typedef enum {
	KEY_NONE         = 0,
	KEY_UP           = 1,
	KEY_DOWN         = 2,
	KEY_RIGHT        = 3,
	KEY_LEFT         = 4,
	KEY_ESC          = 5,
	KEY_ENTER        = 6,
	KEY_NEXT         = 7,
	KEY_NEXT_STACKED = 8,
	KEY_CLOSE        = 9,
	KEY_MIN          = 10,
	KEY_MAX          = 11,
	KEY_SHADE        = 12,
	KEY_MOVE         = 13,
	KEY_RESIZE       = 14,
	KEY_ROOT         = 15,
	KEY_WIN          = 16,
	KEY_DESKTOP      = 17,
	KEY_EXEC         = 18,
	KEY_RESTART      = 19,
	KEY_EXIT         = 20
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

