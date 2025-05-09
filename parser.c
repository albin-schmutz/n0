#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "opcode.h"
#include "cpu.h"
#include "runtime.h"
#include "clazz.h"
#include "tables.h"
#include "source.h"
#include "scanner.h"
#include "generator.h"
#include "parser.h"


/* Error raisers */


static void raise_error_not_defined(SCANNER scan)
{
	scan_raise_error(scan, "Not defined");
}

static void raise_error_factor(SCANNER scan)
{
	scan_raise_error(scan, "Factor ?");
}

static void raise_error_l_value(SCANNER scan)
{
	scan_raise_error(scan, "L-Value ?");
}


/* Checkers */


static void check_modulename(SCANNER scan)
{
	if (strcmp(scan->ident, scan->modulename)) {
		scan_raise_error(scan, "Module filename mismatch");
	}
}

static void check_public(SCANNER scan, SYMBOL symbol)
{
	scan_take(scan);
	if (scan->token == tk_asterisk) {
		if (tbl_level != 0) {
			scan_raise_error(scan, "Public not possible");
		}
		symbol->is_public = 1;
		scan_take(scan);
	}
}

static void check_bool(SCANNER scan, GEN x)
{
	if (tbl_types[x->type].form != FORM_BOOL) {
		scan_raise_error(scan, "Bool ?");
	}
}

static void check_char(SCANNER scan, GEN x)
{
	if (tbl_types[x->type].form != FORM_CHAR) {
		scan_raise_error(scan, "Char ?");
	}
}

static void check_int(SCANNER scan, GEN x)
{
	if (tbl_types[x->type].form != FORM_INT) {
		scan_raise_error(scan, "Int ?");
	}
}

static void check_type(SCANNER scan, int x, int y, int strict)
{
	if (x != y && (strict || tbl_types[x].form != FORM_INT || tbl_types[y].form != FORM_INT)) {
		scan_raise_error(scan, "Incompatible types");
	}
}

static void check_not_const(SCANNER scan, GEN x)
{
	if (x->mode == CLASS_CONST) {
		scan_raise_error(scan, "Branch ops forbidden");
	}
}


/* Identifiers */


static SYMBOL ident_def(SCANNER scan, int *s, int clazz)
{
	tbl_set_marker(scan->ident);
	SYMBOL symbol = tbl_def_symbol(s, clazz, scan->module_id);
	if (!symbol) {
		scan_raise_error(scan, "Multiple definition");
	}
	symbol->level = -1;
	return symbol;
}

static SYMBOL ident_use(SCANNER scan, int *s)
{
	tbl_set_marker(scan->ident);
	SYMBOL symbol = tbl_use_symbol(s, -1);
	if (!symbol) {
		raise_error_not_defined(scan);
	}
	if (symbol->clazz == CLASS_IMPORT) {
		scan_take(scan);
		scan_expect(scan, tk_dot);
		scan_take(scan);
		scan_expect(scan, tk_ident);
		tbl_set_marker(scan->ident);
		symbol = tbl_use_symbol(s, *s);
	}

	if (!symbol ||
			(symbol->clazz != CLASS_PROC &&
			symbol->level != 0 &&
			symbol->level != tbl_level)) {
		raise_error_not_defined(scan);
	}
	return symbol;
}


/* Generators */


static void make_gen(SCANNER scan, GEN gen, SYMBOL symbol, int m)
{
	assert(scan);
	assert(gen);
	assert(symbol);

	gen->mode = symbol->clazz;
	if (symbol->type == -1) {
		gen->type = -1;
		gen->size = 0;
	} else {
		gen->type = symbol->type;
		gen->size = tbl_types[gen->type].size;
	}
	gen->val = symbol->val;
	gen->val2 = symbol->val2;
	if (gen->mode == CLASS_VAR) {
		if (symbol->level == 0) {
			gen->reg = PC;
			if (m != scan->module_id) {
				int offset =
					tbl_modules[scan->module_id].prog_addr -
					tbl_modules[m].prog_addr;
				gen->val -= offset;
			}
		} else {
			gen->reg = FP;
		}
	}
	gen_deref(gen);
}


/* Expressions */


static void expr(SCANNER scan, GEN x);

static void sfunc(SCANNER scan, GEN x, int nr)
{
	struct GEN y;

	scan_take(scan);
	scan_expect(scan, tk_paren_l);
	scan_take(scan);
	expr(scan, x);
	if (nr == 1) { // CHR
		check_int(scan, x);
		x->type = tbl_char_type;
		scan_expect(scan, tk_paren_r);
		scan_take(scan);
	} else if (nr == 2) { // ORD
		check_char(scan, x);
		x->type = tbl_integer_type;
		scan_expect(scan, tk_paren_r);
		scan_take(scan);
	} else if (nr >= 3 && nr <= 5) { // ASH, BITA, BITO
		check_int(scan, x);
		scan_expect(scan, tk_comma);
		scan_take(scan);
		y.exp_const = 0;
		expr(scan, &y);
		if (nr == 3) {
			gen_ash(x, &y);
		} else if (nr == 4) {
			gen_bita(x, &y);
		} else {
			gen_bito(x, &y);
		}
		check_int(scan, &y);
		scan_expect(scan, tk_paren_r);
		scan_take(scan);
	}
}

static void selector(SCANNER scan, GEN x)
{
	struct GEN y;
	TYPE type;
	int array_len;
	TYPE array_base;
	int s;
	SYMBOL symbol;

	while (scan->token == tk_brack_l || scan->token == tk_dot) {
		type = &tbl_types[x->type];
		if (scan->token == tk_brack_l) {
			if (type->form == FORM_ARRAY) {
				scan_take(scan);
				y.exp_const = 0;
				expr(scan, &y);
				check_int(scan, &y);
				array_len = type->length;
				if (y.mode == CLASS_CONST && (y.val < 0 || y.val >= array_len)) {
					scan_raise_error(scan, "Bad index");
				}
				array_base = &tbl_types[type->base];
				gen_index(x, &y, array_len, array_base->size);
				scan_expect(scan, tk_brack_r);
				x->type = type->base;
				x->size = array_base->size;
			} else {
				scan_raise_error(scan, "Array ?");
			}
		} else { // scan->token == tk_dot
			if (type->form == FORM_RECORD) {
				scan_take(scan);
				scan_expect(scan, tk_ident);
				tbl_set_marker(scan->ident);
				s = type->base;
				tbl_find_symbol(&s);
				s = tbl_symbols[s].next;
				if (s == tbl_marker) {
					raise_error_not_defined(scan);
				}
				symbol = &tbl_symbols[s];
				x->type = symbol->type;
				x->size = tbl_types[x->type].size;
				gen_field(x, symbol->val);
			} else {
				scan_raise_error(scan, "Record ?");
			}
		}
		scan_take(scan);
	}
}

static void call_stmt(SCANNER scan, SYMBOL proc);

static void factor_ident(SCANNER scan, GEN x, SYMBOL symbol)
{
	if (symbol->clazz == CLASS_SFUNC) {
		sfunc(scan, x, symbol->val);
	} else if (symbol->clazz == CLASS_CONST || symbol->clazz == CLASS_VAR) {
		make_gen(scan, x, symbol, symbol->scope);
		if (x->exp_const && x->mode != CLASS_CONST) {
			scan_raise_error(scan, "Const ?");
		}
		scan_take(scan);
		selector(scan, x);
	} else {
		raise_error_factor(scan);
	}
}

static void factor(SCANNER scan, GEN x)
{
	SYMBOL symbol;
	int s;

	if (scan->token == kw_true) {
		x->mode = CLASS_CONST;
		x->type = tbl_boolean_type;
		x->val = 1;
		scan_take(scan);
	} else if (scan->token == kw_false) {
		x->mode = CLASS_CONST;
		x->type = tbl_boolean_type;
		x->val = 0;
		scan_take(scan);
	} else if (scan->token == tk_char) {
		x->mode = CLASS_CONST;
		x->type = tbl_char_type;
		x->val = scan->val;
		scan_take(scan);
	} else if (scan->token == tk_int) {
		x->mode = CLASS_CONST;
		x->type = tbl_long_int_type;
		x->val = scan->val;
		scan_take(scan);
	} else if (scan->token == tk_ident) {
		symbol = ident_use(scan, &s);
		factor_ident(scan, x, symbol);
	} else if (scan->token == tk_paren_l) {
		scan_take(scan);
		expr(scan, x);
		scan_expect(scan, tk_paren_r);
		scan_take(scan);
	} else if (scan->token == tk_tilde) {
		scan_take(scan);
		factor(scan, x);
		check_bool(scan, x);
		gen_not(x);
	} else {
		raise_error_factor(scan);
	}
}

static void term(SCANNER scan, GEN x)
{
	int op;
	struct GEN y;

	factor(scan, x);
	while (scan->token == tk_asterisk ||
			scan->token == kw_div ||
			scan->token == kw_mod ||
			scan->token == tk_amper) {
		op = scan->token;
		if (op == tk_amper) {
			check_not_const(scan, x);
			check_bool(scan, x);
			gen_and(x);
		} else {
			check_int(scan, x);
		}
		scan_take(scan);
		y.exp_const = x->exp_const;
		factor(scan, &y);
		check_type(scan, x->type, y.type, 0);
		if (op == tk_asterisk) {
			gen_mul(x, &y);
		} else if (op == kw_div) {
			gen_div(x, &y);
		} else if (op == kw_mod) {
			gen_mod(x, &y);
		} else { // tk_amper
			gen_and2(x, &y);
		}
	}
}

static void simple_expr(SCANNER scan, GEN x)
{
	int op;
	struct GEN y;

	if (scan->token == tk_plus) {
		scan_take(scan);
		term(scan, x);
		check_int(scan, x);
	} else if (scan->token == tk_minus) {
		scan_take(scan);
		term(scan, x);
		check_int(scan, x);
		gen_neg(x);
	} else {
		term(scan, x);
	}
	while (scan->token == tk_plus ||
			scan->token == tk_minus ||
			scan->token == kw_or) {
		op = scan->token;
		if (op == kw_or) {
			check_not_const(scan, x);
			check_bool(scan, x);
			gen_or(x);
		} else {
			check_int(scan, x);
		}
		scan_take(scan);
		y.exp_const = x->exp_const;
		term(scan, &y);
		check_type(scan, x->type, y.type, 0);
		if (op == tk_plus) {
			gen_add(x, &y);
		} else if (op == tk_minus) {
			gen_sub(x, &y);
		} else { // kw_or
			gen_or2(x, &y);
		}
	}
}

static void expr(SCANNER scan, GEN x)
{
	int op;
	struct GEN y;

	simple_expr(scan, x);
	if (scan->token >= tk_eq && scan->token <= tk_geq) {
		op = scan->token;
		scan_take(scan);
		y.exp_const = x->exp_const;
		simple_expr(scan, &y);
		check_type(scan, x->type, y.type, 0);
		if (op == tk_eq) {
			gen_eq(x, &y);
		} else if (op == tk_neq) {
			gen_neq(x, &y);
		} else if (op == tk_lss) {
			gen_lss(x, &y);
		} else if (op == tk_leq) {
			gen_leq(x, &y);
		} else if (op == tk_gtr) {
			gen_gtr(x, &y);
		} else {
			gen_geq(x, &y);
		}
		x->type = tbl_boolean_type;
	}
}


/* Statements */


static void statements(SCANNER scan);

static void param(SCANNER scan, int *s, SYMBOL symbol, int *count)
{
	struct GEN x;
	int is_ref;

	if (*count == 0) {
		scan_raise_error(scan, "Too many params");
	}
	(*count)--;
	x.exp_const = 0;
	expr(scan, &x);
	is_ref = symbol->val2 == 1;
	check_type(scan, x.type, symbol->type, is_ref);
	if (is_ref && x.mode != CLASS_VAR) {
		raise_error_l_value(scan);
	}
	gen_param(&x, is_ref);
	*s = symbol->next;
}

static void call_stmt(SCANNER scan, SYMBOL proc)
{
	int par_count = proc->val3;
	int s = tbl_symbols[proc->scope].next;
	scan_take(scan);
	if (scan->token == tk_paren_l) {
		scan_take(scan);
		if (scan->token != tk_paren_r) {
			param(scan, &s, &tbl_symbols[s], &par_count);
			while (scan->token == tk_comma) {
				scan_take(scan);
				param(scan, &s, &tbl_symbols[s], &par_count);
			}
			scan_expect(scan, tk_paren_r);
		}
		scan_take(scan);
	}
	if (par_count > 0) {
		scan_raise_error(scan, "Too few params");
	}
	if (proc->val == LONG_MIN) { // forward call
		gen_fcall(&proc->val2);
	} else {
		gen_bcall(proc->val);
	}
}

static void assert_stmt(SCANNER scan)
{
	struct GEN x;
	int assert_line;

	assert_line = scan->source.line;
	scan_take(scan);
	scan_expect(scan, tk_paren_l);
	scan_take(scan);
	x.exp_const = 0;
	expr(scan, &x);
	check_bool(scan, &x);
	gen_assert(&x, assert_line);
	scan_expect(scan, tk_paren_r);
	scan_take(scan);
}

static void sys_stmt(SCANNER scan)
{
	struct GEN x;
	struct GEN y;
	int call_nr;

	scan_take(scan);
	scan_expect(scan, tk_paren_l);
	scan_take(scan);
	x.exp_const = 1;
	expr(scan, &x);
	check_int(scan, &x);
	call_nr = x.val;
	scan_expect(scan, tk_comma);
	scan_take(scan);
	x.exp_const = 0;
	expr(scan, &x);
	check_type(scan, x.type, tbl_long_int_type, 1);
	if (x.mode != CLASS_VAR) {
		raise_error_l_value(scan);
	}
	scan_expect(scan, tk_comma);
	scan_take(scan);
	y.exp_const = 0;
	expr(scan, &y);
	gen_sys(call_nr, &x, &y);
	scan_expect(scan, tk_paren_r);
	scan_take(scan);
}

static void sproc_stmt(SCANNER scan, int nr)
{
	if (nr == 1) {
		assert_stmt(scan);
	} else if (nr == 2) {
		sys_stmt(scan);
	}
}

static void assign_stmt(SCANNER scan, GEN x)
{
	struct GEN gen2;

	if (x->mode != CLASS_VAR) {
		raise_error_l_value(scan);
	}
	scan_take(scan);
	selector(scan, x);
	scan_expect(scan, tk_assign);
	scan_take(scan);
	gen2.exp_const = 0;
	expr(scan, &gen2);
	check_type(scan, x->type, gen2.type, 0);
	gen_store(x, &gen2);
}

static void if_stmt(SCANNER scan)
{
	struct GEN x;
	int label = 0;

	scan_take(scan);
	x.exp_const = 0;
	expr(scan, &x);
	check_bool(scan, &x);
	gen_cond_fjump(&x);
	scan_expect(scan, kw_then);
	scan_take(scan);
	statements(scan);
	while (scan->token == kw_elsif) {
		scan_take(scan);
		gen_fjump(&label);
		gen_fix_jumps(x.val);
		expr(scan, &x);
		check_bool(scan, &x);
		gen_cond_fjump(&x);
		scan_expect(scan, kw_then);
		scan_take(scan);
		statements(scan);
	}
	if (scan->token == kw_else) {
		scan_take(scan);
		gen_fjump(&label);
		gen_fix_jumps(x.val);
		statements(scan);
	} else {
		gen_fix_jumps(x.val);
	}
	gen_fix_jumps(label);
	scan_expect(scan, kw_end);
	scan_take(scan);
}

static void repeat_stmt(SCANNER scan)
{
	struct GEN x;
	int label;

	scan_take(scan);
	gen_get_pc_rel(&label);
	statements(scan);
	scan_expect(scan, kw_until);
	scan_take(scan);
	x.exp_const = 0;
	expr(scan, &x);
	check_bool(scan, &x);
	gen_cond_bjump(&x, label);
}

static void while_stmt(SCANNER scan)
{
	struct GEN x;
	int label;

	scan_take(scan);
	gen_get_pc_rel(&label);
	x.exp_const = 0;
	expr(scan, &x);
	check_bool(scan, &x);
	gen_cond_fjump(&x);
	scan_expect(scan, kw_do);
	scan_take(scan);
	statements(scan);
	gen_bjump(label);
	gen_fix_jumps(x.val);
	scan_expect(scan, kw_end);
	scan_take(scan);
}

static void statement_ident(SCANNER scan,  GEN x, SYMBOL symbol)
{
	if (symbol->clazz == CLASS_PROC) {
		call_stmt(scan, symbol);
	} else {
		make_gen(scan, x, symbol, symbol->scope);
		if (x->mode == CLASS_SPROC) {
			sproc_stmt(scan, (int)x->val);
		} else {
			assign_stmt(scan, x);
		}
	}
}

static void statement(SCANNER scan)
{
	struct GEN x;
	int s;

	if (scan->token == tk_ident) {
		ident_use(scan, &s);
		statement_ident(scan, &x, &tbl_symbols[s]);
	} else if (scan->token == kw_if) {
		if_stmt(scan);
	} else if (scan->token == kw_repeat) {
		repeat_stmt(scan);
	} else if (scan->token == kw_while) {
		while_stmt(scan);
	} else {
		scan_raise_error(scan, "Statement ?");
	}
}

static void statements(SCANNER scan)
{
	statement(scan);
	while (scan->token == tk_semic) {
		scan_take(scan);
		statement(scan);
	}
}


/* Declarations */


static void declarations(SCANNER scan, int *addr);

static void type_def(SCANNER scan, int *t, int *s, int clazz);

static void array_type(SCANNER scan, int *t)
{
	struct GEN x;
	TYPE type;
	int size;
	int s;

	scan_take(scan);
	x.exp_const = 1;
	expr(scan, &x);
	check_int(scan, &x);
	type = tbl_create_type(t, FORM_ARRAY, 0);
	size = x.val;
	type->length = size;
	scan_expect(scan, kw_of);
	scan_take(scan);
	type_def(scan, &type->base, &s, 0);
	size *= tbl_types[type->base].size;
	size = run_align(size, WORD_SIZE);
	type->size = size;
}

static void record_field(SCANNER scan, int *addr)
{
	int t;
	int s;

	type_def(scan, &t, &s, CLASS_FIELD);
	TYPE type = &tbl_types[t];
	tbl_symbols[s].val = *addr;
	*addr += type->size;
}

static void record_type(SCANNER scan, int *t)
{
	int addr = 0;

	tbl_open_scope();
	scan_take(scan);
	record_field(scan, &addr);
	while (scan->token == tk_semic) {
		scan_take(scan);
		record_field(scan, &addr);
	}
	addr = run_align(addr, WORD_SIZE);
	scan_expect(scan, kw_end);
	scan_take(scan);
	TYPE type = tbl_create_type(t, FORM_RECORD, addr);
	type->base = tbl_top_scope;
	tbl_close_scope();
}

static void type_def(SCANNER scan, int *t, int *s, int clazz)
{
	SYMBOL symbol = NULL;
	int s2;
	SYMBOL symbol2;

	*s = -1;
	if (clazz) {
		symbol = ident_def(scan, s, clazz);
		check_public(scan, symbol);
		if (clazz == CLASS_TYPE) {
			scan_expect(scan, tk_eq);
			scan_take(scan);
		} else {
			scan_expect(scan, tk_colon);
			scan_take(scan);
		}
	}
	*t = -1;
	if (scan->token == tk_ident) {
		symbol2 = ident_use(scan, &s2);
		if (clazz && *s == s2) {
			raise_error_not_defined(scan);
		}
		if (symbol2->clazz == CLASS_TYPE) {
			*t = symbol2->type;
			scan_take(scan);
		}
	} else if (scan->token == kw_array) {
		array_type(scan, t);
	} else if (scan->token == kw_record) {
		record_type(scan, t);
	}
	if (*t == -1) {
		scan_raise_error(scan, "Type ?");
	}
	if (symbol) {
		symbol->type = *t;
		symbol->level = tbl_level;
	}
}

static void const_decls(SCANNER scan)
{
	struct GEN x;
	SYMBOL symbol;
	int s;

	while (scan->token == tk_ident) {
		symbol = ident_def(scan, &s, CLASS_CONST);
		check_public(scan, symbol);
		scan_expect(scan, tk_eq);
		scan_take(scan);
		x.exp_const = 1;
		expr(scan, &x);
		symbol->type = x.type;
		symbol->val = x.val;
		symbol->level = tbl_level; /* definition finished */
		scan_expect(scan, tk_semic);
		scan_take(scan);
	}
}

static void type_decls(SCANNER scan)
{
	int t;
	int s;

	while (scan->token == tk_ident) {
		type_def(scan, &t, &s, CLASS_TYPE);
		tbl_types[t].owner = s;
		scan_expect(scan, tk_semic);
		scan_take(scan);
	}
}

static void var_decls(SCANNER scan, int *addr)
{
	int t;
	int s;
	int size;

	while (scan->token == tk_ident) {
		type_def(scan, &t, &s, CLASS_VAR);
		size = tbl_types[t].size;
		*addr -= size;
		tbl_symbols[s].val = *addr;
		scan_expect(scan, tk_semic);
		scan_take(scan);
	}
	*addr = -run_align(abs(*addr), WORD_SIZE);
}

static void declarations(SCANNER scan, int *addr)
{
	if (scan->token == kw_const) {
		scan_take(scan);
		const_decls(scan);
	}
	if (scan->token == kw_type) {
		scan_take(scan);
		type_decls(scan);
	}
	if (scan->token == kw_var) {
		scan_take(scan);
		var_decls(scan, addr);
	}

}

static void params(SCANNER scan, int *addr, int *count)
{
	SYMBOL symbol;
	int deref = 0;
	int t = -1;
	int s = -1;
	int offset;

	if (scan->token == kw_var) {
		scan_take(scan);
		deref = 1;
	}
	scan_expect(scan, tk_ident);
	type_def(scan, &t, &s, CLASS_VAR);
	symbol = &tbl_symbols[s];
	symbol->val2 = deref;
	if (!deref && tbl_types[t].form >= FORM_ARRAY) {
		scan_raise_error(scan, "Scalar type ?");
	}
	*addr += WORD_SIZE;
	offset = *addr;
	if (scan->token == tk_semic) {
		scan_take(scan);
		params(scan, addr, count);
	}
	symbol->val = *addr - offset + 2 * WORD_SIZE;
	symbol->is_public = 1;
	(*count)++;
}

static void procedures(SCANNER scan);

static void procedure(SCANNER scan)
{
	SYMBOL symbol;
	int proc;
	int par_size;
	int par_count;
	int var_size;

	scan_expect(scan, tk_ident);
	symbol = ident_def(scan, &proc, CLASS_PROC);
	check_public(scan, symbol);
	symbol->level = tbl_level; // definition finished
	symbol->val = LONG_MIN; // forward call marker (call statement)
	symbol->val2 = 0;
	tbl_open_scope();
	tbl_level++;
	symbol->scope = tbl_top_scope;
	par_size = 0;
	par_count = 0;
	if (scan->token == tk_paren_l) {
		scan_take(scan);
		if (scan->token != tk_paren_r) {
			params(scan, &par_size, &par_count);
			scan_expect(scan, tk_paren_r);
		}
		scan_take(scan);
	}
	symbol->val3 = par_count;
	scan_expect(scan, tk_semic);
	scan_take(scan);
	var_size = 0;
	declarations(scan, &var_size);
	procedures(scan);
	if (symbol->val == LONG_MIN) {
		gen_fix_jumps(symbol->val2);
	}
	gen_get_pc_abs(&symbol->val);
	gen_enter(var_size);
	scan_expect(scan, kw_begin);
	scan_take(scan);
	statements(scan);
	scan_expect(scan, kw_end);
	scan_take(scan);
	scan_expect(scan, tk_ident);
	if (strcmp(scan->ident, symbol->name)) {
		scan_raise_error(scan, "Procedure name mismatch");
	}
	scan_take(scan);
	scan_expect(scan, tk_semic);
	tbl_dealloc_scope(tbl_top_scope, 1);
	if (tbl_symbols[tbl_top_scope].next == tbl_marker) {
		symbol->scope = -1;
	}
	tbl_level--;
	tbl_close_scope();
	gen_return(par_size);
	scan_take(scan);
}

static void procedures(SCANNER scan)
{
	while (scan->token == kw_procedure) {
		scan_take(scan);
		procedure(scan);
	}
}


/* Module and Import */


static void module(SCANNER scan);

static void import(SCANNER scan)
{
	struct SCANNER new_scan;
	int imp;
	int m;
	int loaded;

	scan_take(scan);
	scan_expect(scan, tk_ident);
	ident_def(scan, &imp, CLASS_IMPORT);
	strcpy(new_scan.modulename, scan->ident);
	scan_take(scan);
	if (scan->token == tk_assign) {
		scan_take(scan);
		scan_expect(scan, tk_ident);
		strcpy(new_scan.modulename, scan->ident);
		scan_take(scan);
	}
	m = tbl_open_module(new_scan.modulename);
	loaded = tbl_modules[m].loaded;
	if (loaded == 0) {
		scan_raise_error(scan, "Cyclic imports");
	} else if (loaded == -1) { /* new */
		scan_create(&new_scan, scan);
		module(&new_scan);
	}
	tbl_symbols[imp].scope = tbl_modules[m].scope;
}

static void module(SCANNER scan)
{
	int64_t prog_addr;
	int var_size;
	int entry_point;
	int prog_size;

	scan_take(scan);
	scan_expect(scan, kw_module);
	scan_take(scan);
	scan_expect(scan, tk_ident);
	check_modulename(scan);

	scan->module_id = tbl_open_module(scan->modulename);
	tbl_modules[scan->module_id].loaded = 0; /* not loaded */

	scan_take(scan);
	scan_expect(scan, tk_semic);
	scan_take(scan);
	if (scan->token == kw_import) {
		import(scan);
		while (scan->token == tk_comma) import(scan);
		scan_expect(scan, tk_semic);
		scan_take(scan);
	}

	var_size = 0;
	declarations(scan, &var_size);
	prog_addr = run_create_module(scan->modulename, var_size);
	gen_start(prog_addr);
	tbl_modules[scan->module_id].prog_addr = prog_addr;
	procedures(scan);
	entry_point = -1;
	if (scan->token == kw_begin) {
		gen_get_pc_rel(&entry_point);
		scan_take(scan);
		statements(scan);
		gen_exit();
	}

	scan_expect(scan, kw_end);
	scan_take(scan);
	scan_expect(scan, tk_ident);
	check_modulename(scan);
	scan_take(scan);
	scan_expect(scan, tk_dot);

	tbl_modules[scan->module_id].loaded = 1; /* loaded */
	tbl_close_scope();

	gen_get_pc_rel(&prog_size);
	run_exec_module(prog_size, entry_point);
}

void parser(char *modulename)
{
	struct SCANNER parent;
	struct SCANNER scan;

	tbl_init();
	strcpy(parent.modulename, "ROOT");
	strcpy(scan.modulename, modulename);
	scan_create(&scan, &parent);
	module(&scan);
}
