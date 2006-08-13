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

/*@{*/
void InitializePager();
void StartupPager();
void ShutdownPager();
void DestroyPager();
/*@}*/

struct TrayComponentType *CreatePager();

void UpdatePager();

#endif

