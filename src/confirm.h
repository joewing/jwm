/**
 * @file confirm.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the confirm dialog functions.
 *
 */

#ifndef CONFIRM_H
#define CONFIRM_H

struct ClientNode;

/*@{*/
void InitializeDialogs();
void StartupDialogs();
void ShutdownDialogs();
void DestroyDialogs();
/*@}*/

int ProcessDialogEvent(const XEvent *event);

void ShowConfirmDialog(struct ClientNode *np,
	void (*action)(struct ClientNode*), ...);

#endif

