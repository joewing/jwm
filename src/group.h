/****************************************************************************
 * Functions for handling window groups.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef GROUP_H
#define GROUP_H

void InitializeGroups();
void StartupGroups();
void ShutdownGroups();
void DestroyGroups();

struct GroupType *CreateGroup();
void AddGroupClass(struct GroupType *gp, const char *pattern);
void AddGroupName(struct GroupType *gp, const char *pattern);
void AddGroupOption(struct GroupType *gp, OptionType option);
void AddGroupOptionValue(struct GroupType *gp, OptionType option,
	const char *value);

void ApplyGroups(ClientNode *np);

#endif

