/***************************************************************************
 ***************************************************************************/

#ifndef TASKBAR_H
#define TASKBAR_H

struct ClientNode;

void InitializeTaskBar();
void StartupTaskBar();
void ShutdownTaskBar();
void DestroyTaskBar();

struct TrayComponentType *CreateTaskBar();

void AddClientToTaskBar(struct ClientNode *np);
void RemoveClientFromTaskBar(struct ClientNode *np);

void UpdateTaskBar();

void SignalTaskbar();

void FocusNext();

void SetMaxTaskBarItemWidth(struct TrayComponentType *cp, const char *value);
void SetTaskBarInsertMode(const char *mode);

#endif

