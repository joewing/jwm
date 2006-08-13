/*
 * @file button.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header file for button functions.
 *
 */

#ifndef BUTTON_H
#define BUTTON_H

#include "font.h"

struct IconNode;

typedef enum {
	BUTTON_LABEL,
	BUTTON_MENU,
	BUTTON_MENU_ACTIVE,
	BUTTON_TASK,
	BUTTON_TASK_ACTIVE
} ButtonType;

typedef enum {
	ALIGN_LEFT,
	ALIGN_CENTER,
	ALIGN_RIGHT
} AlignmentType;

typedef struct {

	ButtonType type;
	Drawable drawable;
	GC gc;
	FontType font;
	AlignmentType alignment;

	int x, y;
	int width, height;

	struct IconNode *icon;
	const char *text;

} ButtonNode;

void DrawButton(ButtonNode *bp);

void ResetButton(ButtonNode *bp, Drawable d, GC g);

#endif

