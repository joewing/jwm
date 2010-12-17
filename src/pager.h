/**
 * @file pager.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Pager tray component.
 *
 */

#ifndef PAGER_H
#define PAGER_H

struct TrayComponentType;
struct TimeType;

/*@{*/
void InitializePager();
void StartupPager();
void ShutdownPager();
void DestroyPager();
/*@}*/

/** Create a pager tray component.
 * @param labeled Set to label the pager.
 * @return A new pager tray component.
 */
struct TrayComponentType *CreatePager(int labeled);

/** Update pagers. */
void UpdatePager();

/** Signal pagers. */
void SignalPager(const struct TimeType *now, int x, int y);

#endif /* PAGER_H */

