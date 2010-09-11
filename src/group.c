/**
 * @file group.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions for handling window groups.
 *
 */

#include "jwm.h"
#include "group.h"
#include "client.h"
#include "icon.h"
#include "error.h"
#include "match.h"
#include "desktop.h"
#include "main.h"
#include "misc.h"

/** What part of the window to match. */
typedef enum {
   MATCH_NAME,  /**< Match the window name. */
   MATCH_CLASS  /**< Match the window class. */
} MatchType;

/** List of match patterns for a group. */
typedef struct PatternListType {
   char *pattern;
   MatchType match;
   struct PatternListType *next;
} PatternListType;

/** List of options for a group. */
typedef struct OptionListType {
   OptionType option;
   char *value;
   struct OptionListType *next;
} OptionListType;

/** List of groups. */
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

/** Initialize group data. */
void InitializeGroups() {
}

/** Startup group support. */
void StartupGroups() {
}

/** Shutdown group support. */
void ShutdownGroups() {
}

/** Destroy group data. */
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

/** Release a group pattern list. */
void ReleasePatternList(PatternListType *lp) {

   PatternListType *tp;

   while(lp) {
      tp = lp->next;
      Release(lp->pattern);
      Release(lp);
      lp = tp;
   }

}

/** Release a group option list. */
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

/** Create an empty group. */
GroupType *CreateGroup() {
   GroupType *tp;

   tp = Allocate(sizeof(GroupType));
   tp->patterns = NULL;
   tp->options = NULL;
   tp->next = groups;
   groups = tp;

   return tp;
}

/** Add a window class to a group. */
void AddGroupClass(GroupType *gp, const char *pattern) {

   Assert(gp);

   if(pattern) {
      AddPattern(&gp->patterns, pattern, MATCH_CLASS);
   } else {
      Warning("invalid group class");
   }

}

/** Add a window name to a group. */
void AddGroupName(GroupType *gp, const char *pattern) {

   Assert(gp);

   if(pattern) {
      AddPattern(&gp->patterns, pattern, MATCH_NAME);
   } else {
      Warning("invalid group name");
   }

}

/** Add a pattern to a pattern list. */
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

/** Add an option to a group. */
void AddGroupOption(GroupType *gp, OptionType option) {

   OptionListType *lp;

   lp = Allocate(sizeof(OptionListType));
   lp->option = option;
   lp->value = NULL;
   lp->next = gp->options;
   gp->options = lp;

}

/** Add an option (with value) to a group. */
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

/** Apply groups to a client. */
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
            if(Match(lp->pattern, np->instanceName)) {
               ApplyGroup(gp, np);
               break;
            }
         } else {
            Debug("invalid match in ApplyGroups: %d", lp->match);
         }
      }
   }

}

/** Apply a group to a client. */
void ApplyGroup(const GroupType *gp, ClientNode *np) {

   OptionListType *lp;
   unsigned int temp;
   float tempf;

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
         np->state.status |= STAT_HMAX | STAT_VMAX;
         break;
      case OPTION_MINIMIZED:
         np->state.status |= STAT_MINIMIZED;
         break;
      case OPTION_SHADED:
         np->state.status |= STAT_SHADED;
         break;
      case OPTION_OPACITY:
         tempf = atof(lp->value);
         if(tempf > 0.0 && tempf <= 1.0) {
            np->state.opacity = (unsigned int)(tempf * UINT_MAX);
            np->state.status |= STAT_OPACITY;
         } else {
            Warning("invalid group opacity: %s", lp->value);
         }
         break;
      case OPTION_MAX_V:
         np->state.border &= ~BORDER_MAX_H;
         break;
      case OPTION_MAX_H:
         np->state.border &= ~BORDER_MAX_V;
         break;
      case OPTION_NOFOCUS:
         np->state.status |= STAT_NOFOCUS;
         break;
      default:
         Debug("invalid option: %d", lp->option);
         break;
      }
   }

}

