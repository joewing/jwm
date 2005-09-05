/***************************************************************************
 * Header for the button functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ***************************************************************************/

#ifndef BUTTON_H
#define BUTTON_H

void SetButtonDrawable(Drawable d, GC g);
void SetButtonFont(FontType f);
void SetButtonSize(int w, int h);
void SetButtonAlignment(AlignmentType a);
void SetButtonTextOffset(int o);

void DrawButton(int x, int y, ButtonType type, const char *str);

#endif

