/****************************************************************************
 * Header for X hints.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef PROT_H
#define PROT_H

Atom atoms[ATOM_COUNT];

void InitializeHints();
void StartupHints();
void ShutdownHints();
void DestroyHints();

void ReadCurrentDesktop();

void ReadClientProtocols(ClientNode *np);

void ReadWMName(ClientNode *np);
void ReadWMClass(ClientNode *np);
void ReadWMNormalHints(ClientNode *np);
void ReadWMProtocols(ClientNode *np);
void ReadWMColormaps(ClientNode *np);

void ReadNetWMDesktop(ClientNode *np);

void ReadWinLayer(ClientNode *np);
void WriteWinState(ClientNode *np);

int GetCardinalAtom(Window window, AtomType atom, CARD32 *value);
int GetWindowAtom(Window window, AtomType atom, Window *value);

void SetCardinalAtom(Window window, AtomType atom, CARD32 value);
void SetWindowAtom(Window window, AtomType atom, CARD32 value);

#endif

