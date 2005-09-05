/****************************************************************************
 * Header for the color functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef COLOR_H
#define COLOR_H

extern unsigned long colors[COLOR_COUNT];
extern unsigned int rgbColors[COLOR_COUNT];
extern unsigned long *ramps[RAMP_COUNT];

extern unsigned long white;
extern unsigned long black;

void InitializeColors();
void StartupColors();
void ShutdownColors();
void DestroyColors();

void SetColor(ColorType c, const char *value);

void GetColor(XColor *c);

#endif

