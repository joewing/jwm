/***************************************************************************
 * Debug functions.
 * Copyright (C) 2003 Joe Wingbermuehle
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
 ***************************************************************************/

#include "debug.h"

#ifdef DEBUG

#define CHECKPOINT_LIST_SIZE 10

typedef struct MemoryType {
	const char *file;
	unsigned int line;
	size_t size;
	void *pointer;
	struct MemoryType *next;
} MemoryType;

static MemoryType *allocations = NULL;

typedef struct ResourceType {
	int resource;
	const char *allocationFiles[CHECKPOINT_LIST_SIZE];
	unsigned int allocationLines[CHECKPOINT_LIST_SIZE];
	const char *releaseFiles[CHECKPOINT_LIST_SIZE];
	unsigned int releaseLines[CHECKPOINT_LIST_SIZE];
	unsigned int allocationOffset;
	unsigned int releaseOffset;
	size_t count;
	struct ResourceType *next;
} ResourceType;

static ResourceType *resources = NULL;

static const char *checkpointFile[CHECKPOINT_LIST_SIZE];
static unsigned int checkpointLine[CHECKPOINT_LIST_SIZE];
static int checkpointOffset;

static void DEBUG_PrintResourceStack(ResourceType *rp);

/***************************************************************************
 * Emit a message.
 ***************************************************************************/
void DEBUG_Debug(const char *str, ...) {
	va_list ap;
	va_start(ap, str);

	fprintf(stderr, "DEBUG: ");
	vfprintf(stderr, str, ap);
	fprintf(stderr, "\n");

	va_end(ap);
}

/***************************************************************************
 * Start the debugger.
 ***************************************************************************/
void DEBUG_StartDebug(const char *file, unsigned int line) {
	int x;

	Debug("%s[%u]: debug mode started", file, line);

	checkpointOffset = 0;
	for(x = 0; x < CHECKPOINT_LIST_SIZE; x++) {
		checkpointFile[x] = NULL;
		checkpointLine[x] = 0;
	}

}

/***************************************************************************
 * Stop the debugger.
 ***************************************************************************/
void DEBUG_StopDebug(const char *file, unsigned int line) {
	MemoryType *mp;
	ResourceType *rp;
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

	if(resources) {
		for(rp = resources; rp; rp = rp->next) {
			if(rp->count > 0) {
				Debug("RESOURCE: resource %d has reference count %u",
					rp->resource, rp->count);
				DEBUG_PrintResourceStack(rp);
			}
		}
	}

}

/***************************************************************************
 * Print the resource allocation/release stacks for a resource.
 ***************************************************************************/
void DEBUG_PrintResourceStack(ResourceType *rp) {
	unsigned int x, offset;

	Debug("          Allocation stack: (oldest)");
	offset = rp->allocationOffset;
	for(x = 0; x < CHECKPOINT_LIST_SIZE; x++) {
		if(rp->allocationFiles[offset]) {
			Debug("             %s line %u", rp->allocationFiles[offset],
				rp->allocationLines[offset]);
		}
		offset = (offset + 1) % CHECKPOINT_LIST_SIZE;
	}
	Debug("          Release stack: (oldest)");
	offset = rp->releaseOffset;
	for(x = 0; x < CHECKPOINT_LIST_SIZE; x++) {
		if(rp->releaseFiles[offset]) {
			Debug("             %s line %u", rp->releaseFiles[offset],
				rp->releaseLines[offset]);
		}
		offset = (offset + 1) % CHECKPOINT_LIST_SIZE;
	}
}

/***************************************************************************
 * Set a checkpoint.
 ***************************************************************************/
void DEBUG_SetCheckpoint(const char *file, unsigned int line) {

	checkpointFile[checkpointOffset] = file;
	checkpointLine[checkpointOffset] = line;

	checkpointOffset = (checkpointOffset + 1) % CHECKPOINT_LIST_SIZE;

}

/***************************************************************************
 * Display the location of the last checkpoint.
 ***************************************************************************/
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

/***************************************************************************
 * Allocate memory and log.
 ***************************************************************************/
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

/***************************************************************************
 * Reallocate memory and log.
 ***************************************************************************/
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

/***************************************************************************
 * Release memory and log.
 ***************************************************************************/
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

				free(mp);
				memset(*ptr, 0xFF, mp->size);
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

/***************************************************************************
 * Add a resource.
 ***************************************************************************/
void DEBUG_AllocateResource(int resource, const char *file,
	unsigned int line) {

	ResourceType *rp;

	for(rp = resources; rp; rp = rp->next) {
		if(rp->resource == resource) {

			rp->allocationFiles[rp->allocationOffset] = file;
			rp->allocationLines[rp->allocationOffset] = line;
			rp->allocationOffset
				= (rp->allocationOffset + 1) % CHECKPOINT_LIST_SIZE;

			++rp->count;
			return;
		}
	}

	rp = malloc(sizeof(ResourceType));
	memset(rp, 0, sizeof(ResourceType));
	rp->resource = resource;
	rp->allocationFiles[0] = file;
	rp->allocationLines[0] = line;
	rp->allocationOffset = 1;
	rp->releaseOffset = 0;
	rp->count = 1;

	rp->next = resources;
	resources = rp;

}

/***************************************************************************
 * Remove a resource.
 ***************************************************************************/
void DEBUG_ReleaseResource(int resource, const char *file,
	unsigned int line) {

	ResourceType *rp;

	for(rp = resources; rp; rp = rp->next) {
		if(rp->resource == resource) {
			rp->releaseFiles[rp->releaseOffset] = file;
			rp->releaseLines[rp->releaseOffset] = line;
			rp->releaseOffset = (rp->releaseOffset + 1) % CHECKPOINT_LIST_SIZE;
			if(rp->count <= 0) {
				Debug("RESOURCE: Multiple attempts to release resource %d",
					resource);
				DEBUG_PrintResourceStack(rp);
			} else {
				--rp->count;
			}
			return;
		}
	}

	Debug("RESOURCE: Attempt to release unallocated resource %d",
		resource);
	Debug("          in %s at line %u", file, line);

}

#undef CHECKPOINT_LIST_SIZE

#endif

