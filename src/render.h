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

/** Put a scaled icon.
 * @param icon The icon.
 * @param node The scaled icon data.
 * @param d The drawable on which to render the icon.
 * @param x The x-coordinate to place the icon.
 * @param y The y-coordinate to place the icon.
 * @return 1 if the icon was successfully rendered, 0 otherwise.
 */
int PutScaledRenderIcon(struct IconNode *icon, struct ScaledIconNode *node,
   Drawable d, int x, int y);

/** Create a scaled icon.
 * @param icon The icon.
 * @param width The width of the icon to create.
 * @param height The height of the icon to create.
 * @return The scaled icon.
 */
struct ScaledIconNode *CreateScaledRenderIcon(struct IconNode *icon,
   int width, int height);

#endif /* RENDER_H */

