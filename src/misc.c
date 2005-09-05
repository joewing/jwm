/****************************************************************************
 ****************************************************************************/

#include "jwm.h"

static int IsSymbolic(char ch);
static char *GetSymbolName(const char *str);
static void ReplaceSymbol(char **str, const char *name, const char *value);

/****************************************************************************
 ****************************************************************************/
int IsSymbolic(char ch) {

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


/****************************************************************************
 ****************************************************************************/
char *GetSymbolName(const char *str) {

	char *temp;
	int stop;

	if(*str == '$') {
		temp = Allocate(2);
		temp[0] = '$';
		temp[1] = 0;
	} else {
		for(stop = 0; IsSymbolic(str[stop]); stop++);
		temp = Allocate(stop + 1);
		memcpy(temp, str, stop);
		temp[stop] = 0;
	}

	return temp;

}

/****************************************************************************
 ****************************************************************************/
void ReplaceSymbol(char **str, const char *name, const char *value) {

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
	--temp; /* Account for the "$" */

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


/****************************************************************************
 ****************************************************************************/
void ExpandPath(char **path) {

	char *name;
	char *value;
	int x;

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

