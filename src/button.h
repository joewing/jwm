/***************************************************************************
 * Header for the button functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ***************************************************************************/

#ifndef BUTTON_H
#define BUTTON_H

#include "font.h"

typedef enum {
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

void SetButtonDrawable(Drawable d, GC g);
void SetButtonFont(FontType f);
void SetButtonSize(int w, int h);
void SetButtonAlignment(AlignmentType a);
void SetButtonTextOffset(int o);

void DrawButton(int x, int y, ButtonType type, const char *str);

#endif

