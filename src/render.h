/****************************************************************************
 * Functions to render icons using the XRender extension.
 * Copyright (C) 2005 Joe Wingbermuehle
 ****************************************************************************/

#ifndef RENDER_H
#define RENDER_H

void QueryRenderExtension();

int PutScaledRenderIcon(IconNode *icon, ScaledIconNode *node, Drawable d,
	int x, int y);

ScaledIconNode *CreateScaledRenderIcon(IconNode *icon, int width, int height);

#endif

