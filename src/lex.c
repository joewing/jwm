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

static TokenNode *CreateNode(TokenNode *parent);
static AttributeNode *CreateAttribute(TokenNode *np); 

static int IsElementEnd(char ch);
static int IsValueEnd(char ch);
static int IsAttributeEnd(char ch);
static int IsSpace(char ch);
static char *ReadElementName(const char *line);
static char *ReadElementValue(const char *line);
static char *ReadAttributeValue(const char *line);
static int ParseEntity(const char *entity, char *ch);
static TokenType LookupType(const char *name);

/*****************************************************************************
 *****************************************************************************/
TokenNode *Tokenize(const char *line) {
	TokenNode *np;
	AttributeNode *ap;
	char *temp;
	int inElement;
	int x;
	int found;

	head = NULL;
	current = NULL;
	inElement = 0;

	x = 0;
	/* Skip any initial white space */
	while(IsSpace(line[x])) ++x;

	/* Skip any XML stuff */
	if(!strncmp(line + x, "<?", 2)) {
		while(line[x]) {
			if(!strncmp(line + x, "?>", 2)) {
				x += 2;
				break;
			}
			++x;
		}
	}

	while(line[x]) {

		do {

			while(IsSpace(line[x])) ++x;

			/* Skip comments */
			found = 0;
			if(!strncmp(line + x, "<!--", 4)) {
				while(line[x]) {
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

						if(current->type != LookupType(temp)) {
							Warning("configuration: close tag \"%s\" does not "
								"match open tag \"%s\"", temp,
								GetTokenName(current->type));
						}

					} else {
						Warning("configuration: unexpected and invalid close tag");
					}

					current = current->parent;
				} else {
					if(temp) {
						Warning("configuration: close tag \"%s\" without open "
							"tag", temp);
					} else {
						Warning("configuration: invalid close tag");
					}
				}

				if(temp) {
					x += strlen(temp);
					Release(temp);
				}

			} else {
				np = current;
				current = NULL;
				np = CreateNode(np);
				temp = ReadElementName(line + x);
				if(temp) {
					x += strlen(temp);
					np->type = LookupType(temp);
					Release(temp);
				} else {
					Warning("configuration: invalid open tag");
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
					Warning("configuration: invalid tag");
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
					ap->value = ReadAttributeValue(line + x);
					if(ap->value) {
						x += strlen(ap->value);
					}
					if(line[x] == '\"') {
						++x;
					}
				}
			} else {
				temp = ReadElementValue(line + x);
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
							Warning("configuration: unexpected text: \"%s\"",
								temp);
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
int ParseEntity(const char *entity, char *ch) {
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
		Warning("configuration: invalid entity: \"%s\"", temp);
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
int IsSpace(char ch) {
	switch(ch) {
	case ' ':
	case '\t':
	case '\n':
	case '\r':
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
char *ReadElementValue(const char *line) {
	char *buffer;
	char ch;
	int len, max;
	int x;

	len = 0;
	max = BLOCK_SIZE;
	buffer = Allocate(max + 1);

	for(x = 0; !IsValueEnd(line[x]); x++) {
		if(line[x] == '&') {
			x += ParseEntity(line + x, &ch) - 1;
			if(ch) {
				buffer[len] = ch;
			} else {
				buffer[len] = line[x];
			}
		} else {
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
char *ReadAttributeValue(const char *line) {
	char *buffer;
	char ch;
	int len, max;
	int x;

	len = 0;
	max = BLOCK_SIZE;
	buffer = Allocate(max + 1);

	for(x = 0; !IsAttributeEnd(line[x]); x++) {
		if(line[x] == '&') {
			x += ParseEntity(line + x, &ch) - 1;
			if(ch) {
				buffer[len] = ch;
			} else {
				buffer[len] = line[x];
			}
		} else {
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
TokenType LookupType(const char *name) {
	int x;

	Assert(name);

	for(x = 0; x < sizeof(TOKEN_MAP) / sizeof(char*); x++) {
		if(!strcmp(name, TOKEN_MAP[x])) {
			return x;
		}
	}

	return TOK_INVALID;

}

/*****************************************************************************
 *****************************************************************************/
const char *GetTokenName(TokenType type) {
	if(type < 0 || type >= sizeof(TOKEN_MAP) / sizeof(const char*)) {
		return "[invalid]";
	} else {
		return TOKEN_MAP[type];
	}
}

/*****************************************************************************
 *****************************************************************************/
TokenNode *CreateNode(TokenNode *parent) {
	TokenNode *np;

	np = Allocate(sizeof(TokenNode));
	np->type = TOK_INVALID;
	np->value = NULL;
	np->attributes = NULL;
	np->subnodeHead = NULL;
	np->subnodeTail = NULL;
	np->parent = parent;
	np->next = NULL;

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


