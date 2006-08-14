/**
 * @file root.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the root menu functions.
 *
 */

#ifndef ROOT_H
#define ROOT_H

struct Menu;

/*@{*/
void InitializeRootMenu();
void StartupRootMenu();
void ShutdownRootMenu();
void DestroyRootMenu();
/*@}*/

void SetRootMenu(const char *indexes, struct Menu *m);
void SetShowExitConfirmation(int v);

int IsRootMenuDefined(int index);
void GetRootMenuSize(int index, int *width, int *height);
int ShowRootMenu(int index, int x, int y);

void RunCommand(const char *command);

void Restart();
void Exit();

#endif

