/****************************************************************************
 * Functions for handling window groups.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"
#include "group.h"
#include "client.h"
#include "icon.h"
#include "error.h"
#include "match.h"
#include "desktop.h"
#include "main.h"
#include "misc.h"

typedef enum {
	MATCH_NAME,
	MATCH_CLASS
} MatchType;

typedef struct PatternListType {
	char *pattern;
	MatchType match;
	struct PatternListType *next;
} PatternListType;

typedef struct OptionListType {
	OptionType option;
	char *value;
	struct OptionListType *next;
} OptionListType;

typedef struct GroupType {
	PatternListType *patterns;
	OptionListType *options;
	struct GroupType *next;
} GroupType;

static GroupType *groups = NULL;

static void ReleasePatternList(PatternListType *lp);
static void ReleaseOptionList(OptionListType *lp);
static void AddPattern(PatternListType **lp, const char *pattern,
	MatchType match);
static void ApplyGroup(const GroupType *gp, ClientNode *np);

/****************************************************************************
 ****************************************************************************/
void InitializeGroups() {
}

/****************************************************************************
 ****************************************************************************/
void StartupGroups() {
}

/****************************************************************************
 ****************************************************************************/
void ShutdownGroups() {
}

/****************************************************************************
 ****************************************************************************/
void DestroyGroups() {

	GroupType *gp;

	while(groups) {
		gp = groups->next;
		ReleasePatternList(groups->patterns);
		ReleaseOptionList(groups->options);
		Release(groups);
		groups = gp;
	}

}

/****************************************************************************
 ****************************************************************************/
void ReleasePatternList(PatternListType *lp) {

	PatternListType *tp;

	while(lp) {
		tp = lp->next;
		Release(lp->pattern);
		Release(lp);
		lp = tp;
	}

}

/****************************************************************************
 ****************************************************************************/
void ReleaseOptionList(OptionListType *lp) {

	OptionListType *tp;

	while(lp) {
		tp = lp->next;
		if(lp->value) {
			Release(lp->value);
		}
		Release(lp);
		lp = tp;
	}

}

/****************************************************************************
 ****************************************************************************/
GroupType *CreateGroup() {
	GroupType *tp;

	tp = Allocate(sizeof(GroupType));
	tp->patterns = NULL;
	tp->options = NULL;
	tp->next = groups;
	groups = tp;

	return tp;
}

/****************************************************************************
 ****************************************************************************/
void AddGroupClass(GroupType *gp, const char *pattern) {

	Assert(gp);

	if(pattern) {
		AddPattern(&gp->patterns, pattern, MATCH_CLASS);
	} else {
		Warning("invalid group class");
	}

}

/****************************************************************************
 ****************************************************************************/
void AddGroupName(GroupType *gp, const char *pattern) {

	Assert(gp);

	if(pattern) {
		AddPattern(&gp->patterns, pattern, MATCH_NAME);
	} else {
		Warning("invalid group name");
	}

}

/****************************************************************************
 ****************************************************************************/
void AddPattern(PatternListType **lp, const char *pattern, MatchType match) {

	PatternListType *tp;

	Assert(lp);
	Assert(pattern);

	tp = Allocate(sizeof(PatternListType));
	tp->next = *lp;
	*lp = tp;

	tp->pattern = CopyString(pattern);
	tp->match = match;

}

/****************************************************************************
 ****************************************************************************/
void AddGroupOption(GroupType *gp, OptionType option) {

	OptionListType *lp;

	lp = Allocate(sizeof(OptionListType));
	lp->option = option;
	lp->value = NULL;
	lp->next = gp->options;
	gp->options = lp;

}

/****************************************************************************
 ****************************************************************************/
void AddGroupOptionValue(GroupType *gp, OptionType option,
	const char *value) {

	OptionListType *lp;

	Assert(value);

	lp = Allocate(sizeof(OptionListType));
	lp->option = option;
	lp->value = CopyString(value);
	lp->next = gp->options;
	gp->options = lp;

}

/****************************************************************************
 ****************************************************************************/
void ApplyGroups(ClientNode *np) {

	PatternListType *lp;
	GroupType *gp;

	Assert(np);

	for(gp = groups; gp; gp = gp->next) {
		for(lp = gp->patterns; lp; lp = lp->next) {
			if(lp->match == MATCH_CLASS) {
				if(Match(lp->pattern, np->className)) {
					ApplyGroup(gp, np);
					break;
				}
			} else if(lp->match == MATCH_NAME) {
				if(Match(lp->pattern, np->name)) {
					ApplyGroup(gp, np);
					break;
				}
			} else {
				Debug("invalid match in ApplyGroups: %d", lp->match);
			}
		}
	}

}

/****************************************************************************
 ****************************************************************************/
void ApplyGroup(const GroupType *gp, ClientNode *np) {

	OptionListType *lp;
	unsigned int temp;

	Assert(gp);
	Assert(np);

	for(lp = gp->options; lp; lp = lp->next) {
		switch(lp->option) {
		case OPTION_STICKY:
			np->state.status |= STAT_STICKY;
			break;
		case OPTION_NOLIST:
			np->state.status |= STAT_NOLIST;
			break;
		case OPTION_BORDER:
			np->state.border |= BORDER_OUTLINE;
			break;
		case OPTION_NOBORDER:
			np->state.border &= ~BORDER_OUTLINE;
			break;
		case OPTION_TITLE:
			np->state.border |= BORDER_TITLE;
			break;
		case OPTION_NOTITLE:
			np->state.border &= ~BORDER_TITLE;
			break;
		case OPTION_LAYER:
			temp = atoi(lp->value);
			if(temp <= LAYER_COUNT) {
				SetClientLayer(np, temp);
			} else {
				Warning("invalid group layer: %s", lp->value);
			}
			break;
		case OPTION_DESKTOP:
			temp = atoi(lp->value);
			if(temp >= 1 && temp <= desktopCount) {
				np->state.desktop = temp - 1;
			} else {
				Warning("invalid group desktop: %s", lp->value);
			}
			break;
		case OPTION_ICON:
			DestroyIcon(np->icon);
			np->icon = LoadNamedIcon(lp->value);
			break;
		case OPTION_PIGNORE:
			np->state.status |= STAT_PIGNORE;
			break;
		case OPTION_MAXIMIZED:
			np->state.status |= STAT_MAXIMIZED;
			break;
		case OPTION_MINIMIZED:
			np->state.status |= STAT_MINIMIZED;
			break;
		case OPTION_SHADED:
			np->state.status |= STAT_SHADED;
			break;
		default:
			Debug("invalid option: %d", lp->option);
			break;
		}
	}

}

