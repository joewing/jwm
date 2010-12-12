/**
 * @file clock.h
 * @author Joe Wingbermuehle
 * @date 2005-2006
 *
 * @brief Clock tray component.
 *
 */

#ifndef CLOCK_H
#define CLOCK_H

struct TrayComponentType;
struct TimeType;

/*@{*/
void InitializeClock();
void StartupClock();
void ShutdownClock();
void DestroyClock();
/*@}*/

/** Create a clock component for the tray.
 * @param format The format of the clock.
 * @param zone The timezone of the clock (NULL for local time).
 * @param command The command to execute when the clock is clicked.
 * @param width The width of the clock (0 for auto).
 * @param height The height of the clock (0 for auto).
 */
struct TrayComponentType *CreateClock(const char *format, const char *zone,
   const char *command, int width, int height);

/** Update clocks.
 * This is called on a regular basis to update the time.
 * @param now The current time.
 * @param x The x-coordinate of the mouse.
 * @param y The y-coordinate of the mouse.
 */
void SignalClock(const struct TimeType *now, int x, int y);

#endif /* CLOCK_H */

