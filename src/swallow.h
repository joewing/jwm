/****************************************************************************
 ****************************************************************************/

#ifndef SWALLOW_H
#define SWALLOW_H

typedef enum {
	SWALLOW_LEFT,
	SWALLOW_RIGHT
} SwallowLocationType;

void InitializeSwallow();
void StartupSwallow();
void ShutdownSwallow();
void DestroySwallow();

struct TrayComponentType *CreateSwallow(
	const char *name, const char *command,
	int width, int height);

int CheckSwallowMap(const XMapEvent *event);
int ProcessSwallowEvent(const XEvent *event);

#endif

