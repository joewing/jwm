/*****************************************************************************
 * XML lexer functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 *****************************************************************************/

#include "jwm.h"
#include "lex.h"
#include "error.h"

static const int BLOCK_SIZE = 16;

/* Order is important! The order must match the order in lex.h */
static const char *TOKEN_MAP[] = {
	"[invalid]",
	"ActiveBackground",
	"ActiveForeground",
	"Background",
	"BorderStyle",
	"Class",
	"Clock",
	"ClockStyle",
	"Desktops",
	"Dock",
	"DoubleClickSpeed",
	"DoubleClickDelta",
	"Exit",
	"FocusModel",
	"Font",
	"Foreground",
	"Group",
	"Height",
	"IconPath",
	"Icons",
	"Include",
	"JWM",
	"Key",
	"Menu",
	"MenuStyle",
	"Mouse",
	"MoveMode",
	"Name",
	"Option",
	"Outline",
	"Pager",
	"PagerStyle",
	"Popup",
	"PopupStyle",
	"Program",
	"ResizeMode",
	"Restart",
	"RootMenu",
	"Separator",
	"ShutdownCommand",
	"SnapMode",
	"StartupCommand",
	"Swallow",
	"TaskListStyle",
	"TaskList",
	"Tray",
	"TrayButton",
	"TrayButtonStyle",
	"TrayStyle",
	"Width"
};

static TokenNode *head, *current;

static TokenNode *CreateNode(TokenNode *parent, const char *file, int line);
static AttributeNode *CreateAttribute(TokenNode *np); 

static int IsElementEnd(char ch);
static int IsValueEnd(char ch);
static int IsAttributeEnd(char ch);
static int IsSpace(char ch, int *lineNumber);
static char *ReadElementName(const char *line);
static char *ReadElementValue(const char *line,
	const char *file, int *lineNumber);
static char *ReadAttributeValue(const char *line, const char *file,
	int *lineNumber);
static int ParseEntity(const char *entity, char *ch,
	const char *file, int line);
static TokenType LookupType(const char *name, TokenNode *np);

/*****************************************************************************
 *****************************************************************************/
TokenNode *Tokenize(const char *line, const char *fileName) {

	TokenNode *np;
	AttributeNode *ap;
	char *temp;
	int inElement;
	int x;
	int found;
	int lineNumber;

	head = NULL;
	current = NULL;
	inElement = 0;
	lineNumber = 1;

	x = 0;
	/* Skip any initial white space */
	while(IsSpace(line[x], &lineNumber)) ++x;

	/* Skip any XML stuff */
	if(!strncmp(line + x, "<?", 2)) {
		while(line[x]) {
			if(line[x] == '\n') {
				++lineNumber;
			}
			if(!strncmp(line + x, "?>", 2)) {
				x += 2;
				break;
			}
			++x;
		}
	}

	while(line[x]) {

		do {

			while(IsSpace(line[x], &lineNumber)) ++x;

			/* Skip comments */
			found = 0;
			if(!strncmp(line + x, "<!--", 4)) {
				while(line[x]) {
					if(line[x] == '\n') {
						++lineNumber;
					}
					if(!strncmp(line + x, "-->", 3)) {
						x += 3;
						found = 1;
						break;
					}
					++x;
				}
			}
		} while(found);

		switch(line[x]) {
		case '<':
			++x;
			if(line[x] == '/') {
				++x;
				temp = ReadElementName(line + x);

				if(current) {

					if(temp) {

						if(current->type != LookupType(temp, NULL)) {
							Warning("%s[%d]: close tag \"%s\" does not "
								"match open tag \"%s\"",
								fileName, lineNumber, temp,
								GetTokenName(current));
						}

					} else {
						Warning("%s[%d]: unexpected and invalid close tag",
							fileName, lineNumber);
					}

					current = current->parent;
				} else {
					if(temp) {
						Warning("%s[%d]: close tag \"%s\" without open "
							"tag", fileName, lineNumber, temp);
					} else {
						Warning("%s[%d]: invalid close tag", fileName, lineNumber);
					}
				}

				if(temp) {
					x += strlen(temp);
					Release(temp);
				}

			} else {
				np = current;
				current = NULL;
				np = CreateNode(np, fileName, lineNumber);
				temp = ReadElementName(line + x);
				if(temp) {
					x += strlen(temp);
					LookupType(temp, np);
					Release(temp);
				} else {
					Warning("%s[%d]: invalid open tag", fileName, lineNumber);
				}
			}
			inElement = 1;
			break;
		case '/':
			if(inElement) {
				++x;
				if(line[x] == '>' && current) {
					++x;
					current = current->parent;
					inElement = 0;
				} else {
					Warning("%s[%d]: invalid tag", fileName, lineNumber);
				}
			} else {
				goto ReadDefault;
			}
			break;
		case '>':
			++x;
			inElement = 0;
			break;
		default:
ReadDefault:
			if(inElement) {
				ap = CreateAttribute(current);
				ap->name = ReadElementName(line + x);
				if(ap->name) {
					x += strlen(ap->name);
					if(line[x] == '=') {
						++x;
					}
					if(line[x] == '\"') {
						++x;
					}
					ap->value = ReadAttributeValue(line + x, fileName,
						&lineNumber);
					if(ap->value) {
						x += strlen(ap->value);
					}
					if(line[x] == '\"') {
						++x;
					}
				}
			} else {
				temp = ReadElementValue(line + x, fileName, &lineNumber);
				if(temp) {
					x += strlen(temp);
					if(current) {
						if(current->value) {
							current->value = Reallocate(current->value,
								strlen(current->value) + strlen(temp) + 1);
							strcat(current->value, temp);
							Release(temp);
						} else {
							current->value = temp;
						}
					} else {
						if(temp[0]) {
							Warning("%s[%d]: unexpected text: \"%s\"",
								fileName, lineNumber, temp);
						}
						Release(temp);
					}
				}
			}
			break;
		}
	}

	return head;
}

/*****************************************************************************
 * Parse an entity reference.
 * The entity value is returned in ch and the length of the entity
 * is returned as the value of the function.
 *****************************************************************************/
int ParseEntity(const char *entity, char *ch, const char *file, int line) {
	char *temp;
	int x;

	if(!strncmp("&quot;", entity, 6)) {
		*ch = '\"';
		return 6;
	} else if(!strncmp("&lt;", entity, 4)) {
		*ch = '<';
		return 4;
	} else if(!strncmp("&gt;", entity, 4)) {
		*ch = '>';
		return 4;
	} else if(!strncmp("&amp;", entity, 5)) {
		*ch = '&';
		return 5;
	} else if(!strncmp("&apos;", entity, 6)) {
		*ch = '\'';
		return 6;
	} else {
		for(x = 0; entity[x]; x++) {
			if(entity[x] == ';') {
				break;
			}
		}
		temp = Allocate(x + 2);
		strncpy(temp, entity, x + 1);
		temp[x + 1] = 0;
		Warning("%s[%d]: invalid entity: \"%.8s\"", file, line, temp);
		Release(temp);
		*ch = '&';
		return 1;
	}
}

/*****************************************************************************
 *****************************************************************************/
int IsElementEnd(char ch) {
	switch(ch) {
	case ' ':
	case '\t':
	case '\n':
	case '\r':
	case '\"':
	case '>':
	case '<':
	case '/':
	case '=':
	case 0:
		return 1;
	default:
		return 0;
	}
}

/*****************************************************************************
 *****************************************************************************/
int IsAttributeEnd(char ch) {
	switch(ch) {
	case 0:
	case '\"':
		return 1;
	default:
		return 0;
	}
}

/*****************************************************************************
 *****************************************************************************/
int IsValueEnd(char ch) {
	switch(ch) {
	case 0:
	case '<':
		return 1;
	default:
		return 0;
	}
}

/*****************************************************************************
 *****************************************************************************/
int IsSpace(char ch, int *lineNumber) {
	switch(ch) {
	case ' ':
	case '\t':
	case '\r':
		return 1;
	case '\n':
		++*lineNumber;
		return 1;
	default:
		return 0;
	}
}

/*****************************************************************************
 *****************************************************************************/
char *ReadElementName(const char *line) {
	char *buffer;
	int len, max;
	int x;

	len = 0;
	max = BLOCK_SIZE;
	buffer = Allocate(max + 1);

	for(x = 0; !IsElementEnd(line[x]); x++) {
		buffer[len++] = line[x];
		if(len >= max) {
			max += BLOCK_SIZE;
			buffer = Reallocate(buffer, max + 1);
		}
	}
	buffer[len] = 0;

	return buffer;
}

/*****************************************************************************
 *****************************************************************************/
char *ReadElementValue(const char *line, const char *file, int *lineNumber) {
	char *buffer;
	char ch;
	int len, max;
	int x;

	len = 0;
	max = BLOCK_SIZE;
	buffer = Allocate(max + 1);

	for(x = 0; !IsValueEnd(line[x]); x++) {
		if(line[x] == '&') {
			x += ParseEntity(line + x, &ch, file, *lineNumber) - 1;
			if(ch) {
				buffer[len] = ch;
			} else {
				buffer[len] = line[x];
			}
		} else {
			if(line[x] == '\n') {
				++*lineNumber;
			}
			buffer[len] = line[x];
		}
		++len;
		if(len >= max) {
			max += BLOCK_SIZE;
			buffer = Reallocate(buffer, max + 1);
		}
	}
	buffer[len] = 0;

	return buffer;
}

/*****************************************************************************
 *****************************************************************************/
char *ReadAttributeValue(const char *line, const char *file,
	int *lineNumber) {

	char *buffer;
	char ch;
	int len, max;
	int x;

	len = 0;
	max = BLOCK_SIZE;
	buffer = Allocate(max + 1);

	for(x = 0; !IsAttributeEnd(line[x]); x++) {
		if(line[x] == '&') {
			x += ParseEntity(line + x, &ch, file, *lineNumber) - 1;
			if(ch) {
				buffer[len] = ch;
			} else {
				buffer[len] = line[x];
			}
		} else {
			if(line[x] == '\n') {
				++*lineNumber;
			}
			buffer[len] = line[x];
		}
		++len;
		if(len >= max) {
			max += BLOCK_SIZE;
			buffer = Reallocate(buffer, max + 1);
		}
	}
	buffer[len] = 0;

	return buffer;
}

/*****************************************************************************
 *****************************************************************************/
TokenType LookupType(const char *name, TokenNode *np) {
	unsigned int x;

	Assert(name);

	for(x = 0; x < sizeof(TOKEN_MAP) / sizeof(char*); x++) {
		if(!strcmp(name, TOKEN_MAP[x])) {
			if(np) {
				np->type = x;
			}
			return x;
		}
	}

	if(np) {
		np->type = TOK_INVALID;
		np->invalidName = Allocate(strlen(name) + 1);
		strcpy(np->invalidName, name);
	}

	return TOK_INVALID;

}

/*****************************************************************************
 *****************************************************************************/
const char *GetTokenName(const TokenNode *tp) {
	if(tp->invalidName) {
		return tp->invalidName;
	} else if(tp->type >= sizeof(TOKEN_MAP) / sizeof(const char*)) {
		return "[invalid]";
	} else {
		return TOKEN_MAP[tp->type];
	}
}

/*****************************************************************************
 *****************************************************************************/
TokenNode *CreateNode(TokenNode *parent, const char *file, int line) {
	TokenNode *np;

	np = Allocate(sizeof(TokenNode));
	np->type = TOK_INVALID;
	np->value = NULL;
	np->attributes = NULL;
	np->subnodeHead = NULL;
	np->subnodeTail = NULL;
	np->parent = parent;
	np->next = NULL;

	np->fileName = Allocate(strlen(file) + 1);
	strcpy(np->fileName, file);
	np->line = line;
	np->invalidName = NULL;

	if(!head) {
		head = np;
	}
	if(parent) {
		if(parent->subnodeHead) {
			parent->subnodeTail->next = np;
		} else {
			parent->subnodeHead = np;
		}
		parent->subnodeTail = np;
	} else if(current) {
		current->next = np;
	}
	current = np;

	return np;
}

/*****************************************************************************
 *****************************************************************************/
AttributeNode *CreateAttribute(TokenNode *np) {
	AttributeNode *ap;

	ap = Allocate(sizeof(AttributeNode));
	ap->name = NULL;
	ap->value = NULL;

	ap->next = np->attributes;
	np->attributes = ap;

	return ap;
}


