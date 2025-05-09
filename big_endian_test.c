#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "big_endian.h"

static void test_from_to_i32(void)
{
	int32_t I[] = {
		0, 1, -1, 2, -2, 3894094, -178344,
		0x7fffffff, 0x80000000
	};
	#define n 9
	unsigned char b[n * 4];
	int i = 0;
	while (i < n) {
		be_from_i32(&b[i * 4], I[i]);
		assert(be_to_i32(&b[i * 4]) == I[i]);
		i++;
	}
	char *expected =
		"00000000"
		"00000001"
		"ffffffff"
		"00000002"
		"fffffffe"
		"003b6b4e"
		"fffd4758"
		"7fffffff"
		"80000000";
	char hex[n * 4 * 2 + 1];
	for (i = 0; i < n * 4; ++i) sprintf(&hex[i * 2], "%02x", b[i]);
	assert(!strcmp(expected, hex));
}

static void test_from_to_i64(void)
{
	int64_t I[] = {
		0, 1, -1, 2, -2, 3894094, -8174544544544834,
		0x7fffffffffffffff, 0x8000000000000000
	};
	#define n 9
	unsigned char b[n * 8];
	int i = 0;
	while (i < n) {
		be_from_i64(&b[i * 8], I[i]);
		assert(be_to_i64(&b[i * 8]) == I[i]);
		i++;
	}
	char *expected =
		"0000000000000000"
		"0000000000000001"
		"ffffffffffffffff"
		"0000000000000002"
		"fffffffffffffffe"
		"00000000003b6b4e"
		"ffe2f54b8838c7be"
		"7fffffffffffffff"
		"8000000000000000";
	char hex[n * 8 * 2 + 1];
	for (i = 0; i < n * 8; ++i) sprintf(&hex[i * 2], "%02x", b[i]);
	assert(!strcmp(expected, hex));
}

int main(void)
{
	test_from_to_i32();
	test_from_to_i64();
	return 0;
}
