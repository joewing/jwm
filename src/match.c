/**
 * @file match.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Expression matching.
 *
 */

#include "jwm.h"
#include "match.h"

#include <sys/types.h>
#include <regex.h>

/** Determine if expression matches pattern. */
int Match(const char *pattern, const char *expression) {

	regex_t re;
	regmatch_t rm;

   if(!pattern && !expression) {
      return 1;
   } else if(!pattern || !expression) {
      return 0;
   }


	if(regcomp(&re, pattern, REG_EXTENDED) != 0) {
		return 0;
	}

	if(regexec(&re, expression, 0, &rm, 0) != 0) {
		return 0;
	} else {
		return 1;
	}

}

