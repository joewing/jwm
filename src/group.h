/**
 * @file group.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions for handling window groups.
 *
 */

#ifndef GROUP_H
#define GROUP_H

struct ClientNode;
struct GroupType;

/** Enumeration of group options. */
typedef enum {
   OPTION_INVALID    = 0,
   OPTION_STICKY     = 1,  /**< Start in the sticky state. */
   OPTION_LAYER      = 2,  /**< Start on a specific layer. */
   OPTION_DESKTOP    = 3,  /**< Start on a specific desktop. */
   OPTION_ICON       = 4,  /**< Set the icon to use. */
   OPTION_NOLIST     = 5,  /**< Don't display in the task list. */
   OPTION_BORDER     = 6,  /**< Force a window border. */
   OPTION_NOBORDER   = 7,  /**< Don't draw a window border. */
   OPTION_TITLE      = 8,  /**< Force a window title bar. */
   OPTION_NOTITLE    = 9,  /**< Don't draw a window title bar. */
   OPTION_PIGNORE    = 10, /**< Ignore program-specified location. */
   OPTION_MAXIMIZED  = 11, /**< Start maximized. */
   OPTION_MINIMIZED  = 12, /**< Start minimized. */
   OPTION_SHADED     = 13, /**< Start shaded. */
   OPTION_OPACITY    = 14, /**< Set the opacity. */
   OPTION_MAX_H      = 15, /**< Use horizontal maximization. */
   OPTION_MAX_V      = 16, /**< Use vertical maximization. */
   OPTION_NOFOCUS    = 17  /**< Don't focus on map. */
} OptionType;

void InitializeGroups();
void StartupGroups();
void ShutdownGroups();
void DestroyGroups();

/** Create an empty group.
 * @return An empty group.
 */
struct GroupType *CreateGroup();

/** Add a window class to a group.
 * @param gp The group.
 * @param pattern A pattern to match with the window class.
 */
void AddGroupClass(struct GroupType *gp, const char *pattern);

/** Add a window name to a group.
 * @param gp The group.
 * @param pattern A pattern to match with the window name.
 */
void AddGroupName(struct GroupType *gp, const char *pattern);

/** Add a group option that doesn't take a value.
 * @param gp The group.
 * @param option The option.
 */
void AddGroupOption(struct GroupType *gp, OptionType option);

/** Add a group option that takes a value.
 * @param gp The group.
 * @param option The option.
 * @param value The option value.
 */
void AddGroupOptionValue(struct GroupType *gp, OptionType option,
   const char *value);

/** Apply any matching groups to a client.
 * @param np The client.
 */
void ApplyGroups(struct ClientNode *np);

#endif /* GROUP_H */

