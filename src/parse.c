/****************************************************************************
 * Parser for the JWM XML configuration file.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"

static const char *RESTART_COMMAND = "#restart";
static const char *RESTART_NAME = "Restart";
static const char *EXIT_COMMAND = "#exit";
static const char *EXIT_NAME = "Exit";

static const char *DEFAULT_TITLE = "JWM";
static const char *LABEL_ATTRIBUTE = "label";
static const char *ICON_ATTRIBUTE = "icon";
static const char *CONFIRM_ATTRIBUTE = "confirm";
static const char *ANTIALIAS_ATTRIBUTE = "antialias";
static const char *FORMAT_ATTRIBUTE = "format";
static const char *ENABLED_ATTRIBUTE = "enabled";
static const char *LABELED_ATTRIBUTE = "labeled";
static const char *ONROOT_ATTRIBUTE = "onroot";
static const char *LAYER_ATTRIBUTE = "layer";
static const char *AUTOHIDE_ATTRIBUTE = "autohide";
static const char *MAXWIDTH_ATTRIBUTE = "maxwidth";
static const char *INSERT_ATTRIBUTE = "insert";

static const char *FALSE_VALUE = "false";
static const char *TRUE_VALUE = "true";

static int ParseFile(const char *fileName, int depth);
static char *ReadFile(FILE *fd);

static void Parse(const TokenNode *start, int depth);

static void ParseRootMenu(const TokenNode *start);
static void ParseMenuItem(const TokenNode *start, MenuType *menu);
static void ParseInclude(const TokenNode *tp, int depth);
static void ParseKey(const TokenNode *tp);
static void ParseBorder(const TokenNode *tp);
static void ParseTray(const TokenNode *tp);
static void ParsePager(const TokenNode *tp);
static void ParsePopup(const TokenNode *tp);
static void ParseLoad(const TokenNode *tp);
static void ParseClock(const TokenNode *tp);
static void ParseMenu(const TokenNode *tp);
static void ParseSnapMode(const TokenNode *tp);
static void ParseMoveMode(const TokenNode *tp);
static void ParseResizeMode(const TokenNode *tp);
static void ParseFocusModel(const char *model);
static void ParseGroup(const TokenNode *tp);
static void ParseGroupOption(struct GroupType *group, const char *option);
static void ParseIcons(const TokenNode *tp);

static MenuItemType *InsertMenuItem(MenuItemType *last);
static char *FindAttribute(AttributeNode *ap, const char *name);
static void ReleaseTokens(TokenNode *np);
static void ParseError(const char *str, ...);
static void SetDesktopCount(const char *value);

/****************************************************************************
 ****************************************************************************/
void ParseConfig(const char *fileName) {
	if(!ParseFile(fileName, 0)) {
		if(!ParseFile(SYSTEM_CONFIG, 0)) {
			ParseError("could not open %s or %s", fileName, SYSTEM_CONFIG);
		}
	}
}

/****************************************************************************
 * Parse a specific file.
 * Returns 1 on success and 0 on failure.
 ****************************************************************************/
int ParseFile(const char *fileName, int depth) {

	TokenNode *tokens;
	FILE *fd;
	char *buffer;

	++depth;
	if(depth > MAX_INCLUDE_DEPTH) {
		ParseError("include depth (%d) exceeded", MAX_INCLUDE_DEPTH);
		return 0;
	}

	fd = fopen(fileName, "r");
	if(!fd) {
		return 0;
	}

	buffer = ReadFile(fd);
	fclose(fd);

	tokens = Tokenize(buffer);
	Release(buffer);
	Parse(tokens, depth);
	ReleaseTokens(tokens);

	return 1;

}

/***************************************************************************
 ***************************************************************************/
void ReleaseTokens(TokenNode *np) {
	AttributeNode *ap;
	TokenNode *tp;

	while(np) {
		tp = np->next;

		while(np->attributes) {
			ap = np->attributes->next;
			if(np->attributes->name) {
				Release(np->attributes->name);
			}
			if(np->attributes->value) {
				Release(np->attributes->value);
			}
			Release(np->attributes);
			np->attributes = ap;
		}

		if(np->subnodeHead) {
			ReleaseTokens(np->subnodeHead);
		}

		if(np->value) {
			Release(np->value);
		}

		Release(np);
		np = tp;
	}

}

/***************************************************************************
 ***************************************************************************/
void Parse(const TokenNode *start, int depth) {

	TokenNode *tp;

	if(!start) {
		return;
	}

	if(start->type == TOK_JWM) {
		for(tp = start->subnodeHead; tp; tp = tp->next) {
			switch(tp->type) {
			case TOK_BORDER:
				ParseBorder(tp);
				break;
			case TOK_INCLUDE:
				ParseInclude(tp, depth);
				break;
			case TOK_TRAY:
				ParseTray(tp);
				break;
			case TOK_PAGER:
				ParsePager(tp);
				break;
			case TOK_POPUP:
				ParsePopup(tp);
				break;
			case TOK_LOAD:
				ParseLoad(tp);
				break;
			case TOK_CLOCK:
				ParseClock(tp);
				break;
			case TOK_MENU:
				ParseMenu(tp);
				break;
			case TOK_DESKTOPCOUNT:
				SetDesktopCount(tp->value);
				break;
			case TOK_DOUBLECLICKSPEED:
				SetDoubleClickSpeed(tp->value);
				break;
			case TOK_DOUBLECLICKDELTA:
				SetDoubleClickDelta(tp->value);
				break;
			case TOK_FOCUSMODEL:
				ParseFocusModel(tp->value);
				break;
			case TOK_SNAPMODE:
				ParseSnapMode(tp);
				break;
			case TOK_MOVEMODE:
				ParseMoveMode(tp);
				break;
			case TOK_RESIZEMODE:
				ParseResizeMode(tp);
				break;
			case TOK_ROOTMENU:
				ParseRootMenu(tp);
				break;
			case TOK_KEY:
				ParseKey(tp);
				break;
			case TOK_GROUP:
				ParseGroup(tp);
				break;
			case TOK_ICONS:
				ParseIcons(tp);
				break;
			case TOK_SHUTDOWNCOMMAND:
				SetShutdownCommand(tp->value);
				break;
			case TOK_STARTUPCOMMAND:
				SetStartupCommand(tp->value);
				break;
			default:
				ParseError("invalid tag in JWM: %s", GetTokenName(tp->type));
				break;
			}
		}
	} else {
		ParseError("invalid start tag: %s", GetTokenName(start->type));
	}

}

/****************************************************************************
 ****************************************************************************/
void ParseFocusModel(const char *model) {
	if(model) {
		if(!strcmp(model, "sloppy")) {
			focusModel = FOCUS_SLOPPY;
		} else if(!strcmp(model, "click")) {
			focusModel = FOCUS_CLICK;
		} else {
			ParseError("invalid focus model: %s", model);
		}
	} else {
		ParseError("focus model not specified");
	}
}

/****************************************************************************
 ****************************************************************************/
void ParseSnapMode(const TokenNode *tp) {
	const char *distance;

	distance = FindAttribute(tp->attributes, "distance");
	if(distance) {
		SetSnapDistance(distance);
	} else {
		SetDefaultSnapDistance();
	}

	if(tp->value) {
		if(!strcmp(tp->value, "none")) {
			SetSnapMode(SNAP_NONE);
		} else if(!strcmp(tp->value, "screen")) {
			SetSnapMode(SNAP_SCREEN);
		} else if(!strcmp(tp->value, "border")) {
			SetSnapMode(SNAP_BORDER);
		} else {
			ParseError("invalid snap mode: %s", tp->value);
		}
	} else {
		ParseError("snap mode not specified");
	}
}

/****************************************************************************
 ****************************************************************************/
void ParseMoveMode(const TokenNode *tp) {
	if(tp->value) {
		if(!strcmp(tp->value, "outline")) {
			SetMoveMode(MOVE_OUTLINE);
		} else if(!strcmp(tp->value, "opaque")) {
			SetMoveMode(MOVE_OPAQUE);
		} else {
			ParseError("invalid move mode: %s", tp->value);
		}
	} else {
		ParseError("move mode not specified");
	}
}

/****************************************************************************
 ****************************************************************************/
void ParseResizeMode(const TokenNode *tp) {
	if(tp->value) {
		if(!strcmp(tp->value, "outline")) {
			SetResizeMode(RESIZE_OUTLINE);
		} else if(!strcmp(tp->value, "opaque")) {
			SetResizeMode(RESIZE_OPAQUE);
		} else {
			ParseError("invalid resize mode: %s", tp->value);
		}
	} else {
		ParseError("resize mode not specified");
	}
}

/****************************************************************************
 ****************************************************************************/
void ParseRootMenu(const TokenNode *start) {

	char *title;
	char *value;
	MenuType *menu;

	title = FindAttribute(start->attributes, LABEL_ATTRIBUTE);
	if(title) {
		SetMenuTitle(title);
	}
	value = FindAttribute(start->attributes, ICON_ATTRIBUTE);
	if(value) {
		SetMenuIcon(value);
	}
	value = FindAttribute(start->attributes, ONROOT_ATTRIBUTE);
	if(value && !strcmp(value, FALSE_VALUE)) {
		SetShowMenuOnRoot(0);
	} else {
		SetShowMenuOnRoot(1);
	}

	menu = Allocate(sizeof(MenuType));

	value = FindAttribute(start->attributes, LABELED_ATTRIBUTE);
	if(value && !strcmp(value, TRUE_VALUE)) {
		if(title) {
			menu->label = Allocate(strlen(title) + 1);
			strcpy(menu->label, title);
		} else {
			menu->label = Allocate(strlen(DEFAULT_TITLE) + 1);
			strcpy(menu->label, DEFAULT_TITLE);
		}
	} else {
		menu->label = NULL;
	}

	ParseMenuItem(start->subnodeHead, menu);
	SetRootMenu(menu);

}

/****************************************************************************
 ****************************************************************************/
MenuItemType *InsertMenuItem(MenuItemType *last) {

	MenuItemType *item;

	item = Allocate(sizeof(MenuItemType));
	item->name = NULL;
	item->iconName = NULL;
	item->command = NULL;
	item->submenu = NULL;

	item->next = NULL;
	if(last) {
		last->next = item;
	}

	return item;

}

/****************************************************************************
 ****************************************************************************/
void ParseMenuItem(const TokenNode *start, MenuType *menu) {

	MenuItemType *last;
	MenuType *child;
	const char *value;

	Assert(menu);

	menu->offsets = NULL;
	menu->items = NULL;
	last = NULL;
	while(start) {
		switch(start->type) {
		case TOK_MENU:

			last = InsertMenuItem(last);
			if(!menu->items) {
				menu->items = last;
			}

			value = FindAttribute(start->attributes, LABEL_ATTRIBUTE);
			if(value) {
				last->name = Allocate(strlen(value) + 1);
				strcpy(last->name, value);
			}

			value = FindAttribute(start->attributes, ICON_ATTRIBUTE);
			if(value) {
				last->iconName = Allocate(strlen(value) + 1);
				strcpy(last->iconName, value);
			}

			last->submenu = Allocate(sizeof(MenuType));
			child = last->submenu;

			value = FindAttribute(start->attributes, LABELED_ATTRIBUTE);
			if(value && !strcmp(value, TRUE_VALUE)) {
				if(last->name) {
					child->label = Allocate(strlen(last->name) + 1);
					strcpy(child->label, last->name);
				} else {
					child->label = Allocate(strlen(DEFAULT_TITLE) + 1);
					strcpy(child->label, DEFAULT_TITLE);
				}
			} else {
				child->label = NULL;
			}

			ParseMenuItem(start->subnodeHead, last->submenu);

			break;
		case TOK_PROGRAM:

			last = InsertMenuItem(last);
			if(!menu->items) {
				menu->items = last;
			}

			value = FindAttribute(start->attributes, LABEL_ATTRIBUTE);
			if(value) {
				last->name = Allocate(strlen(value) + 1);
				strcpy(last->name, value);
			} else if(start->value) {
				last->name = Allocate(strlen(start->value) + 1);
				strcpy(last->name, start->value);
			}

			value = FindAttribute(start->attributes, ICON_ATTRIBUTE);
			if(value) {
				last->iconName = Allocate(strlen(value) + 1);
				strcpy(last->iconName, value);
			}

			if(start->value) {
				last->command = Allocate(strlen(start->value) + 1);
				strcpy(last->command, start->value);
			}

			break;
		case TOK_SEPARATOR:

			last = InsertMenuItem(last);
			if(!menu->items) {
				menu->items = last;
			}

			break;
		case TOK_EXIT:

			last = InsertMenuItem(last);
			if(!menu->items) {
				menu->items = last;
			}

			value = FindAttribute(start->attributes, CONFIRM_ATTRIBUTE);
			if(value && !strcmp(value, FALSE_VALUE)) {
				SetShowExitConfirmation(0);
			} else {
				SetShowExitConfirmation(1);
			}

			value = FindAttribute(start->attributes, LABEL_ATTRIBUTE);
			if(!value) {
				value = EXIT_NAME;
			}
			last->name = Allocate(strlen(value) + 1);
			strcpy(last->name, value);

			if(start->value) {
				last->command = Allocate(strlen(EXIT_COMMAND)
					+ strlen(start->value) + 2);
				strcpy(last->command, EXIT_COMMAND);
				strcat(last->command, ":");
				strcat(last->command, start->value);
			} else {
				last->command = Allocate(strlen(EXIT_COMMAND) + 1);
				strcpy(last->command, EXIT_COMMAND);
			}

			value = FindAttribute(start->attributes, ICON_ATTRIBUTE);
			if(value) {
				last->iconName = Allocate(strlen(value) + 1);
				strcpy(last->iconName, value);
			}

			break;
		case TOK_RESTART:

			last = InsertMenuItem(last);
			if(!menu->items) {
				menu->items = last;
			}

			value = FindAttribute(start->attributes, LABEL_ATTRIBUTE);
			if(!value) {
				value = RESTART_NAME;
			}
			last->name = Allocate(strlen(value) + 1);
			strcpy(last->name, value);

			value = FindAttribute(start->attributes, ICON_ATTRIBUTE);
			if(value) {
				last->iconName = Allocate(strlen(value) + 1);
				strcpy(last->iconName, value);
			}

			last->command = Allocate(strlen(RESTART_COMMAND) + 1);
			strcpy(last->command, RESTART_COMMAND);

			break;
		default:
			ParseError("invalid tag in Menu: %s", GetTokenName(start->type));
			break;
		}
		start = start->next;
	}
}

/****************************************************************************
 ****************************************************************************/
void ParseKey(const TokenNode *tp) {
	const char *key;
	const char *mask;
	const char *action;
	const char *command;
	KeyType k;

	Assert(tp);

	mask = FindAttribute(tp->attributes, "mask");
	key = FindAttribute(tp->attributes, "key");
	if(key == NULL) {
		ParseError("no key specified for Key");
		return;
	}

	action = tp->value;
	if(action == NULL) {
		ParseError("no action specified for Key");
		return;
	}

	command = NULL;
	if(!strcmp(action, "up")) {
		k = KEY_UP;
	} else if(!strcmp(action, "down")) {
		k = KEY_DOWN;
	} else if(!strcmp(action, "right")) {
		k = KEY_RIGHT;
	} else if(!strcmp(action, "left")) {
		k = KEY_LEFT;
	} else if(!strcmp(action, "escape")) {
		k = KEY_ESC;
	} else if(!strcmp(action, "select")) {
		k = KEY_ENTER;
	} else if(!strcmp(action, "next")) {
		k = KEY_NEXT;
	} else if(!strcmp(action, "close")) {
		k = KEY_CLOSE;
	} else if(!strcmp(action, "minimize")) {
		k = KEY_MIN;
	} else if(!strcmp(action, "maximize")) {
		k = KEY_MAX;
	} else if(!strcmp(action, "shade")) {
		k = KEY_SHADE;
	} else if(!strcmp(action, "move")) {
		k = KEY_MOVE;
	} else if(!strcmp(action, "resize")) {
		k = KEY_RESIZE;
	} else if(!strcmp(action, "root")) {
		k = KEY_ROOT;
	} else if(!strcmp(action, "window")) {
		k = KEY_WIN;
	} else if(!strcmp(action, "restart")) {
		k = KEY_RESTART;
	} else if(!strcmp(action, "exit")) {
		k = KEY_EXIT;
	} else if(!strcmp(action, "desktop")
		|| !strcmp(action, "desktop#")) {
		k = KEY_DESKTOP;
	} else if(!strncmp(action, "exec:", 5)) {
		k = KEY_EXEC;
		command = action + 5;
	} else {
		ParseError("invalid action: \"%s\"", action);
		return;
	}

	InsertBinding(k, mask, key, command);

}

/***************************************************************************
 ***************************************************************************/
void ParseBorder(const TokenNode *tp) {
	const TokenNode *np;
	const char *aa;

	Assert(tp);

	for(np = tp->subnodeHead; np; np = np->next) {
		switch(np->type) {
		case TOK_FONT:
			aa = FindAttribute(np->attributes, ANTIALIAS_ATTRIBUTE);
			if(aa && !strcmp(aa, FALSE_VALUE)) {
				SetFont(FONT_BORDER, np->value, 0);
			} else {
				SetFont(FONT_BORDER, np->value, 1);
			}
			break;
		case TOK_WIDTH:
			SetBorderWidth(np->value);
			break;
		case TOK_HEIGHT:
			SetTitleHeight(np->value);
			break;
		case TOK_FOREGROUND:
			SetColor(COLOR_BORDER_FG, np->value);
			break;
		case TOK_BACKGROUND:
			SetColor(COLOR_BORDER_BG, np->value);
			break;
		case TOK_ACTIVEFOREGROUND:
			SetColor(COLOR_BORDER_ACTIVE_FG, np->value);
			break;
		case TOK_ACTIVEBACKGROUND:
			SetColor(COLOR_BORDER_ACTIVE_BG, np->value);
			break;
		default:
			ParseError("invalid Border option: %s", GetTokenName(np->type));
			break;
		}
	}

}

/***************************************************************************
 ***************************************************************************/
void ParseInclude(const TokenNode *tp, int depth) {

	char *temp;

	Assert(tp);

	temp = Allocate(strlen(tp->value) + 1);
	strcpy(temp, tp->value);

	ExpandPath(&temp);

	if(!ParseFile(temp, depth)) {
		ParseError("could not open included file %s", temp);
	}
	Release(temp);

}

/***************************************************************************
 ***************************************************************************/
void ParseTray(const TokenNode *tp) {
	const TokenNode *np;
	const char *aa;

	Assert(tp);

	aa = FindAttribute(tp->attributes, AUTOHIDE_ATTRIBUTE);
	if(aa && !strcmp(aa, TRUE_VALUE)) {
		SetAutoHideTray(1);
	} else {
		SetAutoHideTray(0);
	}

	aa = FindAttribute(tp->attributes, MAXWIDTH_ATTRIBUTE);
	if(aa) {
		SetMaxTrayItemWidth(aa);
	}

	aa = FindAttribute(tp->attributes, INSERT_ATTRIBUTE);
	if(aa) {
		SetTrayInsertMode(aa);
	}

	aa = FindAttribute(tp->attributes, LAYER_ATTRIBUTE);
	if(aa) {
		SetTrayLayer(aa);
	}

	for(np = tp->subnodeHead; np; np = np->next) {
		switch(np->type) {
		case TOK_HEIGHT:
			SetTrayHeight(np->value);
			break;
		case TOK_WIDTH:
			SetTrayWidth(np->value);
			break;
		case TOK_ALIGNMENT:
			SetTrayAlignment(np->value);
			break;
		case TOK_FONT:
			aa = FindAttribute(np->attributes, ANTIALIAS_ATTRIBUTE);
			if(aa && !strcmp(aa, FALSE_VALUE)) {
				SetFont(FONT_TRAY, np->value, 0);
			} else {
				SetFont(FONT_TRAY, np->value, 1);
			}
			break;
		case TOK_FOREGROUND:
			SetColor(COLOR_TRAY_FG, np->value);
			break;
		case TOK_BACKGROUND:
			SetColor(COLOR_TRAY_BG, np->value);
			break;
		case TOK_ACTIVEFOREGROUND:
			SetColor(COLOR_TRAY_ACTIVE_FG, np->value);
			break;
		case TOK_ACTIVEBACKGROUND:
			SetColor(COLOR_TRAY_ACTIVE_BG, np->value);
			break;
		default:
			ParseError("invalid Tray option: %s", GetTokenName(np->type));
			break;
		}
	}

}

/***************************************************************************
 ***************************************************************************/
void ParsePager(const TokenNode *tp) {
	const TokenNode *np;

	Assert(tp);

	for(np = tp->subnodeHead; np; np = np->next) {
		switch(np->type) {
		case TOK_OUTLINE:
			SetColor(COLOR_PAGER_OUTLINE, np->value);
			break;
		case TOK_FOREGROUND:
			SetColor(COLOR_PAGER_FG, np->value);
			break;
		case TOK_BACKGROUND:
			SetColor(COLOR_PAGER_BG, np->value);
			break;
		case TOK_ACTIVEFOREGROUND:
			SetColor(COLOR_PAGER_ACTIVE_FG, np->value);
			break;
		case TOK_ACTIVEBACKGROUND:
			SetColor(COLOR_PAGER_ACTIVE_BG, np->value);
			break;
		default:
			ParseError("invalid Pager option: %s", GetTokenName(np->type));
			break;
		}
	}

}

/***************************************************************************
 ***************************************************************************/
void ParsePopup(const TokenNode *tp) {
	const TokenNode *np;
	const char *aa;

	Assert(tp);

	aa = FindAttribute(tp->attributes, ENABLED_ATTRIBUTE);
	if(aa && !strcmp(aa, FALSE_VALUE)) {
		SetPopupEnabled(0);
	} else {
		SetPopupEnabled(1);
	}

	for(np = tp->subnodeHead; np; np = np->next) {
		switch(np->type) {
		case TOK_FONT:
			aa = FindAttribute(np->attributes, ANTIALIAS_ATTRIBUTE);
			if(aa && !strcmp(aa, FALSE_VALUE)) {
				SetFont(FONT_POPUP, np->value, 0);
			} else {
				SetFont(FONT_POPUP, np->value, 1);
			}
			break;
		case TOK_OUTLINE:
			SetColor(COLOR_POPUP_OUTLINE, np->value);
			break;
		case TOK_FOREGROUND:
			SetColor(COLOR_POPUP_FG, np->value);
			break;
		case TOK_BACKGROUND:
			SetColor(COLOR_POPUP_BG, np->value);
			break;
		default:
			ParseError("invalid Popup option: %s", GetTokenName(np->type));
			break;
		}
	}

}

/***************************************************************************
 ***************************************************************************/
void ParseLoad(const TokenNode *tp) {
#ifdef SHOW_LOAD
	const TokenNode *np;
	const char *aa;

	Assert(tp);

	aa = FindAttribute(tp->attributes, ENABLED_ATTRIBUTE);
	if(aa && !strcmp(aa, FALSE_VALUE)) {
		SetLoadEnabled(0);
	} else {
		SetLoadEnabled(1);
	}

	for(np = tp->subnodeHead; np; np = np->next) {
		switch(np->type) {
		case TOK_PROGRAM:
			SetLoadProgram(np->value);
			break;
		case TOK_OUTLINE:
			SetColor(COLOR_LOAD_OUTLINE, np->value);
			break;
		case TOK_FOREGROUND:
			SetColor(COLOR_LOAD_FG, np->value);
			break;
		case TOK_BACKGROUND:
			SetColor(COLOR_LOAD_BG, np->value);
			break;
		default:
			ParseError("invalid Load option: %s", GetTokenName(np->type));
			break;
		}
	}
#endif /* SHOW_LOAD */
}

/****************************************************************************
 ****************************************************************************/
void ParseClock(const TokenNode *tp) {
	const TokenNode *np;
	const char *aa;

	Assert(tp);

	aa = FindAttribute(tp->attributes, FORMAT_ATTRIBUTE);
	SetClockFormat(aa);

	aa = FindAttribute(tp->attributes, ENABLED_ATTRIBUTE);
	if(aa && !strcmp(aa, FALSE_VALUE)) {
		SetClockEnabled(0);
	} else {
		SetClockEnabled(1);
	}

	for(np = tp->subnodeHead; np; np = np->next) {
		switch(np->type) {
		case TOK_PROGRAM:
			SetClockProgram(np->value);
			break;
		default:
			ParseError("invalid Clock option: %s", GetTokenName(np->type));
			break;
		}
	}

}

/***************************************************************************
 ***************************************************************************/
void ParseMenu(const TokenNode *tp) {
	const TokenNode *np;
	const char *aa;

	Assert(tp);

	for(np = tp->subnodeHead; np; np = np->next) {
		switch(np->type) {
		case TOK_FONT:
			aa = FindAttribute(np->attributes, ANTIALIAS_ATTRIBUTE);
			if(aa && !strcmp(aa, FALSE_VALUE)) {
				SetFont(FONT_MENU, np->value, 0);
			} else {
				SetFont(FONT_MENU, np->value, 1);
			}
			break;
		case TOK_FOREGROUND:
			SetColor(COLOR_MENU_FG, np->value);
			break;
		case TOK_BACKGROUND:
			SetColor(COLOR_MENU_BG, np->value);
			break;
		case TOK_ACTIVEFOREGROUND:
			SetColor(COLOR_MENU_ACTIVE_FG, np->value);
			break;
		case TOK_ACTIVEBACKGROUND:
			SetColor(COLOR_MENU_ACTIVE_BG, np->value);
			break;
		default:
			ParseError("invalid Menu option: %s", GetTokenName(np->type));
			break;
		}
	}

}

/***************************************************************************
 ***************************************************************************/
void ParseGroup(const TokenNode *tp) {
	const TokenNode *np;
	struct GroupType *group;

	Assert(tp);

	group = CreateGroup();

	for(np = tp->subnodeHead; np; np = np->next) {
		switch(np->type) {
		case TOK_CLASS:
			AddGroupClass(group, np->value);
			break;
		case TOK_NAME:
			AddGroupName(group, np->value);
			break;
		case TOK_OPTION:
			ParseGroupOption(group, np->value);
			break;
		default:
			ParseError("invalid Group setting: %s", GetTokenName(np->type));
			break;
		}
	}

}

/***************************************************************************
 ***************************************************************************/
void ParseGroupOption(struct GroupType *group, const char *option) {

	if(!strcmp(option, "sticky")) {
		AddGroupOption(group, OPTION_STICKY);
	} else if(!strcmp(option, "nolist")) {
		AddGroupOption(group, OPTION_NOLIST);
	} else if(!strcmp(option, "border")) {
		AddGroupOption(group, OPTION_BORDER);
	} else if(!strcmp(option, "noborder")) {
		AddGroupOption(group, OPTION_NOBORDER);
	} else if(!strcmp(option, "title")) {
		AddGroupOption(group, OPTION_TITLE);
	} else if(!strcmp(option, "notitle")) {
		AddGroupOption(group, OPTION_NOTITLE);
	} else if(!strncmp(option, "layer:", 6)) {
		AddGroupOptionValue(group, OPTION_LAYER, option + 6);
	} else if(!strncmp(option, "desktop:", 8)) {
		AddGroupOptionValue(group, OPTION_DESKTOP, option + 8);
	} else if(!strncmp(option, "icon:", 5)) {
		AddGroupOptionValue(group, OPTION_ICON, option + 5);
	} else {
		ParseError("invalid Group Option: %s", option);
	}

}

/****************************************************************************
 ****************************************************************************/
void ParseIcons(const TokenNode *tp) {

	TokenNode *np;

	Assert(tp);

	for(np = tp->subnodeHead; np; np = np->next) {
		switch(np->type) {
		case TOK_ICONPATH:
			AddIconPath(np->value);
			break;
		default:
			ParseError("invalid Icons setting: %s", GetTokenName(np->type));
			break;
		}
	}

}

/***************************************************************************
 ***************************************************************************/
char *FindAttribute(AttributeNode *ap, const char *name) {

	while(ap) {
		if(!strcmp(name, ap->name)) {
			return ap->value;
		}
		ap = ap->next;
	}

	return NULL;
}

/***************************************************************************
 ***************************************************************************/
char *ReadFile(FILE *fd) {

	const int BLOCK_SIZE = 16;

	char *buffer;
	int len, max;
	int ch;

	len = 0;
	max = BLOCK_SIZE;
	buffer = Allocate(max + 1);

	for(;;) {
		ch = fgetc(fd);
		if(ch == EOF) {
			break;
		}
		buffer[len++] = ch;
		if(len >= max) {
			max += BLOCK_SIZE;
			buffer = Reallocate(buffer, max + 1);
		}
	}
	buffer[len] = 0;

	return buffer;
}

/****************************************************************************
 ****************************************************************************/
void ParseError(const char *str, ...) {
	va_list ap;
	va_start(ap, str);

	WarningVA("configuration error", str, ap);

	va_end(ap);
}

/****************************************************************************
 ****************************************************************************/
void SetDesktopCount(const char *value) {
	desktopCount = atoi(value);
	if(desktopCount <= 0) {
		desktopCount = 0;
	}
	if(desktopCount > MAX_DESKTOP_COUNT) {
		desktopCount = MAX_DESKTOP_COUNT;
	}
}


