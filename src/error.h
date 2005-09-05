/****************************************************************************
 * Header for the error functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef ERROR_H
#define ERROR_H

void FatalError(const char *str, ...);
void Warning(const char *str, ...);
void WarningVA(const char *part, const char *str, va_list ap);
int ErrorHandler(Display *d, XErrorEvent *e);

#endif

