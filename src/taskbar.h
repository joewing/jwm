/***************************************************************************
 ***************************************************************************/

#ifndef TASKBAR_H
#define TASKBAR_H

void InitializeTaskBar();
void StartupTaskBar();
void ShutdownTaskBar();
void DestroyTaskBar();

TrayComponentType *CreateTaskBar();

void AddClientToTaskBar(ClientNode *np);
void RemoveClientFromTaskBar(ClientNode *np);

void UpdateTaskBar();

void FocusNext();

void SetMaxTaskBarItemWidth(unsigned int w);

#endif

