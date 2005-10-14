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
static const char *LABELED_ATTRIBUTE = "labeled";
static const char *ONROOT_ATTRIBUTE = "onroot";
static const char *LAYER_ATTRIBUTE = "layer";
static const char *LAYOUT_ATTRIBUTE = "layout";
static const char *AUTOHIDE_ATTRIBUTE = "autohide";
static const char *X_ATTRIBUTE = "x";
static const char *Y_ATTRIBUTE = "y";
static const char *WIDTH_ATTRIBUTE = "width";
static const char *HEIGHT_ATTRIBUTE = "height";
static const char *NAME_ATTRIBUTE = "name";
static const char *BORDER_ATTRIBUTE = "border";

static const char *FALSE_VALUE = "false";
static const char *TRUE_VALUE = "true";

static int ParseFile(const char *fileName, int depth);
static char *ReadFile(FILE *fd);

/* Misc. */
static void Parse(const TokenNode *start, int depth);
static void ParseInclude(const TokenNode *tp, int depth);
static void ParseShutdownCommand(const TokenNode *tp);
static void ParseStartupCommand(const TokenNode *tp);

/* Menus. */
static void ParseRootMenu(const TokenNode *start);
static void ParseMenuItem(const TokenNode *start, MenuType *menu);
static MenuItemType *InsertMenuItem(MenuItemType *last);

/* Tray. */
static void ParseTray(const TokenNode *tp);
static void ParsePager(const TokenNode *tp, TrayType *tray);
static void ParseTaskList(const TokenNode *tp, TrayType *tray);
static void ParseSwallow(const TokenNode *tp, TrayType *tray);
static void ParseTrayButton(const TokenNode *tp, TrayType *tray);

/* Groups. */
static void ParseGroup(const TokenNode *tp);
static void ParseGroupOption(struct GroupType *group, const char *option);

/* Style. */
static void ParseBorderStyle(const TokenNode *start);
static void ParseTaskListStyle(const TokenNode *start);
static void ParseTrayStyle(const TokenNode *start);
static void ParsePagerStyle(const TokenNode *start);
static void ParseMenuStyle(const TokenNode *start);
static void ParsePopupStyle(const TokenNode *start);
static void ParseIcons(const TokenNode *tp);

/* Feel. */
static void ParseKey(const TokenNode *tp);
static void ParsePopup(const TokenNode *tp);
static void ParseSnapMode(const TokenNode *tp);
static void ParseMoveMode(const TokenNode *tp);
static void ParseResizeMode(const TokenNode *tp);
static void ParseFocusModel(const TokenNode *tp);

static char *FindAttribute(AttributeNode *ap, const char *name);
static void ReleaseTokens(TokenNode *np);
static void ParseError(const char *str, ...);

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
			case TOK_BORDERSTYLE:
				ParseBorderStyle(tp);
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
				ParseFocusModel(tp);
				break;
			case TOK_GROUP:
				ParseGroup(tp);
				break;
			case TOK_ICONS:
				ParseIcons(tp);
				break;
			case TOK_INCLUDE:
				ParseInclude(tp, depth);
				break;
			case TOK_KEY:
				ParseKey(tp);
				break;
			case TOK_MENUSTYLE:
				ParseMenuStyle(tp);
				break;
			case TOK_MOVEMODE:
				ParseMoveMode(tp);
				break;
			case TOK_PAGERSTYLE:
				ParsePagerStyle(tp);
				break;
			case TOK_POPUP:
				ParsePopup(tp);
				break;
			case TOK_POPUPSTYLE:
				ParsePopupStyle(tp);
				break;
			case TOK_RESIZEMODE:
				ParseResizeMode(tp);
				break;
			case TOK_ROOTMENU:
				ParseRootMenu(tp);
				break;
			case TOK_SHUTDOWNCOMMAND:
				ParseShutdownCommand(tp);
				break;
			case TOK_SNAPMODE:
				ParseSnapMode(tp);
				break;
			case TOK_STARTUPCOMMAND:
				ParseStartupCommand(tp);
				break;
			case TOK_TASKLISTSTYLE:
				ParseTaskListStyle(tp);
				break;
			case TOK_TRAY:
				ParseTray(tp);
				break;
			case TOK_TRAYSTYLE:
				ParseTrayStyle(tp);
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
void ParseFocusModel(const TokenNode *tp) {
	if(tp->value) {
		if(!strcmp(tp->value, "sloppy")) {
			focusModel = FOCUS_SLOPPY;
		} else if(!strcmp(tp->value, "click")) {
			focusModel = FOCUS_CLICK;
		} else {
			ParseError("invalid focus model: \"%s\"", tp->value);
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
void ParseResizeMode(const TokenNode *tp)
{
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

	char *value;
	MenuType *menu;

	value = FindAttribute(start->attributes, ONROOT_ATTRIBUTE);
	if(value && !strcmp(value, FALSE_VALUE)) {
		SetShowMenuOnRoot(0);
	} else {
		SetShowMenuOnRoot(1);
	}

	menu = Allocate(sizeof(MenuType));

	value = FindAttribute(start->attributes, HEIGHT_ATTRIBUTE);
	if(value) {
		menu->itemHeight = atoi(value);
	} else {
		menu->itemHeight = 0;
	}

	value = FindAttribute(start->attributes, LABELED_ATTRIBUTE);
	if(value && !strcmp(value, TRUE_VALUE)) {
		menu->label = Allocate(strlen(DEFAULT_TITLE) + 1);
		strcpy(menu->label, DEFAULT_TITLE);
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

			value = FindAttribute(start->attributes, HEIGHT_ATTRIBUTE);
			if(value) {
				child->itemHeight = atoi(value);
			} else {
				child->itemHeight = menu->itemHeight;
			}

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
void ParseBorderStyle(const TokenNode *tp) {

	const TokenNode *np;

	Assert(tp);

	for(np = tp->subnodeHead; np; np = np->next) {
		switch(np->type) {
		case TOK_FONT:
			SetFont(FONT_BORDER, np->value);
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
void ParseTaskListStyle(const TokenNode *tp) {

	TokenNode *np;

	for(np = tp->subnodeHead; np; np = np->next) {
		switch(np->type) {
		case TOK_FONT:
			SetFont(FONT_TASK, np->value);
			break;
		case TOK_FOREGROUND:
			SetColor(COLOR_TASK_FG, np->value);
			break;
		case TOK_BACKGROUND:
			SetColor(COLOR_TASK_BG, np->value);
			break;
		case TOK_ACTIVEFOREGROUND:
			SetColor(COLOR_TASK_ACTIVE_FG, np->value);
			break;
		case TOK_ACTIVEBACKGROUND:
			SetColor(COLOR_TASK_ACTIVE_BG, np->value);
			break;
		default:
			ParseError("invalid TaskListStyle option: %s",
				GetTokenName(np->type));
			break;
		}
	}

}

/***************************************************************************
 ***************************************************************************/
void ParseTrayStyle(const TokenNode *tp) {

	const TokenNode *np;

	for(np = tp->subnodeHead; np; np = np->next) {
		switch(np->type) {
		case TOK_BACKGROUND:
			SetColor(COLOR_TRAY_BG, np->value);
			break;
		default:
			ParseError("invalid TrayStyle option: %s", GetTokenName(np->type));
			break;
		}
	}

}

/***************************************************************************
 ***************************************************************************/
void ParseTray(const TokenNode *tp) {

	const TokenNode *np;
	const char *attr;
	TrayType *tray;

	Assert(tp);

	tray = CreateTray();

	attr = FindAttribute(tp->attributes, AUTOHIDE_ATTRIBUTE);
	if(attr && !strcmp(attr, TRUE_VALUE)) {
		SetAutoHideTray(tray, 1);
	} else {
		SetAutoHideTray(tray, 0);
	}

	attr = FindAttribute(tp->attributes, X_ATTRIBUTE);
	if(attr) {
		SetTrayX(tray, attr);
	}

	attr = FindAttribute(tp->attributes, Y_ATTRIBUTE);
	if(attr) {
		SetTrayY(tray, attr);
	}

	attr = FindAttribute(tp->attributes, WIDTH_ATTRIBUTE);
	if(attr) {
		SetTrayWidth(tray, attr);
	}

	attr = FindAttribute(tp->attributes, HEIGHT_ATTRIBUTE);
	if(attr) {
		SetTrayHeight(tray, attr);
	}

	attr = FindAttribute(tp->attributes, LAYOUT_ATTRIBUTE);
	if(attr) {
		SetTrayLayout(tray, attr);
	}

	attr = FindAttribute(tp->attributes, LAYER_ATTRIBUTE);
	if(attr) {
		SetTrayLayer(tray, attr);
	}

	attr = FindAttribute(tp->attributes, BORDER_ATTRIBUTE);
	if(attr) {
		SetTrayBorder(tray, attr);
	}

	for(np = tp->subnodeHead; np; np = np->next) {
		switch(np->type) {
		case TOK_PAGER:
			ParsePager(np, tray);
			break;
		case TOK_TASKLIST:
			ParseTaskList(np, tray);
			break;
		case TOK_SWALLOW:
			ParseSwallow(np, tray);
			break;
		case TOK_TRAYBUTTON:
			ParseTrayButton(np, tray);
			break;
		default:
			ParseError("invalid Tray option: %s", GetTokenName(np->type));
			break;
		}
	}

}

/***************************************************************************
 ***************************************************************************/
void ParsePager(const TokenNode *tp, TrayType *tray) {

	TrayComponentType *cp;

	Assert(tp);
	Assert(tray);

	cp = CreatePager();
	AddTrayComponent(tray, cp);

}

/***************************************************************************
 ***************************************************************************/
void ParseTaskList(const TokenNode *tp, TrayType *tray) {

	TrayComponentType *cp;

	Assert(tp);
	Assert(tray);

	cp = CreateTaskBar();
	AddTrayComponent(tray, cp);

}

/***************************************************************************
 ***************************************************************************/
void ParseSwallow(const TokenNode *tp, TrayType *tray) {

	TrayComponentType *cp;
	const char *name;
	const char *temp;
	int width, height;

	Assert(tp);
	Assert(tray);

	name = FindAttribute(tp->attributes, NAME_ATTRIBUTE);
	if(name == NULL) {
		name = tp->value;
	}

	width = 0;
	temp = FindAttribute(tp->attributes, WIDTH_ATTRIBUTE);
	if(temp) {
		width = atoi(temp);
	}

	height = 0;
	temp = FindAttribute(tp->attributes, HEIGHT_ATTRIBUTE);
	if(temp) {
		height = atoi(temp);
	}

	cp = CreateSwallow(name, tp->value, width, height);
	if(cp) {
		AddTrayComponent(tray, cp);
	}

}

/***************************************************************************
 ***************************************************************************/
void ParseTrayButton(const TokenNode *tp, TrayType *tray) {

	TrayComponentType *cp;
	const char *icon;

	Assert(tp);
	Assert(tray);

	icon = FindAttribute(tp->attributes, ICON_ATTRIBUTE);

	cp = CreateTrayButton(icon);
	if(cp) {
		AddTrayComponent(tray, cp);
	}

}

/***************************************************************************
 ***************************************************************************/
void ParsePagerStyle(const TokenNode *tp) {

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
			ParseError("invalid PagerStyle option: %s", GetTokenName(np->type));
			break;
		}
	}

}

/***************************************************************************
 ***************************************************************************/
void ParsePopupStyle(const TokenNode *tp)
{

	const TokenNode *np;

	Assert(tp);

	for(np = tp->subnodeHead; np; np = np->next) {
		switch(np->type) {
		case TOK_FONT:
			SetFont(FONT_POPUP, np->value);
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
			ParseError("invalid PopupStyle option: %s", GetTokenName(np->type));
			break;
		}
	}

}

/***************************************************************************
 ***************************************************************************/
void ParsePopup(const TokenNode *tp)
{
/*TODO*/
}

/***************************************************************************
 ***************************************************************************/
void ParseMenuStyle(const TokenNode *tp)
{

	const TokenNode *np;

	Assert(tp);

	for(np = tp->subnodeHead; np; np = np->next) {
		switch(np->type) {
		case TOK_FONT:
			SetFont(FONT_MENU, np->value);
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
			ParseError("invalid MenuStyle option: %s", GetTokenName(np->type));
			break;
		}
	}

}

/***************************************************************************
 ***************************************************************************/
void ParseGroup(const TokenNode *tp)
{

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
void ParseGroupOption(struct GroupType *group, const char *option)
{

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
void ParseShutdownCommand(const TokenNode *tp)
{
	SetShutdownCommand(tp->value);
}

/****************************************************************************
 ****************************************************************************/
void ParseStartupCommand(const TokenNode *tp)
{
	SetStartupCommand(tp->value);
}

/****************************************************************************
 ****************************************************************************/
void ParseIcons(const TokenNode *tp)
{

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
char *FindAttribute(AttributeNode *ap, const char *name)
{

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
char *ReadFile(FILE *fd)
{

	const int BLOCK_SIZE = 1024;

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
void ParseError(const char *str, ...)
{
	va_list ap;
	va_start(ap, str);
	WarningVA("configuration error", str, ap);
	va_end(ap);
}


