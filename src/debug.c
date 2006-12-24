/**
 * @file debug.h
 * @author Joe Wingbermuehle
 * @date 2003-2006
 *
 * @brief Debug functions.
 *
 */

#include "debug.h"

/** Emit a message (if compiled with -DDEBUG). */
void Debug(const char *str, ...) {
#ifdef DEBUG

   va_list ap;
   va_start(ap, str);

   Assert(str);

   fprintf(stderr, "DEBUG: ");
   vfprintf(stderr, str, ap);
   fprintf(stderr, "\n");

   va_end(ap);

#endif /* DEBUG */
}

#ifdef DEBUG

#define CHECKPOINT_LIST_SIZE 8

typedef struct MemoryType {
   const char *file;
   unsigned int line;
   size_t size;
   void *pointer;
   struct MemoryType *next;
} MemoryType;

static MemoryType *allocations = NULL;

static const char *checkpointFile[CHECKPOINT_LIST_SIZE];
static unsigned int checkpointLine[CHECKPOINT_LIST_SIZE];
static int checkpointOffset;

/** Start the debugger. */
void DEBUG_StartDebug(const char *file, unsigned int line) {
   int x;

   Debug("%s[%u]: debug mode started", file, line);

   checkpointOffset = 0;
   for(x = 0; x < CHECKPOINT_LIST_SIZE; x++) {
      checkpointFile[x] = NULL;
      checkpointLine[x] = 0;
   }

}

/** Stop the debugger. */
void DEBUG_StopDebug(const char *file, unsigned int line) {
   MemoryType *mp;
   unsigned int count = 0;

   Debug("%s[%u]: debug mode stopped", file, line);

   if(allocations) {
      Debug("MEMORY: memory leaks follow");
      for(mp = allocations; mp; mp = mp->next) {
         Debug("        %u bytes in %s at line %u",
            mp->size, mp->file, mp->line);
         ++count;
      }
      if(count == 1) {
         Debug("MEMORY: 1 memory leak");
      } else {
         Debug("MEMORY: %u memory leaks", count);
      }
   } else {
      Debug("MEMORY: no memory leaks");
   }

}

/** Set a checkpoint. */
void DEBUG_SetCheckpoint(const char *file, unsigned int line) {

   checkpointFile[checkpointOffset] = file;
   checkpointLine[checkpointOffset] = line;

   checkpointOffset = (checkpointOffset + 1) % CHECKPOINT_LIST_SIZE;

}

/** Display the location of the last checkpoint. */
void DEBUG_ShowCheckpoint() {
   int x, offset;

   Debug("CHECKPOINT LIST (oldest)");
   offset = checkpointOffset;
   for(x = 0; x < CHECKPOINT_LIST_SIZE; x++) {
      if(checkpointFile[offset]) {
         Debug("   %s[%u]", checkpointFile[offset], checkpointLine[offset]);
      }
      offset = (offset + 1) % CHECKPOINT_LIST_SIZE;
   }
   Debug("END OF CHECKPOINT LIST (most recent)");

}

/** Allocate memory and log. */
void *DEBUG_Allocate(size_t size, const char *file, unsigned int line) {
   MemoryType *mp;

   if(size <= 0) {
      Debug("MEMORY: %s[%u]: Attempt to allocate %d bytes of memory",
         file, line, size);
   }

   mp = (MemoryType*)malloc(sizeof(MemoryType));
   Assert(mp);

   mp->file = file;
   mp->line = line;
   mp->size = size;

   mp->pointer = malloc(size + sizeof(char));
   if(!mp->pointer) {
      Debug("MEMORY: %s[%u]: Memory allocation failed (%d bytes)",
         file, line, size);
      Assert(0);
   }

   /* Make uninitialized accesses easy to find. */
   memset(mp->pointer, 85, size);

   /* Canary value for buffer overflow checking. */
   ((char*)mp->pointer)[size] = 42;

   mp->next = allocations;
   allocations = mp;

   return mp->pointer;
}

/** Reallocate memory and log. */
void *DEBUG_Reallocate(void *ptr, size_t size, const char *file,
   unsigned int line) {

   MemoryType *mp;

   if(size <= 0) {
      Debug("MEMORY: %s[%u]: Attempt to reallocate %d bytes of memory",
         file, line, size);
   }
   if(!ptr) {
      Debug("MEMORY: %s[%u]: Attempt to reallocate NULL pointer. "
         "Calling Allocate...", file, line);
      return DEBUG_Allocate(size, file, line);
   } else {

      for(mp = allocations; mp; mp = mp->next) {
         if(mp->pointer == ptr) {

            if(((char*)ptr)[mp->size] != 42) {
               Debug("MEMORY: %s[%u]: The canary is dead.", file, line);
            }

            mp->file = file;
            mp->line = line;
            mp->size = size;
            mp->pointer = realloc(ptr, size + sizeof(char));
            if(!mp->pointer) {
               Debug("MEMORY: %s[%u]: Failed to reallocate %d bytes.",
                  file, line, size);
               Assert(0);
            }
            ((char*)mp->pointer)[size] = 42;
            return mp->pointer;
         }
      }

      Debug("MEMORY: %s[%u]: Attempt to reallocate unallocated pointer",
         file, line);
      mp = malloc(sizeof(MemoryType));
      Assert(mp);
      mp->file = file;
      mp->line = line;
      mp->size = size;
      mp->pointer = malloc(size + sizeof(char));
      if(!mp->pointer) {
         Debug("MEMORY: %s[%u]: Failed to reallocate %d bytes.",
            file, line, size);
         Assert(0);
      }
      memset(mp->pointer, 85, size);
      ((char*)mp->pointer)[size] = 42;

      mp->next = allocations;
      allocations = mp;

      return mp->pointer;
   }

}

/** Release memory and log. */
void DEBUG_Release(void **ptr, const char *file, unsigned int line) {
   MemoryType *mp, *last;

   if(!ptr) {
      Debug("MEMORY: %s[%u]: Invalid attempt to release", file, line);
   } else if(!*ptr) {
      Debug("MEMORY: %s[%u]: Attempt to delete NULL pointer",
         file, line);
   } else {
      last = NULL;
      for(mp = allocations; mp; mp = mp->next) {
         if(mp->pointer == *ptr) {
            if(last) {
               last->next = mp->next;
            } else {
               allocations = mp->next;
            }

            if(((char*)*ptr)[mp->size] != 42) {
               Debug("MEMORY: %s[%u]: The canary is dead.", file, line);
            }

            memset(*ptr, 0xFF, mp->size);
            free(mp);
            free(*ptr);
            *ptr = NULL;
            return;
         }
         last = mp;
      }
      Debug("MEMORY: %s[%u]: Attempt to delete unallocated pointer",
         file, line);
      memset(*ptr, 0xFF, mp->size);
      free(*ptr);

      /* This address should cause a segfault or bus error. */
      *ptr = (void*)1;

   }
}

#undef CHECKPOINT_LIST_SIZE

#endif

