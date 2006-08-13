/**
 * @file misc.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Miscellaneous functions and macros.
 *
 */

#ifndef MISC_H
#define MISC_H

#define Min( x, y ) ( (x) > (y) ? (y) : (x) )
#define Max( x, y ) ( (x) > (y) ? (x) : (y) )
#define Ceiling( x ) ( (x) > (double)(long)(x) ? (long)((x) + 1) : (long)(x) )
#define Floor( x ) ( (long)(x) )
#define Round( x ) ( (long)((x) + 0.5) )

void ExpandPath(char **path);
void Trim(char *str);
char *CopyString(const char *str);

#endif

