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

/** Add a theme path.
 * @param path The path to add.
 */
void AddThemePath(const char *path);

/** Set the theme to use.
 * @param name The name of the theme.
 */
void SetTheme(const char *name);

#endif

