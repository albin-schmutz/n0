#include <assert.h>
#include <stdio.h>
#include "source.h"

void src_open(SOURCE src, char *filename)
{
	assert(src);
	assert(filename);

	src->file = fopen(filename, "rb");
	src->last_eol = 0;
	src->eol = 1;
	src->line = 0;
}

void src_close(SOURCE src)
{
	assert(src);

	if (src->file) {
		fclose(src->file);
	}
}

void src_fetch_char(SOURCE src)
{
	assert(src);

	if (src->last_eol == -1) {
		src->last_eol = 0;
	} else if (src->eol) {
		src->line++;
		src->pos = 1;
		src->eol = 0;
	} else {
		src->pos++;
	}
	src->ch = fgetc(src->file);
	if (src->ch >= 0) {
		if (src->ch == '\n' || src->ch == '\r') {
			if (src->last_eol == 0) {
				src->last_eol = src->ch;
				src->ch = '\n';
				src->eol = 1;
			} else if (src->last_eol == src->ch) {
				src->ch = '\n';
				src->eol = 1;
			} else {
				src->last_eol = -1;
				src_fetch_char(src);
			}
		} else if (src->last_eol != 0) {
			src->last_eol = 0;
		}
	}
}
