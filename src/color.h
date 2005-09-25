/****************************************************************************
 * Header for the color functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef COLOR_H
#define COLOR_H

extern CARD32 colors[COLOR_COUNT];
extern CARD32 rgbColors[COLOR_COUNT];
extern CARD32 *ramps[RAMP_COUNT];

extern CARD32 white;
extern CARD32 black;

void InitializeColors();
void StartupColors();
void ShutdownColors();
void DestroyColors();

void SetColor(ColorType c, const char *value);

void GetColor(XColor *c);

#endif

