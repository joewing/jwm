/**
 * @file render.h
 * @author Joe Wingbermuehle
 * @date 2005-2006
 *
 * @brief Functions to render icons using the XRender extension.
 *
 */

#ifndef RENDER_H
#define RENDER_H

struct IconNode;
struct ScaledIconNode;

void QueryRenderExtension();

int PutScaledRenderIcon(struct IconNode *icon, struct ScaledIconNode *node,
   Drawable d, int x, int y);

struct ScaledIconNode *CreateScaledRenderIcon(struct IconNode *icon,
   int width, int height);

#endif

