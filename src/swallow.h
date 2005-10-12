/****************************************************************************
 ****************************************************************************/

#ifndef SWALLOW_H
#define SWALLOW_H

void InitializeSwallow();
void StartupSwallow();
void ShutdownSwallow();
void DestroySwallow();

TrayComponentType *CreateSwallow(const char *name, const char *command);

#endif

