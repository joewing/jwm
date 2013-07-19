/**
 * @file lex.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief XML lexer functions.
 *
 */

#include "jwm.h"
#include "lex.h"
#include "error.h"
#include "misc.h"

/** Amount to increase allocations by when reading text. */
static const int BLOCK_SIZE = 16;

/** Literal names for tokens.
 * This order is important. It must match the order of the enumeration
 * in lex.h.
 */
static const char *TOKEN_MAP[] = {
   "[invalid]",
   "Active",
   "ActiveBackground",
   "ActiveForeground",
   "Background",
   "Border",
   "Class",
   "Clock",
   "ClockStyle",
   "Close",
   "Desktops",
   "Desktop",
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
   "Inactive",
   "Include",
   "JWM",
   "Key",
   "Kill",
   "Layer",
   "Maximize",
   "Menu",
   "MenuStyle",
   "Minimize",
   "Mouse",
   "Move",
   "MoveMode",
   "Name",
   "Opacity",
   "Option",
   "Outline",
   "Pager",
   "PagerStyle",
   "Popup",
   "PopupStyle",
   "Program",
   "Resize",
   "ResizeMode",
   "Restart",
   "RestartCommand",
   "RootMenu",
   "SendTo",
   "Separator",
   "Shade",
   "ShutdownCommand",
   "SnapMode",
   "Spacer",
   "StartupCommand",
   "Stick",
   "Swallow",
   "TaskListStyle",
   "TaskList",
   "Text",
   "Title",
   "Tray",
   "TrayButton",
   "TrayButtonStyle",
   "TrayStyle",
   "Width",
   "WindowStyle"
};

static TokenNode *head, *current;

static TokenNode *CreateNode(TokenNode *parent, const char *file,
                             unsigned int line);
static AttributeNode *CreateAttribute(TokenNode *np); 

static char IsElementEnd(char ch);
static char IsValueEnd(char ch);
static char IsAttributeEnd(char ch);
static char *ReadElementName(const char *line);
static char *ReadValue(const char *line,
                       const char *file,
                       char (*IsEnd)(char),
                       unsigned int *offset,
                       unsigned int *lineNumber);
static char *ReadElementValue(const char *line,
                              const char *file,
                              unsigned int *offset,
                              unsigned int *lineNumber);
static char *ReadAttributeValue(const char *line,
                                const char *file,
                                unsigned int *offset,
                                unsigned int *lineNumber);
static int ParseEntity(const char *entity, char *ch,
                       const char *file, unsigned int line);
static TokenType LookupType(const char *name, TokenNode *np);

/** Tokenize a data. */
TokenNode *Tokenize(const char *line, const char *fileName)
{
   TokenNode *np;
   AttributeNode *ap;
   char *temp;
   unsigned int x;
   unsigned int offset;
   unsigned int lineNumber;
   char inElement;
   char found;

   head = NULL;
   current = NULL;
   inElement = 0;
   lineNumber = 1;

   x = 0;
   /* Skip any initial white space */
   while(IsSpace(line[x], &lineNumber)) {
      x += 1;
   }

   /* Skip any XML stuff */
   if(!strncmp(line + x, "<?", 2)) {
      while(line[x]) {
         if(line[x] == '\n') {
            lineNumber += 1;
         }
         if(!strncmp(line + x, "?>", 2)) {
            x += 2;
            break;
         }
         x += 1;
      }
   }

   while(line[x]) {

      do {

         while(IsSpace(line[x], &lineNumber)) {
            x += 1;
         }

         /* Skip comments */
         found = 0;
         if(!strncmp(line + x, "<!--", 4)) {
            while(line[x]) {
               if(line[x] == '\n') {
                  lineNumber += 1;
               }
               if(!strncmp(line + x, "-->", 3)) {
                  x += 3;
                  found = 1;
                  break;
               }
               x += 1;
            }
         }
      } while(found);

      switch(line[x]) {
      case '<':
         x += 1;
         if(line[x] == '/') {
            x += 1;
            temp = ReadElementName(line + x);

            if(current) {

               if(JLIKELY(temp)) {

                  if(JUNLIKELY(current->type != LookupType(temp, NULL))) {
                     Warning(_("%s[%u]: close tag \"%s\" does not "
                             "match open tag \"%s\""),
                             fileName, lineNumber, temp,
                             GetTokenName(current));
                  }

               } else {
                  Warning(_("%s[%u]: unexpected and invalid close tag"),
                          fileName, lineNumber);
               }

               current = current->parent;
            } else {
               if(temp) {
                  Warning(_("%s[%u]: close tag \"%s\" without open tag"),
                          fileName, lineNumber, temp);
               } else {
                  Warning(_("%s[%u]: invalid close tag"), fileName, lineNumber);
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
            if(JLIKELY(temp)) {
               x += strlen(temp);
               LookupType(temp, np);
               Release(temp);
            } else {
               Warning(_("%s[%u]: invalid open tag"), fileName, lineNumber);
            }
         }
         inElement = 1;
         break;
      case '/':
         if(inElement) {
            x += 1;
            if(JLIKELY(line[x] == '>' && current)) {
               x += 1;
               current = current->parent;
               inElement = 0;
            } else {
               Warning(_("%s[%u]: invalid tag"), fileName, lineNumber);
            }
         } else {
            goto ReadDefault;
         }
         break;
      case '>':
         x += 1;
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
                  x += 1;
               }
               if(line[x] == '\"') {
                  x += 1;
               }
               ap->value = ReadAttributeValue(line + x, fileName,
                                              &offset, &lineNumber);
               x += offset;
               if(line[x] == '\"') {
                  x += 1;
               }
            }
         } else {
            temp = ReadElementValue(line + x, fileName, &offset, &lineNumber);
            x += offset;
            if(temp) {
               if(current) {
                  if(current->value) {
                     current->value = Reallocate(current->value,
                                                 strlen(current->value) +
                                                 strlen(temp) + 1);
                     strcat(current->value, temp);
                     Release(temp);
                  } else {
                     current->value = temp;
                  }
               } else {
                  if(JUNLIKELY(temp[0])) {
                     Warning(_("%s[%u]: unexpected text: \"%s\""),
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

/** Parse an entity reference.
 * The entity value is returned in ch and the length of the entity
 * is returned as the value of the function.
 */
int ParseEntity(const char *entity, char *ch, const char *file,
                unsigned int line)
{
   char *temp;
   unsigned int x;

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
      temp = AllocateStack(x + 2);
      strncpy(temp, entity, x + 1);
      temp[x + 1] = 0;
      Warning(_("%s[%d]: invalid entity: \"%.8s\""), file, line, temp);
      ReleaseStack(temp);
      *ch = '&';
      return 1;
   }
}

/** Determine if ch is the end of a tag/attribute name. */
char IsElementEnd(char ch)
{
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

/** Determine if ch is the end of an attribute value. */
char IsAttributeEnd(char ch)
{
   switch(ch) {
   case 0:
   case '\"':
      return 1;
   default:
      return 0;
   }
}

/** Determine if ch is the end of tag data. */
char IsValueEnd(char ch)
{
   switch(ch) {
   case 0:
   case '<':
      return 1;
   default:
      return 0;
   }
}

/** Get the name of the next element. */
char *ReadElementName(const char *line)
{

   char *buffer;
   unsigned int len;

   /* Get the length of the element. */
   for(len = 0; !IsElementEnd(line[len]); len++);

   /* Allocate space for the element. */
   buffer = Allocate(len + 1);
   memcpy(buffer, line, len);
   buffer[len] = 0;

   return buffer;

}

/** Read the value of an element or attribute. */
char *ReadValue(const char *line,
                const char *file,
                char (*IsEnd)(char),
                unsigned int *offset,
                unsigned int *lineNumber)
{
   char *buffer;
   char ch;
   unsigned int len, max;
   unsigned int x;

   len = 0;
   max = BLOCK_SIZE;
   buffer = Allocate(max + 1);

   for(x = 0; !(IsEnd)(line[x]); x++) {
      if(line[x] == '&') {
         x += ParseEntity(line + x, &ch, file, *lineNumber) - 1;
         if(ch) {
            buffer[len] = ch;
         } else {
            buffer[len] = line[x];
         }
      } else {
         if(line[x] == '\n') {
            *lineNumber += 1;
         }
         buffer[len] = line[x];
      }
      len += 1;
      if(len >= max) {
         max += BLOCK_SIZE;
         buffer = Reallocate(buffer, max + 1);
      }
   }
   buffer[len] = 0;
   Trim(buffer);
   *offset = x;

   return buffer;
}

/** Get the value of the current element. */
char *ReadElementValue(const char *line,
                       const char *file,
                       unsigned int *offset,
                       unsigned int *lineNumber)
{
   return ReadValue(line, file, IsValueEnd, offset, lineNumber);
}

/** Get the value of the current attribute. */
char *ReadAttributeValue(const char *line,
                         const char *file,
                         unsigned int *offset,
                         unsigned int *lineNumber)
{
   return ReadValue(line, file, IsAttributeEnd, offset, lineNumber);
}

/** Get the token for a tag name. */
TokenType LookupType(const char *name, TokenNode *np)
{
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

   if(JUNLIKELY(np)) {
      np->type = TOK_INVALID;
      np->invalidName = CopyString(name);
   }

   return TOK_INVALID;

}

/** Get a string representation of a token. */
const char *GetTokenName(const TokenNode *tp)
{
   if(tp->invalidName) {
      return tp->invalidName;
   } else if(tp->type >= sizeof(TOKEN_MAP) / sizeof(const char*)) {
      return "[invalid]";
   } else {
      return TOKEN_MAP[tp->type];
   }
}

/** Get the string representation of a token. */
const char *GetTokenTypeName(TokenType type) {
   return TOKEN_MAP[type];
}

/** Create an empty XML tag node. */
TokenNode *CreateNode(TokenNode *parent, const char *file,
                      unsigned int line)
{
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

/** Create an empty XML attribute node. */
AttributeNode *CreateAttribute(TokenNode *np)
{
   AttributeNode *ap;
   ap = Allocate(sizeof(AttributeNode));
   ap->name = NULL;
   ap->value = NULL;
   ap->next = np->attributes;
   np->attributes = ap;
   return ap;
}

