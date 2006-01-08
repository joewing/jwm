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

void RemoveClientStrut(struct ClientNode *np);
void ReadClientStrut(struct ClientNode *np);

void PlaceClient(struct ClientNode *np, int alreadyMapped);
void PlaceMaximizedClient(struct ClientNode *np);
void GravitateClient(struct ClientNode *np, int negate);

void GetBorderSize(const struct ClientNode *np,
	int *north, int *south, int *east, int *west);
void GetGravityDelta(int gravity, int north, int south, int east, int west,
	int *x, int *y);

#endif

