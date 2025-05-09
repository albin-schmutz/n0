/*
	Provides a reader for source files.
	Tracks line and position.
	Can handle mixed end-of-lines.
*/

typedef struct SOURCE *SOURCE;

struct SOURCE {
	FILE *file;
	int line, pos;
	int eol, last_eol;
	int ch;
};

extern void src_open(SOURCE src, char *filename);
extern void src_close(SOURCE src);
extern void src_fetch_char(SOURCE src);
