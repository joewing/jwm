/*
 * @file border.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header file for border functions.
 *
 */

#ifndef BORDER_H
#define BORDER_H

struct ClientNode;

typedef enum {
	BA_NONE      = 0,
	BA_RESIZE    = 1,
	BA_MOVE      = 2,
	BA_CLOSE     = 3,
	BA_MAXIMIZE  = 4,
	BA_MINIMIZE  = 5,
	BA_MENU      = 6,
	BA_RESIZE_N  = 0x10,
	BA_RESIZE_S  = 0x20,
	BA_RESIZE_E  = 0x40,
	BA_RESIZE_W  = 0x80
} BorderActionType;

void InitializeBorders();
void StartupBorders();
void ShutdownBorders();
void DestroyBorders();

BorderActionType GetBorderActionType(const struct ClientNode *np, int x, int y);
void DrawBorder(const struct ClientNode *np, const XExposeEvent *expose);

int GetBorderIconSize();

void GetBorderSize(const struct ClientNode *np,
	int *north, int *south, int *east, int *west);

void SetBorderWidth(const char *str);
void SetTitleHeight(const char *str);

void ExposeCurrentDesktop();

#endif

