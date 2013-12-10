/**
 * @file misc.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Miscellaneous functions and macros.
 *
 */

#include "jwm.h"
#include "misc.h"

static char IsSymbolic(char ch);
static char *GetSymbolName(const char *str);
static void ReplaceSymbol(char **str, const char *name, const char *value);

/** Determine if a character is a space character. */
char IsSpace(char ch, unsigned int *lineNumber)
{
   switch(ch) {
   case ' ':
   case '\t':
   case '\r':
      return 1;
   case '\n':
      *lineNumber += 1;
      return 1;
   default:
      return 0;
   }
}

/** Determine if a character is a valid for a shell variable. */
char IsSymbolic(char ch)
{
   if(ch >= 'A' && ch <= 'Z') {
      return 1;
   } else if(ch >= 'a' && ch <= 'z') {
      return 1;
   } else if(ch >= '0' && ch <= '9') {
      return 1;
   } else if(ch == '_') {
      return 1;
   } else {
      return 0;
   }
}

/** Get the name of a shell variable (returns a copy). */
char *GetSymbolName(const char *str)
{

   char *temp;

   if(*str == '$') {
      temp = Allocate(2);
      temp[0] = '$';
      temp[1] = 0;
   } else {
      int stop;
      for(stop = 0; IsSymbolic(str[stop]); stop++);
      temp = Allocate(stop + 1);
      memcpy(temp, str, stop);
      temp[stop] = 0;
   }

   return temp;

}

/** Replace "name" with "value" in str (reallocates if needed). */
void ReplaceSymbol(char **str, const char *name, const char *value)
{

   char *temp;
   int strLength;
   int nameLength;
   int valueLength;
   int x;

   Assert(str);
   Assert(name);

   strLength = strlen(*str);
   nameLength = strlen(name) + 1;
   if(value) {
      valueLength = strlen(value);
   } else {
      valueLength = 0;
   }

   if(valueLength > nameLength) {
      temp = Allocate(strLength - nameLength + valueLength + 1);
      strcpy(temp, *str);
      Release(*str);
      *str = temp;
   }

   temp = strstr(*str, name);
   Assert(temp);
   temp -= 1; /* Account for the "$" */

   if(nameLength > valueLength) {

      /* Move left */
      for(x = 0; temp[x]; x++) {
         temp[x] = temp[x + nameLength - valueLength];
      }
      temp[x] = temp[x + nameLength - valueLength];

   } else if(nameLength < valueLength) {

      /* Move right */
      for(x = strlen(temp); x >= 0; x--) {
         temp[x + valueLength - nameLength] = temp[x];
      }

   }


   if(value) {
      memcpy(temp, value, valueLength);
   }

}

/** Perform shell-like macro path expansion. */
void ExpandPath(char **path)
{

   char *name;
   char *value;
   unsigned int x;

   Assert(path);

   for(x = 0; (*path)[x]; x++) {

      if((*path)[x] == '$') {
         name = GetSymbolName(*path + x + 1);
         value = getenv(name);
         ReplaceSymbol(path, name, value);
         Release(name);
         if(value) {
            x += strlen(value) - 1;
         }
      }

   }

}

/** Trim leading and trailing whitespace from a string. */
void Trim(char *str)
{

   unsigned int length;
   unsigned int start;
   unsigned int x;
   unsigned int line;

   Assert(str);

   length = strlen(str);

   /* Determine how much to cut off of the left. */
   line = 0;
   for(start = 0; IsSpace(str[start], &line); start++);

   /* Trim the left. */
   if(start > 0) {
      length -= start;
      for(x = 0; x < length + 1; x++) {
         str[x] = str[x + start];
      }
   }

   /* Trim the right. */
   while(length > 0 && IsSpace(str[length - 1], &line)) {
      length -= 1;
      str[length] = 0;
   }

}

/** Copy a string. */
char *CopyString(const char *str)
{

   char *temp;
   unsigned int len;

   if(!str) {
      return NULL;
   }

   len = strlen(str) + 1;
   temp = Allocate(len);
   memcpy(temp, str, len);

   return temp;

}

/** Parse a float. */
float ParseFloat(const char *str)
{
   float result;
#if defined(HAVE_SETLOCALE) && defined(ENABLE_NLS)
   setlocale(LC_ALL, "C");
#endif
   result = atof(str);
#if defined(HAVE_SETLOCALE) && defined(ENABLE_NLS)
   setlocale(LC_ALL, "");
#endif
   return result;
}

