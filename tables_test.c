#include <assert.h>
#include <stdint.h>
#include "alloc_free.h"
#include "clazz.h"
#include "tables.h"

#define INIT_LAST_TYPE 4
#define INIT_LAST_SYMBOL 13

static void test_init(void)
{
	TYPE type;
	int s;

	tbl_init();

	assert(tbl_level == 0);

	assert(tbl_short_int_type == 2);
	type = &tbl_types[tbl_short_int_type];
	assert(type->size == 2);
	assert(type->form == FORM_INT);

	assert(tbl_long_int_type == INIT_LAST_TYPE);

	assert(tbl_marker == 0);
	assert(tbl_top_scope == 1);

	tbl_set_marker("ASSERT");
	s = tbl_top_scope;
	tbl_find_symbol(&s);
	assert(s == 1);
	assert(tbl_symbols[s].next == 2);

	tbl_set_marker("SHORTINT");
	s = tbl_top_scope;
	tbl_find_symbol(&s);
	assert(s == 10);
	assert(tbl_symbols[s].next == 11);

	tbl_set_marker("LONGINT");
	s = tbl_top_scope;
	tbl_find_symbol(&s);
	assert(s == 12);
	assert(tbl_symbols[s].next == INIT_LAST_SYMBOL);

	assert(tbl_symbols[INIT_LAST_SYMBOL].next == 0);
}

static void deallocate_scope_with_childs(int is_public[2])
{
	SYMBOL symbol;
	int s;

	/* scope with const a and b */
	tbl_init();
	int scope = INIT_LAST_SYMBOL + 1;
	int a = INIT_LAST_SYMBOL + 2;
	int b = INIT_LAST_SYMBOL + 3;

	tbl_open_scope();
	assert(tbl_top_scope == scope);

	tbl_set_marker("a");
	symbol = tbl_def_symbol(&s, CLASS_CONST, -1);
	symbol->is_public = is_public[0];

	assert(tbl_symbols[scope].next == s);
	assert(s == a);

	tbl_set_marker("b");
	symbol = tbl_def_symbol(&s, CLASS_CONST, -1);
	symbol->is_public = is_public[1];
	assert(s == b);

	tbl_dealloc_scope(tbl_top_scope, 1);
}

static void test_deallocate_scope_with_childs(void)
{
	int s;

	int is_public[3][2] = {
		{ 0, 0 },
		{ 0, 1 },
		{ 1, 1 },
	};

	int scope = INIT_LAST_SYMBOL + 1;
	int a = INIT_LAST_SYMBOL + 2;
	int b = INIT_LAST_SYMBOL + 3;

	deallocate_scope_with_childs(is_public[0]);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == scope);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == b);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == a);

	deallocate_scope_with_childs(is_public[1]);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == a);

	tbl_set_marker("a");
	s = tbl_top_scope;
	tbl_find_symbol(&s);
	assert(s == b);
	assert(tbl_symbols[s].next == tbl_marker);
	tbl_set_marker("b");
	s = tbl_top_scope;
	tbl_find_symbol(&s);
	assert(s == scope);
	assert(tbl_symbols[s].next == b);

	deallocate_scope_with_childs(is_public[2]);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == b + 1);

	tbl_set_marker("a");
	s = tbl_top_scope;
	tbl_find_symbol(&s);
	assert(s == scope);
	assert(tbl_symbols[s].next == a);
	tbl_set_marker("b");
	s = tbl_top_scope;
	tbl_find_symbol(&s);
	assert(s == a);
	assert(tbl_symbols[s].next == b);
}


static void deallocate_anonymous_record_inside_array(int is_public[3])
{
	int s;
	int t;

	/* scope with array A containing record with members i and j */
	tbl_init();
	int scope = INIT_LAST_SYMBOL + 1;
	int A = INIT_LAST_SYMBOL + 2;
	int A_type = INIT_LAST_TYPE + 1;
	int R_type = INIT_LAST_TYPE + 2;
	int R_type_scope = INIT_LAST_SYMBOL + 3;
	int i = INIT_LAST_SYMBOL + 4;
	int j = INIT_LAST_SYMBOL + 5;

	tbl_open_scope();
	assert(tbl_top_scope == scope);

	tbl_set_marker("A");
	SYMBOL symbol_A = tbl_def_symbol(&s, CLASS_TYPE, -1);
	symbol_A->is_public = is_public[0];
	assert(s == A);

	tbl_create_type(&t, FORM_ARRAY, 0);
	symbol_A->type = t;
	TYPE type_A = &tbl_types[symbol_A->type];
	assert(symbol_A->type == A_type);

	tbl_create_type(&t, FORM_RECORD, 0);
	type_A->base = t;
	TYPE type_R = &tbl_types[type_A->base];
	assert(type_A->base == R_type);

	tbl_open_scope();
	type_R->base = tbl_top_scope;
	assert(type_R->base == R_type_scope);
	assert(tbl_top_scope == type_R->base);

	tbl_set_marker("i");
	SYMBOL symbol_i = tbl_def_symbol(&s, CLASS_VAR, -1);
	symbol_i->type = tbl_integer_type;
	symbol_i->is_public = is_public[1];
	assert(s == i);

	tbl_set_marker("j");
	SYMBOL symbol_j = tbl_def_symbol(&s, CLASS_VAR, -1);
	symbol_j->type = tbl_integer_type;
	symbol_j->is_public = is_public[2];
	assert(s == j);

	tbl_close_scope();
	assert(tbl_top_scope == scope);

	tbl_dealloc_scope(tbl_top_scope, 1);
}

static void test_deallocate_anonymous_record_inside_array(void)
{
	int s;

	int is_public[3][3] = {
		{ 0, 0, 0 },
		{ 1, 0, 0 },
		{ 1, 1, 0 },
	};

	int scope = INIT_LAST_SYMBOL + 1;
	int A = INIT_LAST_SYMBOL + 2;
	int A_type = INIT_LAST_TYPE + 1;
	int R_type = INIT_LAST_TYPE + 2;
	int R_type_scope = INIT_LAST_SYMBOL + 3;
	int i = INIT_LAST_SYMBOL + 4;
	int j = INIT_LAST_SYMBOL + 5;

	/*

	A* = ARRAY 10 OF RECORD i : INTEGER; j : INTEGER END;

	free i j R_type_scope R_type A_type A scope

	*/

	deallocate_anonymous_record_inside_array(is_public[0]);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == scope);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == A);
	af_alloc(&tbl_af_types, &s);
	assert(s == A_type);
	af_alloc(&tbl_af_types, &s);
	assert(s == R_type);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == R_type_scope);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == j);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == i);

	/*

	A = ARRAY 10 OF RECORD i : INTEGER; j : INTEGER END;

	free i j R_type_scope R_type

	*/

	deallocate_anonymous_record_inside_array(is_public[1]);
	af_alloc(&tbl_af_types, &s);
	assert(s == R_type);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == R_type_scope);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == j);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == i);

	/*

	A* = ARRAY 10 OF RECORD i* : INTEGER; j : INTEGER END;

	free j R_type

	*/

	deallocate_anonymous_record_inside_array(is_public[2]);
	af_alloc(&tbl_af_types, &s);
	assert(s == R_type);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == j);
}


static void deallocate_record(int is_public[4])
{
	int s;
	int t;

	/* scope with record R and members i j and array a */
	tbl_init();
	int scope = INIT_LAST_SYMBOL + 1;
	int R = INIT_LAST_SYMBOL + 2;
	int R_type = INIT_LAST_TYPE + 1;
	int R_type_scope = INIT_LAST_SYMBOL + 3;
	int i = INIT_LAST_SYMBOL + 4;
	int j = INIT_LAST_SYMBOL + 5;
	int a = INIT_LAST_SYMBOL + 6;
	int a_type = INIT_LAST_TYPE + 2;

	tbl_open_scope();
	assert(tbl_top_scope == scope);

	tbl_set_marker("R");
	tbl_def_symbol(&s, CLASS_TYPE, -1);
	SYMBOL symbol_R = &tbl_symbols[s];
	symbol_R->is_public = is_public[0];
	assert(s == R);

	tbl_create_type(&t, FORM_RECORD, 0);
	symbol_R->type = t;
	TYPE type_R = &tbl_types[symbol_R->type];
	assert(symbol_R->type == R_type);

	tbl_open_scope();
	type_R->base = tbl_top_scope;
	assert(type_R->base == R_type_scope);
	assert(tbl_top_scope == type_R->base);

	tbl_set_marker("i");
	tbl_def_symbol(&s, CLASS_VAR, -1);
	SYMBOL symbol_i = &tbl_symbols[s];
	symbol_i->is_public = is_public[1];
	symbol_i->type = tbl_integer_type;
	assert(s == i);

	tbl_set_marker("j");
	tbl_def_symbol(&s, CLASS_VAR, -1);
	SYMBOL symbol_j = &tbl_symbols[s];
	symbol_j->is_public = is_public[2];
	symbol_j->type = tbl_integer_type;
	assert(s == j);

	tbl_set_marker("a");
	tbl_def_symbol(&s, CLASS_VAR, -1);
	SYMBOL symbol_a = &tbl_symbols[s];
	symbol_a->is_public = is_public[3];
	assert(s == a);

	tbl_create_type(&t, FORM_ARRAY, 0);
	symbol_a->type = t;
	TYPE type_a = &tbl_types[symbol_a->type];
	type_a->base = tbl_integer_type;
	assert(symbol_a->type == a_type);

	tbl_close_scope();
	assert(tbl_top_scope == scope);

	tbl_dealloc_scope(tbl_top_scope, 1);
}

static void test_deallocate_record(void)
{
	int s;

	int is_public[3][4] = {
		{ 0, 0, 0, 0 },
		{ 1, 0, 0, 0 },
		{ 1, 0, 0, 1 }
	};

	int scope = INIT_LAST_SYMBOL + 1;
	int R = INIT_LAST_SYMBOL + 2;
	int R_type = INIT_LAST_TYPE + 1;
	int R_type_scope = INIT_LAST_SYMBOL + 3;
	int i = INIT_LAST_SYMBOL + 4;
	int j = INIT_LAST_SYMBOL + 5;
	int a = INIT_LAST_SYMBOL + 6;
	int a_type = INIT_LAST_TYPE + 2;

	/*

	R = RECORD
		i : INTEGER;
		h : INTEGER;
		a : ARRAY[] OF INTEGER
	END;

	free i j a_type a R_type_scope R_type R scope

	*/

	deallocate_record(is_public[0]);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == scope);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == R);
	af_alloc(&tbl_af_types, &s);
	assert(s == R_type);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == R_type_scope);
	af_alloc(&tbl_af_types, &s);
	assert(s == a_type);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == a);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == j);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == i);

	/*

	R* = RECORD
		i : INTEGER;
		h : INTEGER;
		a : ARRAY[] OF INTEGER
	END;

	free i j a_type a R_type_scope

	*/

	deallocate_record(is_public[1]);
	assert(tbl_types[R_type].base == -1);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == R_type_scope);
	af_alloc(&tbl_af_types, &s);
	assert(s == a_type);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == a);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == j);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == i);

	/*

	R* = RECORD
		i : INTEGER;
		h : INTEGER;
		a* : ARRAY[] OF INTEGER
	END;

	free i j

	*/

	deallocate_record(is_public[2]);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == j);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == i);
}

static void deallocate_procedure(int is_public[3])
{
	int s;

	/* scope with procedure P, params a b, variables c, d */
	tbl_init();
	int scope = INIT_LAST_SYMBOL + 1;
	int P = INIT_LAST_SYMBOL + 2;
	int P_scope = INIT_LAST_SYMBOL + 3;
	int a = INIT_LAST_SYMBOL + 4;
	int b = INIT_LAST_SYMBOL + 5;
	int c = INIT_LAST_SYMBOL + 6;
	int d = INIT_LAST_SYMBOL + 7;

	tbl_open_scope();
	assert(tbl_top_scope == scope);

	tbl_set_marker("P");
	tbl_def_symbol(&s, CLASS_PROC, -1);
	tbl_symbols[s].is_public = is_public[0];
	assert(s == P);

	tbl_open_scope();
	assert(tbl_top_scope == P_scope);
	tbl_symbols[P].scope = tbl_top_scope;

	tbl_set_marker("a");
	tbl_def_symbol(&s, CLASS_VAR, -1);
	tbl_symbols[s].is_public = is_public[1];
	assert(s == a);

	tbl_set_marker("b");
	tbl_def_symbol(&s, CLASS_VAR, -1);
	tbl_symbols[s].is_public = is_public[2];
	assert(s == b);

	tbl_set_marker("c");
	tbl_def_symbol(&s, CLASS_VAR, -1);
	assert(s == c);

	tbl_set_marker("d");
	tbl_def_symbol(&s, CLASS_VAR, -1);
	assert(s == d);

	tbl_close_scope();
	assert(tbl_top_scope == scope);

	tbl_dealloc_scope(tbl_top_scope, 1);
}

static void test_deallocate_procedure(void)
{
	int s;

	int is_public[2][3] = {
		{ 1, 1, 1 },
		{ 0, 0, 0 }
	};

	int scope = INIT_LAST_SYMBOL + 1;
	int P = INIT_LAST_SYMBOL + 2;
	int P_scope = INIT_LAST_SYMBOL + 3;
	int a = INIT_LAST_SYMBOL + 4;
	int b = INIT_LAST_SYMBOL + 5;
	int c = INIT_LAST_SYMBOL + 6;
	int d = INIT_LAST_SYMBOL + 7;

	/*

	PROCEDURE P*(a : INTEGER; b : INTEGER);
	VAR c : INTEGER; d : INTEGER;

	free c d

	*/

	deallocate_procedure(is_public[0]);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == d);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == c);

	/*

	PROCEDURE P(a : INTEGER; b : INTEGER);
	VAR c : INTEGER; d : INTEGER;

	free a b c d P_scope P scope

	*/

	deallocate_procedure(is_public[1]);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == scope);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == P);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == P_scope);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == d);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == c);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == b);
	af_alloc(&tbl_af_symbols, &s);
	assert(s == a);
}

static void test_def_use_symbol(void)
{
	int s;

	tbl_init();

	tbl_open_scope();
	assert(tbl_top_scope == INIT_LAST_SYMBOL + 1);

	tbl_set_marker("foo");
	tbl_def_symbol(&s, CLASS_CONST, -1);
	assert(s == INIT_LAST_SYMBOL + 2);

	tbl_set_marker("bar");
	tbl_def_symbol(&s, CLASS_CONST, -1);
	assert(s == INIT_LAST_SYMBOL + 3);

	tbl_set_marker("foo");
	tbl_def_symbol(&s, CLASS_CONST, -1);
	assert(s == -1);

	tbl_set_marker("bar");
	s = tbl_top_scope;
	tbl_find_symbol(&s);
	assert(s == INIT_LAST_SYMBOL + 2);
	assert(tbl_symbols[s].next == INIT_LAST_SYMBOL + 3);

	tbl_set_marker("BITO");
	tbl_use_symbol(&s, -1);
	assert(s == 8);

	tbl_set_marker("gugus");
	tbl_use_symbol(&s, -1);
	assert(s == -1);

	tbl_set_marker("foo");
	tbl_use_symbol(&s, -1);
	assert(s == INIT_LAST_SYMBOL + 2);

	tbl_set_marker("bar");
	tbl_use_symbol(&s, -1);
	assert(s == INIT_LAST_SYMBOL + 3);
}

static void test_open_module(void)
{
	tbl_init();

	int m1 = tbl_open_module("gugus");
	assert(m1 == 0);
	assert(tbl_modules[m1].scope == INIT_LAST_SYMBOL + 1);

	int m2 = tbl_open_module("foobar");
	assert(m2 == 1);
	assert(tbl_modules[m2].scope == INIT_LAST_SYMBOL + 2);

	int m3 = tbl_open_module("gugus");
	assert(m3 == m1);

	int m4 = tbl_open_module("foobar");
	assert(m4 == m2);
}

int main(void)
{
	test_init();
	test_deallocate_scope_with_childs();
	test_deallocate_anonymous_record_inside_array();
	test_deallocate_record();
	test_deallocate_procedure();
	test_def_use_symbol();
	test_open_module();
	return 0;
}
