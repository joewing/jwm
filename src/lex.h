/**
 * @file lex.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief XML lexer header file.
 *
 */

#ifndef LEX_H
#define LEX_H

/** Tokens.
 * Note that any change made to this typedef must be reflected in
 * TOKEN_MAP in lex.c.
 */
typedef enum {

   TOK_INVALID,

   TOK_ACTIVE,
   TOK_ACTIVEBACKGROUND,
   TOK_ACTIVEFOREGROUND,
   TOK_BACKGROUND,
   TOK_BORDER,
   TOK_BUTTONCLOSE,
   TOK_BUTTONMAX,
   TOK_BUTTONMAXACTIVE,
   TOK_BUTTONMIN,
   TOK_CLASS,
   TOK_CLOCK,
   TOK_CLOCKSTYLE,
   TOK_CLOSE,
   TOK_DESKTOPS,
   TOK_DESKTOP,
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
   TOK_INACTIVE,
   TOK_INCLUDE,
   TOK_JWM,
   TOK_KEY,
   TOK_KILL,
   TOK_LAYER,
   TOK_MAXIMIZE,
   TOK_MENU,
   TOK_MENUSTYLE,
   TOK_MINIMIZE,
   TOK_MOVE,
   TOK_MOVEMODE,
   TOK_NAME,
   TOK_OPACITY,
   TOK_OPTION,
   TOK_OUTLINE,
   TOK_PAGER,
   TOK_PAGERSTYLE,
   TOK_POPUP,
   TOK_POPUPSTYLE,
   TOK_PROGRAM,
   TOK_RESIZE,
   TOK_RESIZEMODE,
   TOK_RESTART,
   TOK_RESTARTCOMMAND,
   TOK_ROOTMENU,
   TOK_SENDTO,
   TOK_SEPARATOR,
   TOK_SHADE,
   TOK_SHUTDOWNCOMMAND,
   TOK_SNAPMODE,
   TOK_STARTUPCOMMAND,
   TOK_STICK,
   TOK_SWALLOW,
   TOK_TASKLISTSTYLE,
   TOK_TASKLIST,
   TOK_TEXT,
   TOK_TITLE,
   TOK_TRAY,
   TOK_TRAYBUTTON,
   TOK_TRAYBUTTONSTYLE,
   TOK_TRAYSTYLE,
   TOK_WIDTH,
   TOK_WINDOWSTYLE

} TokenType;

/** Structure to represent an XML attribute. */
typedef struct AttributeNode {

   char *name;                  /**< The name of the attribute. */
   char *value;                 /**< The value for the attribute. */
   struct AttributeNode *next;  /**< The next attribute in the list. */

} AttributeNode;

/** Structure to represent an XML tag. */
typedef struct TokenNode {

   TokenType type;            /**< Tag type. */
   char *invalidName;         /**< Name of the tag if invalid. */
   char *value;               /**< Body of the tag. */
   char *fileName;            /**< Name of the file containing this tag. */
   int line;                  /**< Line number of the start of this tag. */
   struct AttributeNode *attributes;   /**< Linked list of attributes. */
   struct TokenNode *parent;           /**< Parent tag. */
   struct TokenNode *subnodeHead;      /**< Start of children. */
   struct TokenNode *subnodeTail;      /**< End of children. */
   struct TokenNode *next;             /**< Next tag at the current level. */

} TokenNode;

/** Tokenize a buffer.
 * @param line The buffer to tokenize.
 * @param fileName The name of the file for error reporting.
 * @return A linked list of tokens from the buffer.
 */
TokenNode *Tokenize(const char *line, const char *fileName);

/** Get a string represention of a token.
 * This is identical to GetTokenTypeName if tp is a valid token.
 * @param tp The token node.
 * @return The name (never NULL).
 */
const char *GetTokenName(const TokenNode *tp);

/** Get a string represention of a token.
 * @param type The token.
 * @return The name (never NULL).
 */
const char *GetTokenTypeName(TokenType type);

#endif /* LEX_H */

