/*****************************************************************************
 * XML lexer header file.
 * Copyright (C) 2004 Joe Wingbermuehle
 *****************************************************************************/

#ifndef LEX_H
#define LEX_H

TokenNode *Tokenize(const char *line);

const char *GetTokenName(TokenType type);

#endif

