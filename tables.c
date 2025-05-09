#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "alloc_free.h"
#include "clazz.h"
#include "tables.h"

int tbl_level;

int tbl_boolean_type;
int tbl_char_type;
int tbl_short_int_type;
int tbl_integer_type;
int tbl_long_int_type;

struct ALLOC_FREE tbl_af_types;
struct TYPE tbl_types[MAX_TYPES];

struct ALLOC_FREE tbl_af_symbols;
struct SYMBOL tbl_symbols[MAX_SYMBOLS];

int tbl_marker;
int tbl_top_scope;
static int universe;

struct MODULE tbl_modules[MAX_MODULES];
static int top_module;

TYPE init_type(int *t, int form, int size)
{
	assert(t);

	TYPE type;

	af_alloc(&tbl_af_types, t);
	type = &tbl_types[*t];
	type->size = size;
	type->length = 0;
	type->form = form;
	type->owner = -1;
	type->base = -1;

	return type;
}

static void reset_symbol(SYMBOL s)
{
	s->name[0] = 0;
	s->val = 0;
	s->val2 = 0;
	s->val3 = 0;
	s->next = tbl_marker;
	s->clazz = 0;
	s->scope = -1;
	s->type = -1;
	s->level = -1;
	s->is_public = 0;
}

static void init_symbol(int *s, char *name, int clazz, int t, int64_t val)
{
	assert(s);
	assert(*s);
	assert(name);

	SYMBOL symbol;
	int s2;

	af_alloc(&tbl_af_symbols, &s2);
	symbol = &tbl_symbols[s2];
	reset_symbol(symbol);

	strcpy(symbol->name, name);
	tbl_symbols[*s].next = s2;
	symbol->clazz = clazz;
	symbol->type = t;
	symbol->level = 0;
	symbol->val = val;
	if (clazz == CLASS_TYPE) {
		tbl_types[t].owner = s2;
	}
	*s = s2;
}

void tbl_init(void)
{
	af_init();

	int s;

	af_create(&tbl_af_types, MAX_TYPES);
	init_type(&tbl_boolean_type, FORM_BOOL, 1);
	init_type(&tbl_char_type, FORM_CHAR, 1);
	init_type(&tbl_short_int_type, FORM_INT, 2);
	init_type(&tbl_integer_type, FORM_INT, 4);
	init_type(&tbl_long_int_type, FORM_INT, 8);

	tbl_level = 0;
	tbl_top_scope = -1;
	af_create(&tbl_af_symbols, MAX_SYMBOLS);
	af_alloc(&tbl_af_symbols, &tbl_marker);
	reset_symbol(&tbl_symbols[tbl_marker]);
	tbl_open_scope();
	universe = tbl_top_scope;
	s = tbl_top_scope;

	init_symbol(&s, "ASSERT", CLASS_SPROC, -1, 1);
	init_symbol(&s, "SYS", CLASS_SPROC, -1, 2);
	init_symbol(&s, "CHR", CLASS_SFUNC, tbl_char_type, 1);
	init_symbol(&s, "ORD", CLASS_SFUNC, tbl_integer_type, 2);
	init_symbol(&s, "ASH", CLASS_SFUNC, tbl_long_int_type, 3);
	init_symbol(&s, "BITA", CLASS_SFUNC, tbl_long_int_type, 4);
	init_symbol(&s, "BITO", CLASS_SFUNC, tbl_long_int_type, 5);
	init_symbol(&s, "BOOLEAN", CLASS_TYPE, tbl_boolean_type, 0);
	init_symbol(&s, "CHAR", CLASS_TYPE, tbl_char_type, 0);
	init_symbol(&s, "SHORTINT", CLASS_TYPE, tbl_short_int_type, 0);
	init_symbol(&s, "INTEGER", CLASS_TYPE, tbl_integer_type, 0);
	init_symbol(&s, "LONGINT", CLASS_TYPE, tbl_long_int_type, 0);
	tbl_symbols[s].next = tbl_marker;

	top_module = 0;
}


/* types */


TYPE tbl_create_type(int *t, int form, int size)
{
	assert(t);

	TYPE type = init_type(t, form, size);
	return type;
}

static void dealloc_type(int t, int private_only)
{
	TYPE type;
	int scope;

	type = &tbl_types[t];
	if (type->form == FORM_RECORD) {
		scope = type->base;
		tbl_dealloc_scope(scope, private_only);
		if (tbl_symbols[scope].next == tbl_marker) {
			type->base = -1;
		}
	} else if (type->form == FORM_ARRAY && tbl_types[type->base].owner == -1) {
		dealloc_type(type->base, private_only);
		af_free(&tbl_af_types, type->base);
	}
}


/* symbols */


void tbl_set_marker(char *id)
{
	assert(id);

	strcpy(tbl_symbols[tbl_marker].name, id);
}

void tbl_find_symbol(int *s)
{
	assert(s);

	while (strcmp(tbl_symbols[tbl_symbols[*s].next].name, tbl_symbols[tbl_marker].name)) {
		*s = tbl_symbols[*s].next;
	}
}

void tbl_open_scope()
{
	SYMBOL symbol;
	int s;

	af_alloc(&tbl_af_symbols, &s);
	symbol = &tbl_symbols[s];
	reset_symbol(symbol);
	symbol->clazz = CLASS_SCOPE;
	symbol->scope = tbl_top_scope;
	tbl_top_scope = s;
}


static void dealloc_symbol(int s, int private_only)
{
	SYMBOL symbol;
	TYPE type;
	int t;

	symbol = &tbl_symbols[s];
	if (symbol->clazz == CLASS_PROC && symbol->scope != -1) {
		tbl_dealloc_scope(symbol->scope, private_only);
	}
	t = symbol->type;
	if (t != -1) {
		type = &tbl_types[t];
 		if (type->owner == -1 || type->owner == s) {
			dealloc_type(t, private_only);
			if (!private_only || !symbol->is_public) {
				af_free(&tbl_af_types, t);
			}
		}
	}
}

void tbl_dealloc_scope(int scope, int private_only)
{
	assert(scope >= 0);

	SYMBOL prev_p;
	SYMBOL symbol;
	int prev;
	int s;

	prev = scope;
	prev_p = &tbl_symbols[prev];
	s = prev_p->next;
	while (s != tbl_marker) {
		symbol = &tbl_symbols[s];
		if (private_only && symbol->is_public) {
			dealloc_symbol(s, 1);
			prev = s;
			prev_p = &tbl_symbols[prev];
			s = symbol->next;
		} else {
			dealloc_symbol(s, 0);
			prev_p->next = symbol->next;
			af_free(&tbl_af_symbols, s);
			s = prev_p->next;
		}
	}
	if (tbl_symbols[scope].next == tbl_marker) {
		af_free(&tbl_af_symbols, scope);
	}
}

void tbl_close_scope(void)
{
	tbl_top_scope = tbl_symbols[tbl_top_scope].scope;
}

SYMBOL tbl_def_symbol(int *s, int clazz, int m)
{
	assert(s);

	SYMBOL symbol = NULL;
	int prev = tbl_top_scope;

	tbl_find_symbol(&prev);
	*s = -1;
	if (tbl_symbols[prev].next == tbl_marker) {
		af_alloc(&tbl_af_symbols, s);
		symbol = &tbl_symbols[*s];
		reset_symbol(symbol);
		if (clazz > CLASS_SCOPE) {
			strcpy(symbol->name, tbl_symbols[tbl_marker].name);
		}
		symbol->clazz = clazz;
		symbol->scope = m;
		tbl_symbols[prev].next = *s;
	}
	return symbol;
}

SYMBOL tbl_use_symbol(int *s, int imp)
{
	assert(s);

	int scope;

	*s = -1;
	if (imp == -1) {
		scope = tbl_top_scope;
	} else {
		scope = tbl_symbols[imp].scope;
	}
	assert(tbl_symbols[scope].clazz == CLASS_SCOPE);
	while (scope != -1) {
		*s = scope;
		tbl_find_symbol(s);
		*s = tbl_symbols[*s].next;
		if (*s == tbl_marker) *s = -1;
		if (*s == -1 && imp == -1) {
			scope = tbl_symbols[scope].scope;
		} else {
			scope = -1;
		}
	}

	return *s == -1 ? NULL : &tbl_symbols[*s];
}


/* modules */


int tbl_open_module(char *name)
{
	assert(name);

	int m = 0;
	while (m < top_module) {
		if (strcmp(tbl_symbols[tbl_modules[m].scope].name, name)) {
			(m)++;
		} else {
			(m) += 1000;
		}
	}
	if (m < 1000) {
		assert(top_module < MAX_MODULES);
		top_module++;
		tbl_open_scope();
		strcpy(tbl_symbols[tbl_top_scope].name, name);
		tbl_modules[m].scope = tbl_top_scope;
		tbl_modules[m].loaded = -1; /* new and not loaded */
	} else {
		(m) -= 1000;
	}
	return m;
}
