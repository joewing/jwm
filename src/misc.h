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

/** Return the minimum of two values. */
#define Min( x, y ) ( (x) > (y) ? (y) : (x) )

/** Return the maximum of two values. */
#define Max( x, y ) ( (x) > (y) ? (x) : (y) )

/** Perform shell-like macro path expansion.
 * @param path The path to expand (possibly reallocated).
 */
void ExpandPath(char **path);

/** Trim leading and trailing whitespace from a string.
 * @param str The string to trim.
 */
void Trim(char *str);

/** Copy a string.
 * Note that NULL is accepted. When provided NULL, NULL will be returned.
 * @param str The string to copy.
 * @return A copy of the string.
 */
char *CopyString(const char *str);

#endif /* MISC_H */

