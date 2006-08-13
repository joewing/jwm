/****************************************************************************
 * Outlines for moving and resizing.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef OUTLINE_H
#define OUTLINE_H

void InitializeOutline();
void StartupOutline();
void ShutdownOutline();
void DestroyOutline();

/** Draw an outline.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @param width The width of the outline.
 * @param height The height of the outline.
 */
void DrawOutline(int x, int y, int width, int height);

/** Clear an outline. */
void ClearOutline();

#endif

