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
TrayComponentType *CreateTrayComponent();
void AddTrayComponent(TrayType *tp, TrayComponentType *cp);

void ShowTray(TrayType *tp);
void HideTray(TrayType *tp);

void DrawTray();
void DrawSpecificTray(const TrayType *tp);
void UpdateSpecificTray(const TrayType *tp, const TrayComponentType *cp);

TrayType *GetTrays();

Window GetSupportingWindow();

int ProcessTrayEvent(const XEvent *event);

void SetAutoHideTray(TrayType *tp, int v);
void SetTrayX(TrayType *tp, const char *str);
void SetTrayY(TrayType *tp, const char *str);
void SetTrayWidth(TrayType *tp, const char *str);
void SetTrayHeight(TrayType *tp, const char *str);
void SetTrayLayout(TrayType *tp, const char *str);
void SetTrayLayer(TrayType *tp, const char *str);
void SetTrayBorder(TrayType *tp, const char *str);

#endif

