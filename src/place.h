/**
 * @file place.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for client placement functions.
 *
 */

#ifndef PLACE_H
#define PLACE_H

struct ClientNode;

/*@{*/
void InitializePlacement();
void StartupPlacement();
void ShutdownPlacement();
void DestroyPlacement();
/*@}*/

void RemoveClientStrut(struct ClientNode *np);
void ReadClientStrut(struct ClientNode *np);

void PlaceClient(struct ClientNode *np, int alreadyMapped);
void PlaceMaximizedClient(struct ClientNode *np);
void GravitateClient(struct ClientNode *np, int negate);

void GetGravityDelta(const struct ClientNode *np, int *x, int *y);

void ConstrainSize(struct ClientNode *np);

#endif

