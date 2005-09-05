
#ifndef KEY_H
#define KEY_H

void InitializeKeys();
void StartupKeys();
void ShutdownKeys();
void DestroyKeys();

KeyType GetKey(const XKeyEvent *event);
void GrabKeys(ClientNode *np);

void InsertBinding(KeyType key, const char *modifiers,
	const char *stroke, const char *command);

void RunKeyCommand(const XKeyEvent *event);

#endif

