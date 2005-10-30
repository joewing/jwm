/****************************************************************************
 * Functions for handling window groups.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef GROUP_H
#define GROUP_H

struct ClientNode;
struct GroupType;

typedef enum {
	OPTION_INVALID  = 0,
	OPTION_STICKY   = 1,
	OPTION_LAYER    = 2,
	OPTION_DESKTOP  = 3,
	OPTION_ICON     = 4,
	OPTION_NOLIST   = 5,
	OPTION_BORDER   = 6,
	OPTION_NOBORDER = 7,
	OPTION_TITLE    = 8,
	OPTION_NOTITLE  = 9
} OptionType;

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

void ApplyGroups(struct ClientNode *np);

#endif

