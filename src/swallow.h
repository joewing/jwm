/****************************************************************************
 ****************************************************************************/

#ifndef SWALLOW_H
#define SWALLOW_H

void InitializeSwallow();
void StartupSwallow();
void ShutdownSwallow();
void DestroySwallow();

void Swallow(const char *name, const char *command,
	SwallowLocationType location);

#endif

