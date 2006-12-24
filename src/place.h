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

/** Remove struts associated with a client.
 * @param np The client.
 */
void RemoveClientStrut(struct ClientNode *np);

/** Read struts associated with a client.
 * @param np The client.
 */
void ReadClientStrut(struct ClientNode *np);

/** Place a client on the screen.
 * @param np The client to place.
 * @param alreadyMapped 1 if already mapped, 0 if unmapped.
 */
void PlaceClient(struct ClientNode *np, int alreadyMapped);

/** Place a maximized client on the screen.
 * @param np The client to place.
 */
void PlaceMaximizedClient(struct ClientNode *np);

/** Move a client window for a border.
 * @param np The client.
 * @param negate 0 to gravitate for a border, 1 to gravitate for no border.
 */
void GravitateClient(struct ClientNode *np, int negate);

/** Get the x and y deltas for gravitating a client.
 * @param np The client.
 * @param x Location to store the x delta.
 * @param y Location to store the y delta.
 */
void GetGravityDelta(const struct ClientNode *np, int *x, int *y);

/** Constrain the size of a client to available screen space.
 * @param np The client.
 */
void ConstrainSize(struct ClientNode *np);

#endif /* PLACE_H */

