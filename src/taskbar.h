/***************************************************************************
 ***************************************************************************/

#ifndef TASKBAR_H
#define TASKBAR_H

struct ClientNode;
struct TimeType;

void InitializeTaskBar();
void StartupTaskBar();
void ShutdownTaskBar();
void DestroyTaskBar();

struct TrayComponentType *CreateTaskBar();

void AddClientToTaskBar(struct ClientNode *np);
void RemoveClientFromTaskBar(struct ClientNode *np);

void UpdateTaskBar();

void SignalTaskbar(const struct TimeType *now, int x, int y);

void FocusNext();
void FocusPrevious();
void FocusNextStackedCircular();

void SetMaxTaskBarItemWidth(struct TrayComponentType *cp, const char *value);
void SetTaskBarInsertMode(const char *mode);

void UpdateNetClientList();

#endif

