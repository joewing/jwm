/***************************************************************************
 * Header for the tray functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ***************************************************************************/

#ifndef TRAY_H
#define TRAY_H

void InitializeTray();
void StartupTray();
void ShutdownTray();
void DestroyTray();

TrayType *CreateTray();
void AddTrayComponent(TrayType *tp, TrayComponentType *cp);

int GetTrayWidth(TrayType *tp);
int GetTrayHeight(TrayType *tp);
int GetTrayX(TrayType *tp);
int GetTrayY(TrayType *tp);

void ShowTray(TrayType *tp);
void HideTray(TrayType *tp);

void DrawTray();
void DrawSpecificTray(const TrayType *tp);

TrayType *GetTrays();

int ProcessTrayEvent(const XEvent *event);

void SetAutoHideTray(TrayType *tp, int v);
void SetTrayX(TrayType *tp, const char *str);
void SetTrayY(TrayType *tp, const char *str);
void SetTrayWidth(TrayType *tp, const char *str);
void SetTrayHeight(TrayType *tp, const char *str);
void SetTrayLayout(TrayType *tp, const char *str);
void SetTrayLayer(TrayType *tp, const char *str);

#endif

