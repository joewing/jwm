/**
 * @file theme.h
 * @author Joe Wingbermuehle
 * @date 2006
 *
 * @brief Header for the theme functions.
 *
 */

#ifndef THEME_H
#define THEME_H

/*@{*/
void InitializeThemes();
void StartupThemes();
void ShutdownThemes();
void DestroyThemes();
/*@}*/

void AddThemePath(char *path);

void SetTheme(const char *name);

#endif

