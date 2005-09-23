/***************************************************************************
 * Header for the tray functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ***************************************************************************/

#ifndef TRAY_H
#define TRAY_H

extern int autoHideTray;
extern int trayHeight;
extern int trayWidth;
extern int trayX, trayY;
extern int trayLayer;

extern Window trayWindow;
extern int trayIsHidden;

void InitializeTray();
void StartupTray();
void ShutdownTray();
void DestroyTray();

void ShowTray();
void HideTray();

void FocusNext();

void NextDesktop();
void PreviousDesktop();
void ChangeDesktop(int desktop);

void DrawTray();
int UpdateTime();
void UpdatePager();

void AddClientToTray(ClientNode *np);
void RemoveClientFromTray(ClientNode *np);

int ProcessTrayEvent(const XEvent *event);

void SetAutoHideTray(int v);
void SetMaxTrayItemWidth(const char *str);
void SetTrayHeight(const char *str);
void SetTrayWidth(const char *str);
void SetTrayAlignment(const char *str);
void SetClockProgram(const char *command);
void SetLoadProgram(const char *command);
void SetMenuTitle(const char *title);
void SetMenuIcon(const char *name);
void SetTrayInsertMode(const char *str);
void SetTrayLayer(const char *str);

#endif

