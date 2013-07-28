/**
 * @file resize.h
 * @author Joe Wingbermuehle
 * @date 2005-2006
 *
 * @brief Header for client window resize functions.
 *
 */

#ifndef RESIZE_H
#define RESIZE_H

#include "border.h"

struct ActionContext;

/** Resize a client window.
 * @param ac The action context.
 * @param startx The starting mouse x-coordinate (window relative).
 * @param starty The starting mouse y-coordinate (window relative).
 */
void ResizeClient(const struct ActionContext *ac,
                  int startx, int starty);

/** Resize a client window using the keyboard (mouse optional).
 * @param ac The action context.
 * @param startx (ignored)
 * @param starty (ignored)
 */
void ResizeClientKeyboard(const struct ActionContext *ac,
                          int startx, int starty);

#endif /* RESIZE_H */

