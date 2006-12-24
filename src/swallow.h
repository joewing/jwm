/**
 * @file swallow.h
 * @author Joe Wingbermuehle
 * @date 2005-2006
 *
 * @brief Swallow tray component.
 *
 */

#ifndef SWALLOW_H
#define SWALLOW_H

/*@{*/
void InitializeSwallow();
void StartupSwallow();
void ShutdownSwallow();
void DestroySwallow();
/*@}*/

/** Create a swallowed application tray component.
 * @param name The name of the application to swallow.
 * @param command The command used to start the swallowed application.
 * @param width The width to use (0 for default).
 * @param height the height to use (0 for default).
 */
struct TrayComponentType *CreateSwallow(
   const char *name, const char *command,
   int width, int height);

/** Determine if a map event was for a window that should be swallowed.
 * @param event The map event.
 * @return 1 if this window should be swallowed, 0 if not.
 */
int CheckSwallowMap(const XMapEvent *event);

/** Process an event on a swallowed window.
 * @param event The event to process.
 * @return 1 if the event was for a swallowed window, 0 if not.
 */
int ProcessSwallowEvent(const XEvent *event);

#endif /* SWALLOW_H */

