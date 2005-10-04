/****************************************************************************
 * Functions to render icons using the XRender extension.
 * Copyright (C) 2005 Joe Wingbermuehle
 ****************************************************************************/

#ifndef RENDER_H
#define RENDER_H

void QueryRenderExtension();

int PutRenderIcon(const IconNode *icon, Drawable d, int x, int y);

IconNode *CreateRenderIconFromImage(ImageNode *image);

#endif

