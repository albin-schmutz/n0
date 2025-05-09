#include <assert.h>
#include <stdio.h>
#include "source.h"

static void test(void)
{
	struct SOURCE src;

	src_open(&src, "source_test.bin");
	assert(src.file);

	src_fetch_char(&src);
	assert(src.ch == 'a');
	assert(src.line == 1);
	assert(src.pos == 1);

	src_fetch_char(&src);
	assert(src.ch == '\n');
	assert(src.line == 1);
	assert(src.pos == 2);

	src_fetch_char(&src);
	assert(src.ch == 'b');
	assert(src.line == 2);
	assert(src.pos == 1);

	src_fetch_char(&src);
	assert(src.ch == 'b');
	assert(src.line == 2);
	assert(src.pos == 2);

	src_fetch_char(&src);
	assert(src.ch == '\n');
	assert(src.line == 2);
	assert(src.pos == 3);

	src_fetch_char(&src);
	assert(src.ch == '\n');
	assert(src.line == 3);
	assert(src.pos == 1);

	src_fetch_char(&src);
	assert(src.ch == 'c');
	assert(src.line == 4);
	assert(src.pos == 1);

	src_fetch_char(&src);
	assert(src.ch == 'c');
	assert(src.line == 4);
	assert(src.pos == 2);

	src_fetch_char(&src);
	assert(src.ch == 'c');
	assert(src.line == 4);
	assert(src.pos == 3);

	src_fetch_char(&src);
	assert(src.ch == '\n');
	assert(src.line == 4);
	assert(src.pos == 4);

	src_fetch_char(&src);
	assert(src.ch == ' ');
	assert(src.line == 5);
	assert(src.pos == 1);

	src_fetch_char(&src);
	assert(src.ch == 'd');
	assert(src.line == 5);
	assert(src.pos == 2);

	src_fetch_char(&src);
	assert(src.ch == '\n');
	assert(src.line == 5);
	assert(src.pos == 3);

	src_fetch_char(&src);

	int i = 0;
	while (i < 64) {
		int j = 0;
		while (j < 10) {
			assert(src.ch == '0' + j);
			src_fetch_char(&src);
			j++;
		}
		j = 0;
		while (j < 26) {
			assert(src.ch == 'A' + j);
			src_fetch_char(&src);
			j++;
		}
		j = 0;
		while (j < 26) {
			assert(src.ch == 'a' + j);
			src_fetch_char(&src);
			j++;
		}
		assert(src.ch == '!');
		src_fetch_char(&src);
		assert(src.ch == '\n');
		src_fetch_char(&src);
		i++;
	}
	assert(src.ch == 'H');
	src_fetch_char(&src);
	assert(src.ch == 'a');
	src_fetch_char(&src);
	assert(src.ch == 'l');
	src_fetch_char(&src);
	assert(src.ch == 'l');
	src_fetch_char(&src);
	assert(src.ch == 'o');
	src_fetch_char(&src);
	assert(src.ch == '!');
	src_fetch_char(&src);
	assert(src.ch == '\n');
	src_fetch_char(&src);
	assert(src.ch == -1);

	src_close(&src);
}

int main(void)
{
	test();
	return 0;
}
