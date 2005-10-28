/****************************************************************************
 * Header for the color functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef COLOR_H
#define COLOR_H

extern unsigned long colors[COLOR_COUNT];
extern unsigned long rgbColors[COLOR_COUNT];

extern unsigned long white;
extern unsigned long black;

void InitializeColors();
void StartupColors();
void ShutdownColors();
void DestroyColors();

void SetColor(ColorType c, const char *value);

void GetColor(XColor *c);

#ifdef USE_XFT
XftColor *GetXftColor(ColorType type);
#endif

#endif

