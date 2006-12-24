/**
 * @file debug.h
 * @author Joe Wingbermuehle
 * @date 2003-2006
 *
 * @brief Header for the debug functions.
 *
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef DEBUG_H
#define DEBUG_H

#ifndef MAKE_DEPEND
#   include <stdarg.h>
#   include <stdio.h>
#   include <stdlib.h>
#   include <string.h>
#   ifdef HAVE_ALLOCA_H
#      include <alloca.h>
#   endif
#endif /* MAKE_DEPEND */

void Debug(const char *str, ...);

#ifdef HAVE_ALLOCA_H

#   define AllocateStack( x ) alloca( x )
#   define ReleaseStack( x ) ((void)0)

#else

#   define AllocateStack( x ) Allocate( x )
#   define ReleaseStack( x ) Release( x )

#endif

#ifdef DEBUG

#   define Assert( x ) \
      if(!( x )) {     \
         Debug("ASSERT FAILED: %s[%u]", __FILE__, __LINE__ ); \
         abort(); \
      }

#   define SetCheckpoint() \
      DEBUG_SetCheckpoint( __FILE__, __LINE__ )
#   define ShowCheckpoint() \
      DEBUG_ShowCheckpoint()

#   define StartDebug() \
      DEBUG_StartDebug( __FILE__, __LINE__ )
#   define StopDebug() \
      DEBUG_StopDebug( __FILE__, __LINE__ )

#   define Allocate( x ) \
      DEBUG_Allocate( (x), __FILE__, __LINE__ )
#   define Reallocate( x, y ) \
      DEBUG_Reallocate( (x), (y), __FILE__, __LINE__ )
#   define Release( x ) \
      DEBUG_Release( (void*)(& x), __FILE__, __LINE__ )

   void DEBUG_SetCheckpoint(const char*, unsigned int);
   void DEBUG_ShowCheckpoint();

   void DEBUG_StartDebug(const char*, unsigned int);
   void DEBUG_StopDebug(const char*, unsigned int);

   void *DEBUG_Allocate(size_t, const char*, unsigned int);
   void *DEBUG_Reallocate(void*, size_t, const char*, unsigned int);
   void DEBUG_Release(void**, const char*, unsigned int);

#else /* DEBUG */

#   define Assert( x )           ((void)0)

#   define SetCheckpoint()       ((void)0)
#   define ShowCheckpoint()      ((void)0)

#   define StartDebug()          ((void)0)
#   define StopDebug()           ((void)0)

#   define Allocate( x )         malloc( (x) )
#   define Reallocate( x, y )    realloc( (x), (y) )
#   define Release( x )          free( (x) )

#endif /* DEBUG */

#endif /* DEBUG_H */

