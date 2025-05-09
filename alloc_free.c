#include <assert.h>
#include <stdint.h>
#include "alloc_free.h"

#define MAX_ENTRIES 300

static int16_t entries[MAX_ENTRIES];
static int next_free;

void af_init(void)
{
	next_free = 0;
}

void af_create(ALLOC_FREE af, int size)
{
	assert(af);
	assert(size > 0);

	af->start = next_free;
	af->size = size + 1; /* place for end marker */
	next_free += af->size;
	assert(next_free < MAX_ENTRIES);
	af->top = 0;
	af->free = af->size;
	af->alloc = 0;
}

void af_alloc(ALLOC_FREE af, int *entry)
{
	assert(af);
	assert(entry);

	if (af->free < af->size) {
		*entry = entries[af->start + af->free];
		af->free++;
	} else {
		assert(af->alloc + 1 < af->free);
		*entry = af->top;
		af->top++;
	}

	entries[af->start + af->alloc] = *entry;
	af->alloc++;
}

void af_free(ALLOC_FREE af, int entry)
{
	assert(af);
	assert(entry >= 0);
	assert(af->alloc < af->free);

	entries[af->start + af->alloc] = entry; /* end marker */
	int find = 0;
	while (entries[af->start + find] != entry) find++;
	assert(find < af->alloc);
	af->free--;
	entries[af->start + af->free] = entry;
	af->alloc--;
	if (af->alloc > 0) {
		entries[af->start + find] = entries[af->start + af->alloc];
	}
}
