/***************************************************************************
 * Header for the button functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ***************************************************************************/

#ifndef BUTTON_H
#define BUTTON_H

#include "font.h"
#include "color.h"

struct IconNode;
struct PartNode;

typedef enum {
	BUTTON_NORMAL,
	BUTTON_ACTIVE,
	BUTTON_PUSHED,
	BUTTON_NONE
} ButtonType;

typedef enum {
	ALIGN_LEFT,
	ALIGN_CENTER,
	ALIGN_RIGHT
} AlignmentType;

typedef struct ButtonData {
	ButtonType type;
	AlignmentType alignment;
	FontType font;
	ColorType color;
	ColorType background;
	struct PartNode *fillPart;
	int x, y;
	int width, height;
	Drawable drawable;
	GC gc;
	struct IconNode *icon;
	const char *text;
} ButtonData;

void ResetButton(ButtonData *button, Drawable d, GC g);

void DrawButton(const ButtonData *button);

void SetButtonOffsets(int north, int south, int east, int west);
void GetButtonOffsets(int *north, int *south, int *east, int *west);

#endif

