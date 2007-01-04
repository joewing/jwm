/**
 * @file background.h
 * @author Joe Wingbermuehle
 * @date 2007
 *
 * @brief Background control functions.
 *
 */

#ifndef BACKGROUND_H
#define BACKGROUND_H

/** Initialize any data needed for background support. */
void InitializeBackgrounds();

/** Startup background support. */
void StartupBackgrounds();

/** Shutdown background support. */
void ShutdownBackgrounds();

/** Release any data needed for background support. */
void DestroyBackgrounds();

/** Set the background to use for the specified desktops.
 * @param desktop The desktop whose background to set (-1 for the default).
 * @param type The type of background.
 * @param value The background.
 */
void SetBackground(int desktop, const char *type, const char *value);

/** Load the background for the specified desktop.
 * @param desktop The current desktop.
 */
void LoadBackground(int desktop);

#endif /* BACKGROUND_H */

