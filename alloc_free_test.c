#include <assert.h>
#include <stdint.h>
#include "alloc_free.h"

static void test_max_alloc(void)
{
	struct ALLOC_FREE af;
	int entry;

	af_init();
	af_create(&af, 10);
	int i = 0;
	while (i < 10) {
		af_alloc(&af, &entry);
		assert(entry == i);
		i++;
	}
	entry = 0;
	while (entry < 10) {
		af_free(&af, entry);
		entry++;
	}
}

static void test_2_mixed_alloc_free_actions(void)
{
	struct ALLOC_FREE af1;
	struct ALLOC_FREE af2;
	int entry;

	af_init();
	af_create(&af1, 12);
	af_create(&af2, 50);

	int16_t i = 0;
	while (i < 10) {
		af_alloc(&af1, &entry);
		assert(entry == i);
		af_alloc(&af2, &entry);
		assert(entry == i);
		i++;
	}
	while (i < 20) {
		af_alloc(&af2, &entry);
		assert(entry == i);
		i++;
	}

	i = 3;
	while (i < 6) {
		af_free(&af1, i);
		i++;
	}

	af_alloc(&af1, &entry);
	assert(entry == 5);
	af_alloc(&af2, &entry);
	assert(entry == 20);
	af_alloc(&af1, &entry);
	assert(entry == 4);
	af_alloc(&af2, &entry);
	assert(entry == 21);
	af_alloc(&af1, &entry);
	assert(entry == 3);
	af_alloc(&af2, &entry);
	assert(entry == 22);
	af_alloc(&af1, &entry);
	assert(entry == 10);
	af_alloc(&af2, &entry);
	assert(entry == 23);

	af_free(&af1, 10);
	entry = -1;
	af_alloc(&af1, &entry);
	assert(entry == 10);
}

int main(void)
{
	test_max_alloc();
	test_2_mixed_alloc_free_actions();
	return 0;
}
