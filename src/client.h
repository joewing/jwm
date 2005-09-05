/****************************************************************************
 * Header for client window functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef CLIENT_H
#define CLIENT_H

extern ClientNode *nodes[LAYER_COUNT];
extern ClientNode *nodeTail[LAYER_COUNT];

ClientNode *FindClientByWindow(Window w);
ClientNode *FindClientByParent(Window p);
ClientNode *GetActiveClient();

void InitializeClients();
void StartupClients();
void ShutdownClients();
void DestroyClients();

ClientNode *AddClientWindow(Window w, int alreadyMapped, int notOwner);
void RemoveClient(ClientNode *np);

void MinimizeClient(ClientNode *np);
void ShadeClient(ClientNode *np);
void UnshadeClient(ClientNode *np);
void SetClientWithdrawn(ClientNode *np, int isWithdrawn);
void WithdrawClient(ClientNode *np);
void RestoreClient(ClientNode *np);
void MaximizeClient(ClientNode *np);
void FocusClient(ClientNode *np);
void FocusNextStacked(ClientNode *np);
void RefocusClient();
void DeleteClient(ClientNode *np);
void KillClient(ClientNode *np);
void RaiseClient(ClientNode *np);
void RestackClients();
void SetClientLayer(ClientNode *np, int layer);
void SetClientDesktop(ClientNode *np, int desktop);
void SetClientSticky(ClientNode *np, int isSticky);

void HideClient(ClientNode *np);
void ShowClient(ClientNode *np);

void ReadClientHints(ClientNode *np);
void ReadMotifHints(ClientNode *np);

void UpdateClientColormap(ClientNode *np);

void SetShape(ClientNode *np);

void SendConfigureEvent(ClientNode *np);

#endif

