/****************************************************************************
 * Header for client placement functions.
 * Copyright (C) 2005 Joe Wingbermuehle
 ****************************************************************************/

#ifndef PLACE_H
#define PLACE_H

struct ClientNode;

void InitializePlacement();
void StartupPlacement();
void ShutdownPlacement();
void DestroyPlacement();

void PlaceClient(struct ClientNode *np, int alreadyMapped);
void PlaceMaximizedClient(struct ClientNode *np);
void GravitateClient(struct ClientNode *np, int negate);

#endif

