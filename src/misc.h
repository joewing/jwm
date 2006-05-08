/****************************************************************************
 * Misc. macros.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef MISC_H
#define MISC_H

#define Min( x, y ) ( (x) > (y) ? (y) : (x) )
#define Max( x, y ) ( (x) > (y) ? (x) : (y) )

void ExpandPath(char **path);
void Trim(char *str);
char *CopyString(const char *str);

#endif

