#include <assert.h>
#include <stdint.h>
#include "big_endian.h"

static int64_t get(unsigned char b[], int size)
{
	assert(size > 0);
	assert(size <= 8);
	int64_t result = 0;
	int pos = 0;
	while (pos < size) {
		result = (result << 8) | (unsigned char)b[pos++];
	}
	return result;
}

static void put(unsigned char b[], int size, int64_t i)
{
	assert(size > 0);
	assert(size <= 8);
	int pos = size;
	while (pos--) {
		b[pos] = i & 255;
		i >>= 8;
	}
}

int32_t be_to_i32(unsigned char b[])
{
	return get(b, 4);
}

int64_t be_to_i64(unsigned char b[])
{
	return get(b, 8);
}

void be_from_i32(unsigned char b[], int32_t i)
{
	put(b, 4, i);
}

void be_from_i64(unsigned char b[], int64_t i)
{
	put(b, 8, i);
}
