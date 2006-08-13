/**
 * @file status.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the status functions.
 *
 */

#ifndef STATUS_H
#define STATUS_H

struct ClientNode;

void CreateMoveWindow(struct ClientNode *np);
void UpdateMoveWindow(struct ClientNode *np);
void DestroyMoveWindow();

void CreateResizeWindow(struct ClientNode *np);
void UpdateResizeWindow(struct ClientNode *np, int gwidth, int gheight);
void DestroyResizeWindow();

void SetMoveStatusType(const char *str);
void SetResizeStatusType(const char *str);

#endif


