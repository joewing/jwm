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
struct ImageNode;
struct ScaledIconNode;

/** Put a scaled icon.
 * @param image The image to display.
 * @param node The rendered image to display.
 * @param d The drawable on which to render the icon.
 * @param x The x-coordinate to place the icon.
 * @param y The y-coordinate to place the icon.
 * @return 1 if the icon was successfully rendered, 0 otherwise.
 */
void PutScaledRenderIcon(const struct ImageNode *image,
                         const struct ScaledIconNode *node,
                         Drawable d, int x, int y);

/** Create a scaled icon.
 * @param icon The icon.
 * @param fg The foreground color (for bitmaps).
 * @return The scaled icon.
 */
struct ScaledIconNode *CreateScaledRenderIcon(struct ImageNode *image, long fg);

#endif /* RENDER_H */
