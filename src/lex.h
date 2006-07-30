/*****************************************************************************
 * XML lexer header file.
 * Copyright (C) 2004 Joe Wingbermuehle
 *****************************************************************************/

#ifndef LEX_H
#define LEX_H

/****************************************************************************
 * Note: Any change made to this typedef must be reflected in
 * TOKEN_MAP in lex.c.
 ****************************************************************************/
typedef enum {

	TOK_INVALID,

	TOK_ACTIVEBACKGROUND,
	TOK_ACTIVEFOREGROUND,
	TOK_BACKGROUND,
	TOK_BORDERSTYLE,
	TOK_CLASS,
	TOK_CLOCK,
	TOK_CLOCKSTYLE,
	TOK_DESKTOPS,
	TOK_DOCK,
	TOK_DOUBLECLICKSPEED,
	TOK_DOUBLECLICKDELTA,
	TOK_EXIT,
	TOK_FOCUSMODEL,
	TOK_FONT,
	TOK_FOREGROUND,
	TOK_GROUP,
	TOK_HEIGHT,
	TOK_ICONPATH,
	TOK_INCLUDE,
	TOK_JWM,
	TOK_KEY,
	TOK_MENU,
	TOK_MENUSTYLE,
	TOK_MOUSE,
	TOK_MOVEMODE,
	TOK_NAME,
	TOK_OPTION,
	TOK_OUTLINE,
	TOK_PAGER,
	TOK_PAGERSTYLE,
	TOK_POPUP,
	TOK_POPUPSTYLE,
	TOK_PROGRAM,
	TOK_RESIZEMODE,
	TOK_RESTART,
	TOK_ROOTMENU,
	TOK_SEPARATOR,
	TOK_SHUTDOWNCOMMAND,
	TOK_SNAPMODE,
	TOK_STARTUPCOMMAND,
	TOK_SWALLOW,
	TOK_TASKLISTSTYLE,
	TOK_TASKLIST,
	TOK_THEME,
	TOK_THEMEPATH,
	TOK_TRAY,
	TOK_TRAYBUTTON,
	TOK_TRAYBUTTONSTYLE,
	TOK_TRAYSTYLE,
	TOK_WIDTH

} TokenType;

/****************************************************************************
 ****************************************************************************/
typedef struct AttributeNode {

	char *name;
	char *value;
	struct AttributeNode *next;

} AttributeNode;

/****************************************************************************
 ****************************************************************************/
typedef struct TokenNode {

	TokenType type;
	char *invalidName;
	char *value;
	char *fileName;
	int line;
	struct AttributeNode *attributes;
	struct TokenNode *parent;
	struct TokenNode *subnodeHead, *subnodeTail;
	struct TokenNode *next;

} TokenNode;

TokenNode *Tokenize(const char *line, const char *fileName);

const char *GetTokenName(const TokenNode *tp);
const char *GetTokenTypeName(TokenType type);

#endif

