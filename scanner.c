#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "source.h"
#include "scanner.h"

#define MAX_LEN_INT 20

static void get_char(SCANNER scan)
{
	src_fetch_char(&scan->source);
	if (scan->source.ch == -1) {
		scan_raise_error(scan, "Unexpected eof");
	}
}

void scan_create(SCANNER scan, SCANNER parent)
{
	assert(scan);
	assert(parent);

	char msg[80];
	char filename[64];

	strcpy(filename, scan->modulename);
	strcat(filename, ".n0");
	src_open(&scan->source, filename);
	if (!scan->source.file) {
		strcpy(msg, "Open file error: ");
		strcat(msg, filename);
		scan_raise_error(parent, msg);
	}

	get_char(scan);
}

void scan_close(SCANNER scan)
{
	assert(scan);

	src_close(&scan->source);
}

void scan_raise_error(SCANNER scan, char *msg)
{
	assert(scan);
	assert(msg);

	fprintf(stderr, "Error in %s at %i/%i: %s\n",
		scan->modulename, scan->line, scan->pos, msg);
	exit(1);
}

static void raise_error_unexp_char(SCANNER scan)
{
	scan_raise_error(scan, "Unexpected char");
}

static void comment(SCANNER scan)
{
	get_char(scan);
	int ok = 1;
	while (ok) {
		if (scan->source.ch == '*') {
			get_char(scan);
			if (scan->source.ch == ')') {
				get_char(scan);
				ok = 0;
			}
		} else if (scan->source.ch == '(') {
			get_char(scan);
			if (scan->source.ch == '*') comment(scan);
		} else {
			get_char(scan);
		}
	}
	scan->token = tk_none;
}

static void number(SCANNER scan)
{
	char buf[MAX_LEN_INT];
	int i = 0;
	int hex = 0;
	int base = 10;
	while (scan->source.ch != -1 &&
		(hex || (scan->source.ch >= '0' && scan->source.ch <= '9'))) {
		if (i < MAX_LEN_INT) {
			buf[i++] = scan->source.ch;
			get_char(scan);
			hex = scan->source.ch >= 'A' && scan->source.ch <= 'F';
			if (hex) base = 16;
		} else {
			scan_raise_error(scan,"Integer too long");
		}
	}
	scan->token = tk_int;
	if (scan->source.ch == 'H') {
		base = 16;
		get_char(scan);
	} else if (scan->source.ch == 'X') {
		scan->token = tk_char;
		base = 16;
		get_char(scan);
	} else if (base == 16) {
		scan_raise_error(scan, "Postfix H or X ?");
	}
	int j = 0;
	scan->val = 0;
	while (j < i) {
		char c = buf[j++];
		int n = c >= 'A' && c <= 'F' ? 10 - 'A' + c : c - '0';
		scan->val = base * scan->val + n;
	}
}

static enum token keyword(char *id, int len)
{
	if (len == 2) {
		if (!strcmp("DO", id)) return kw_do;
		if (!strcmp("IF", id)) return kw_if;
		if (!strcmp("OF", id)) return kw_of;
		if (!strcmp("OR", id)) return kw_or;
	} else if (len == 3) {
		if (!strcmp("DIV", id)) return kw_div;
		if (!strcmp("END", id)) return kw_end;
		if (!strcmp("MOD", id)) return kw_mod;
		if (!strcmp("VAR", id)) return kw_var;
	} else if (len == 4) {
		if (!strcmp("ELSE", id)) return kw_else;
		if (!strcmp("THEN", id)) return kw_then;
		if (!strcmp("TRUE", id)) return kw_true;
		if (!strcmp("TYPE", id)) return kw_type;
	} else if (len == 5) {
		if (!strcmp("ARRAY", id)) return kw_array;
		if (!strcmp("BEGIN", id)) return kw_begin;
		if (!strcmp("CONST", id)) return kw_const;
		if (!strcmp("ELSIF", id)) return kw_elsif;
		if (!strcmp("FALSE", id)) return kw_false;
		if (!strcmp("UNTIL", id)) return kw_until;
		if (!strcmp("WHILE", id)) return kw_while;
	} else if (len == 6) {
		if (!strcmp("IMPORT", id)) return kw_import;
		if (!strcmp("MODULE", id)) return kw_module;
		if (!strcmp("RECORD", id)) return kw_record;
		if (!strcmp("REPEAT", id)) return kw_repeat;
		if (!strcmp("RETURN", id)) return kw_return;
	} else {
		if (!strcmp("PROCEDURE", id) ) return kw_procedure;
	}
	return tk_none;
}

static void identifier(SCANNER scan)
{
	int i = 0;
	int up_case = 1;
	while ((scan->source.ch >= 'A' && scan->source.ch <= 'Z') ||
		(scan->source.ch >= '0' && scan->source.ch <= '9') ||
		(scan->source.ch >= 'a' && scan->source.ch <= 'z')) {
		if (i < 31) {
			up_case = up_case && scan->source.ch >= 'A' && scan->source.ch <= 'Z';
			scan->ident[i++] = scan->source.ch;
			get_char(scan);
		} else {
			scan_raise_error(scan, "Identifier too long");
		}
	}
	scan->ident[i] = 0;
	scan->token = (up_case && i < 10) ?
		keyword(scan->ident, i) : tk_none;
	if (scan->token == tk_none) scan->token = tk_ident;
}

void scan_take(SCANNER scan)
{
	assert(scan);

	scan->token = tk_none;
	while (scan->token == tk_none) {
		while (scan->source.ch <= ' ') get_char(scan);
		scan->line = scan->source.line;
		scan->pos = scan->source.pos;
		if (scan->source.ch < 'A') {
			if (scan->source.ch < '0') {
				if (scan->source.ch == '#') {
					scan->token = tk_neq;
					get_char(scan);
				} else if (scan->source.ch == '&') {
					scan->token = tk_amper;
					get_char(scan);
				} else if (scan->source.ch == '(') {
					get_char(scan);
					if (scan->source.ch == '*') {
						comment(scan);
					} else {
						scan->token = tk_paren_l;
					}
				} else if (scan->source.ch == ')') {
					scan->token = tk_paren_r;
					get_char(scan);
				} else if (scan->source.ch == '*') {
					scan->token = tk_asterisk;
					get_char(scan);
				} else if (scan->source.ch == '+') {
					scan->token = tk_plus;
					get_char(scan);
				} else if (scan->source.ch == ',') {
					scan->token = tk_comma;
					get_char(scan);
				} else if (scan->source.ch == '-') {
					scan->token = tk_minus;
					get_char(scan);
				} else if (scan->source.ch == '.') {
					scan->token = tk_dot;
					get_char(scan);
				} else { /* ! " $ ' / */
					raise_error_unexp_char(scan);
				}
			} else if (scan->source.ch < ':') {
				number(scan);
			} else if (scan->source.ch == ':') {
				scan->token = tk_colon;
				get_char(scan);
				if (scan->source.ch == '=') {
					scan->token = tk_assign;
					get_char(scan);
				}
			} else if (scan->source.ch == ';') {
				scan->token = tk_semic;
				get_char(scan);
			} else if (scan->source.ch == '<') {
				scan->token = tk_lss;
				get_char(scan);
				if (scan->source.ch == '=') {
					scan->token = tk_leq;
					get_char(scan);
				}
			} else if (scan->source.ch == '>') {
				scan->token = tk_gtr;
				get_char(scan);
				if (scan->source.ch == '=') {
					scan->token = tk_geq;
					get_char(scan);
				}
			} else if (scan->source.ch == '=') {
				scan->token = tk_eq;
				get_char(scan);
			} else { /* ? @ */
				raise_error_unexp_char(scan);
			}
		} else if (scan->source.ch < '[') {
			identifier(scan);
		} else if (scan->source.ch < 'a') {
			if (scan->source.ch == '[') {
				scan->token = tk_brack_l;
				get_char(scan);
			} else if (scan->source.ch == ']') {
				scan->token = tk_brack_r;
				get_char(scan);
			} else { /* \ _ ` */
				raise_error_unexp_char(scan);
			}
		} else if (scan->source.ch < '{') {
			identifier(scan);
		} else {
			if (scan->source.ch == '~') {
				scan->token = tk_tilde;
				get_char(scan);
			} else {
				raise_error_unexp_char(scan);
			}
		}
	}
}

void scan_expect(SCANNER scan, enum token exp)
{
	assert(scan);

	char *msg = NULL;

	if (scan->token != exp) {
		switch (exp) {
		case kw_begin: msg = "BEGIN ?"; break;
		case kw_do: msg = "DO ?"; break;
		case kw_end: msg = "END ?"; break;
		case kw_module: msg = "MODULE ?"; break;
		case kw_of: msg = "OF ?"; break;
		case kw_return: msg = "RETURN ?"; break;
		case kw_then: msg = "THEN ?"; break;
		case kw_until: msg = "UNTIL ?"; break;
		case tk_ident: msg = "Identifier ?"; break;
		case tk_assign: msg = ":= ?"; break;
		case tk_colon: msg = ": ?"; break;
		case tk_comma: msg = ", ?"; break;
		case tk_dot: msg = ". ?"; break;
		case tk_eq: msg = "= ?"; break;
		case tk_brack_l: msg = "[ ?"; break;
		case tk_brack_r: msg = "] ?"; break;
		case tk_paren_l: msg = "( ?"; break;
		case tk_paren_r: msg = ") ?"; break;
		case tk_semic: msg = "; ?"; break;
		default: assert(0);
		}
		scan_raise_error(scan, msg);
	}
}
