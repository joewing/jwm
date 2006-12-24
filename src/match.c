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

/** State data for pattern matching. */
typedef struct MatchStateType {
   const char *pattern;
   const char *expression;
   int patternOffset;
   int expressionOffset;
   int expressionLength;
} MatchStateType;

static int DoMatch(MatchStateType state);

/** Determine if expression matches pattern. */
int Match(const char *pattern, const char *expression) {

   MatchStateType state;

   if(!pattern && !expression) {
      return 1;
   } else if(!pattern || !expression) {
      return 0;
   }

   state.pattern = pattern;
   state.expression = expression;
   state.patternOffset = 0;
   state.expressionOffset = 0;
   state.expressionLength = strlen(expression);

   return DoMatch(state);

}

/** Match helper function. */
int DoMatch(MatchStateType state) {

   char p, e;

   for(;;) {
      p = state.pattern[state.patternOffset];
      e = state.expression[state.expressionOffset];

      if(p == 0 && e == 0) {
         return 1;
      } else if(p == 0 || e == 0) {
         return 0;
      }

      switch(p) {
      case '*':
         ++state.patternOffset;
         while(state.expressionOffset < state.expressionLength) {
            if(DoMatch(state)) {
               return 1;
            }
            ++state.expressionOffset;
         }
         return 0;
      default:
         if(p == e) {
            ++state.patternOffset;
            ++state.expressionOffset;
            break;
         } else {
            return 0;
         }
      }
   }

}


