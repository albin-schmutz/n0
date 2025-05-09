#include <assert.h>
#include <stdint.h>
#include "opcode.h"
#include "emitter.h"
#include "clazz.h"
#include "generator.h"

#define WORD_SIZE 8
#define INSTR_SIZE 4

static struct EMITTER emit;
static int regs[FP];

static void alloc_reg(int *reg)
{
	int i = 0;

	while (i < FP && regs[i] != 0) i++;
	assert(i < FP);
	regs[i] = 1;
	*reg = i;
}

static void free_reg(int reg)
{
	if (reg < FP) {
		regs[reg] = 0;
	}
}

void gen_start(int64_t pc)
{
	int i = 0;

	while (i < FP) regs[i++] = 0;
	emit_init(&emit, pc);
}

void gen_exit(void)
{
	emit.opcode.oc = POP;
	emit.opcode.a = PC;
	emit.opcode.b = SP;
	emit.opcode.imm = 0;
	emit_a_b_imm(&emit);
}

void gen_get_pc_rel(int *pc)
{
	*pc = emit.pc * INSTR_SIZE;
}

void gen_get_pc_abs(int64_t *pc)
{
	*pc = (int64_t)emit.base + emit.pc * INSTR_SIZE;
}

void gen_enter(int var_size)
{
	emit.opcode.oc = PSH;
	emit.opcode.a = BR;
	emit.opcode.b = SP;
	emit.opcode.imm = -WORD_SIZE;
	emit_a_b_imm(&emit);
	emit.opcode.oc = PSH;
	emit.opcode.a = FP;
	emit.opcode.b = SP;
	emit.opcode.imm = -WORD_SIZE;
	emit_a_b_imm(&emit);
	emit.opcode.oc = MOV;
	emit.opcode.a = FP;
	emit.opcode.b = SP;
	emit.opcode.imm = 0;
	emit_a_b_imm(&emit);
	if (var_size) {
		emit.opcode.oc = ADDI;
		emit.opcode.a = SP;
		emit.opcode.b = SP;
		emit.opcode.imm = var_size;
		emit_a_b_imm(&emit);
	}
}

void gen_return(int par_size)
{
	emit.opcode.oc = MOV;
	emit.opcode.a = SP;
	emit.opcode.b = FP;
	emit.opcode.imm = 0;
	emit_a_b_imm(&emit);
	emit.opcode.oc = POP;
	emit.opcode.a = FP;
	emit.opcode.b = SP;
	emit.opcode.imm = WORD_SIZE;
	emit_a_b_imm(&emit);
	emit.opcode.oc = POP;
	emit.opcode.a = PC;
	emit.opcode.b = SP;
	emit.opcode.imm = WORD_SIZE + par_size;
	emit_a_b_imm(&emit);
}

void gen_fix_jumps(int label)
{
	while (label != 0) {
		int label_2 = emit.base[label];
		emit.base[label] = (label_2 & BIT_OC_MASK) + ((emit.pc - 1 - label) << BIT_OC);
		label = label_2 >> BIT_OC;
	}
}

static void fix_jumps_with(int label, int dst)
{
	while (label != 0) {
		int label_2 = emit.base[label];
		emit.base[label] = (label_2 & BIT_OC_MASK) + ((dst - 1 - label) << BIT_OC);
		label = label_2 >> BIT_OC;
	}
}

static void merge_jumps(int64_t *label_1, int label_0)
{
	if (label_0 != 0) {
		int label_3 = label_0;
		int label_2;
		do {
			label_2 = label_3;
			label_3 = emit.base[label_2] >> BIT_OC;
		} while (label_3 != 0);
		emit.base[label_2] += ((*label_1 - label_3) << BIT_OC);
		*label_1 = label_0;
	}
}

static void negate_cond(int *neg)
{
	*neg = *neg % 2 == 0 ? *neg + 1 : *neg - 1;
}

static void load(GEN x)
{
	if (x->mode != MODE_REG) {
		if (x->mode == CLASS_CONST) {
			alloc_reg(&x->reg);
			emit.opcode.oc = MOVI;
			emit.opcode.a = x->reg;
			emit.opcode.b = 0;
			emit.opcode.imm = x->val;
			emit_a_b_imm(&emit);
		} else if (x->mode == CLASS_VAR) {
			int reg;
			alloc_reg(&reg);
			if (x->reg == PC) {
				x->val = x->val - (emit.pc + 1) * INSTR_SIZE;
			}
			if (x->size == 1) {
				emit.opcode.oc = LDB;
				emit.opcode.a = reg;
				emit.opcode.b = x->reg;
				emit.opcode.imm = x->val;
				emit_a_b_imm(&emit);
			} else if (x->size == 2) {
				emit.opcode.oc = LDW;
				emit.opcode.a = reg;
				emit.opcode.b = x->reg;
				emit.opcode.imm = x->val;
				emit_a_b_imm(&emit);
			} else if (x->size == 4) {
				emit.opcode.oc = LDD;
				emit.opcode.a = reg;
				emit.opcode.b = x->reg;
				emit.opcode.imm = x->val;
				emit_a_b_imm(&emit);
			} else { /* (x->size == 8) */
				emit.opcode.oc = LDQ;
				emit.opcode.a = reg;
				emit.opcode.b = x->reg;
				emit.opcode.imm = x->val;
				emit_a_b_imm(&emit);
			}
			free_reg(x->reg);
			x->reg = reg;
		} else if (x->mode == MODE_COND) {
			int cond = x->cond;
			negate_cond(&cond);
			emit.opcode.oc = cond;
			emit.opcode.imm = 2;
			emit_offs(&emit);
			gen_fix_jumps(x->val2);
			emit.opcode.oc = MOVI;
			emit.opcode.a = x->reg;
			emit.opcode.b = 0;
			emit.opcode.imm = 1;
			emit_a_b_imm(&emit);
			emit.opcode.oc = JUMP;
			emit.opcode.imm = 1;
			emit_offs(&emit);
			gen_fix_jumps(x->val);
			emit.opcode.oc = MOVI;
			emit.opcode.a = x->reg;
			emit.opcode.b = 0;
			emit.opcode.imm = 0;
			emit_a_b_imm(&emit);
		}
		x->mode = MODE_REG;
	}
}

void gen_param(GEN x, int is_ref)
{
	if (is_ref) { /* ref param */
		int reg;
		if (x->val == 0) { /* gen_deref */
			reg = x->reg;
		} else {
			if (x->reg == PC) {
				x->val = x->val - (emit.pc + 1) * INSTR_SIZE;
			}
			alloc_reg(&reg);
			emit.opcode.oc = MOV;
			emit.opcode.a = reg;
			emit.opcode.b = x->reg;
			emit.opcode.imm = 0;
			emit_a_b_imm(&emit);
			emit.opcode.oc = ADDI;
			emit.opcode.a = reg;
			emit.opcode.b = reg;
			emit.opcode.imm = x->val;
			emit_a_b_imm(&emit);
		}
		emit.opcode.oc = PSH;
		emit.opcode.a = reg;
		emit.opcode.b = SP;
		emit.opcode.imm = -WORD_SIZE;
		emit_a_b_imm(&emit);
		free_reg(reg);
	} else { /* value param */
		load(x);
		emit.opcode.oc = PSH;
		emit.opcode.a = x->reg;
		emit.opcode.b = SP;
		emit.opcode.imm = -WORD_SIZE;
		emit_a_b_imm(&emit);
	}
	free_reg(x->reg);
}

void gen_deref(GEN x)
{
	while (x->val2 > 0) {
		int reg;
		alloc_reg(&reg);
		emit.opcode.oc = LDQ;
		emit.opcode.a = reg;
		emit.opcode.b = x->reg;
		emit.opcode.imm = x->val;
		emit_a_b_imm(&emit);
		free_reg(x->reg);
		x->reg = reg;
		x->val = 0; /* gen_param */
		x->val2--;
	}
}

static void load_cond(GEN x)
{
	if (x->mode != MODE_COND) {
		load(x);
		emit.opcode.oc = SUBI;
		emit.opcode.a = BR;
		emit.opcode.b = x->reg;
		emit.opcode.imm = 0;
		emit_a_b_imm(&emit);
		free_reg(x->reg);
		x->mode = MODE_COND;
		x->val = 0;
		x->val2 = 0;
		x->cond = BNE;
	}
}

/* x rel y */
static void cond(int oc, GEN x, GEN y)
{
	load(x);
	if (y->mode == CLASS_CONST) {
		emit.opcode.oc = SUBI;
		emit.opcode.a = BR;
		emit.opcode.b = x->reg;
		emit.opcode.imm = y->val;
		emit_a_b_imm(&emit);
	} else {
		load(y);
		emit.opcode.oc = SUB;
		emit.opcode.a = BR;
		emit.opcode.b = x->reg;
		emit.opcode.imm = y->reg;
		emit_a_b_c(&emit);
		free_reg(y->reg);
	}
	free_reg(x->reg);
	x->mode = MODE_COND;
	x->val = 0;
	x->val2 = 0;
	x->cond = oc;
}

/* ASSERT(x) */
void gen_assert(GEN x, int line)
{
	assert(x);

	load_cond(x);
	int label = emit.pc;
	emit.opcode.oc = x->cond;
	emit.opcode.imm = x->val2;
	emit_offs(&emit);
	gen_fix_jumps(x->val);
	x->val2 = label;
	emit.opcode.oc = MOVI;
	emit.opcode.a = BR;
	emit.opcode.b = 0;
	emit.opcode.imm = line;
	emit_a_b_imm(&emit);
	emit.opcode.oc = SYS + 0;
	emit.opcode.a = BR;
	emit.opcode.b = 0;
	emit.opcode.imm = 0;
	emit_a_b_imm(&emit);
	gen_fix_jumps(x->val2);
	free_reg(x->reg);
}

/* SYS(call_nr, x, y) */
void gen_sys(int call_nr, GEN x, GEN y)
{
	emit.opcode.oc = SYS + call_nr;
	emit.opcode.a = x->reg;
	emit.opcode.b = y->reg;
	emit.opcode.imm = 0;
	emit_a_b_imm(&emit);
	free_reg(x->reg);
	free_reg(y->reg);
}

/* x = ASH(x, y) */
void gen_ash(GEN x, GEN y)
{
	if (x->mode == CLASS_CONST && y->mode == CLASS_CONST) {
		if (y->val < 0) {
			x->val >>= -y->val;
		} else {
			x->val <<= y->val;
		}
	} else {
		load(x);
		load(y);
		emit.opcode.oc = ASH;
		emit.opcode.a = x->reg;
		emit.opcode.b = y->reg;
		emit.opcode.imm = 0;
		emit_a_b_imm(&emit);
		free_reg(y->reg);
	}
}

/* x = BITA(x, y) */
void gen_bita(GEN x, GEN y)
{
	if (x->mode == CLASS_CONST && y->mode == CLASS_CONST) {
		x->val &= y->val;
	} else {
		load(x);
		load(y);
		emit.opcode.oc = BITA;
		emit.opcode.a = x->reg;
		emit.opcode.b = y->reg;
		emit.opcode.imm = 0;
		emit_a_b_imm(&emit);
		free_reg(y->reg);
	}
}

/* x = BITO(x, y) */
void gen_bito(GEN x, GEN y)
{
	if (x->mode == CLASS_CONST && y->mode == CLASS_CONST) {
		x->val |= y->val;
	} else {
		load(x);
		load(y);
		emit.opcode.oc = BITO;
		emit.opcode.a = x->reg;
		emit.opcode.b = y->reg;
		emit.opcode.imm = 0;
		emit_a_b_imm(&emit);
		free_reg(y->reg);
	}
}

/* ~x */
void gen_not(GEN x)
{
	assert(x);

	if (x->mode == CLASS_CONST) {
		x->val = x->val ? 0 : 1;
	} else if (x->mode == MODE_COND) {
		negate_cond(&x->cond);
		int t = x->val;
		x->val = x->val2;
		x->val2 = t;
	} else {
		load(x);
		emit.opcode.oc = NOT;
		emit.opcode.a = x->reg;
		emit.opcode.b = x->reg;
		emit.opcode.imm = 0;
		emit_a_b_imm(&emit);
	}
}

/* -x */
void gen_neg(GEN x)
{
	assert(x);

	if (x->mode == CLASS_CONST) {
		x->val = -x->val;
	} else {
		load(x);
		emit.opcode.oc = NEG;
		emit.opcode.a = x->reg;
		emit.opcode.b = x->reg;
		emit.opcode.imm = 0;
		emit_a_b_imm(&emit);
	}
}

/* x = y */
void gen_eq(GEN x, GEN y)
{
	assert(x);
	assert(y);

	if (x->mode == CLASS_CONST && y->mode == CLASS_CONST) {
		x->val = x->val == y->val ? 1 : 0;
	} else {
		cond(BEQ, x, y);
	}
}

/* x # y */
void gen_neq(GEN x, GEN y)
{
	assert(x);
	assert(y);

	if (x->mode == CLASS_CONST && y->mode == CLASS_CONST) {
		x->val = x->val != y->val ? 1 : 0;
	} else {
		cond(BNE, x, y);
	}
}

/* x < y */
void gen_lss(GEN x, GEN y)
{
	assert(x);
	assert(y);

	if (x->mode == CLASS_CONST && y->mode == CLASS_CONST) {
		x->val = x->val < y->val ? 1 : 0;
	} else {
		cond(BLS, x, y);
	}
}

/* x <= y */
void gen_leq(GEN x, GEN y)
{
	assert(x);
	assert(y);

	if (x->mode == CLASS_CONST && y->mode == CLASS_CONST) {
		x->val = x->val <= y->val ? 1 : 0;
	} else {
		cond(BLE, x, y);
	}
}

/* x > y */
void gen_gtr(GEN x, GEN y)
{
	assert(x);
	assert(y);

	if (x->mode == CLASS_CONST && y->mode == CLASS_CONST) {
		x->val = x->val > y->val ? 1 : 0;
	} else {
		cond(BGT, x, y);
	}
}

/* x >= y */
void gen_geq(GEN x, GEN y)
{
	assert(x);
	assert(y);

	if (x->mode == CLASS_CONST && y->mode == CLASS_CONST) {
		x->val = x->val >= y->val ? 1 : 0;
	} else {
		cond(BGE, x, y);
	}
}

/* x OR */
void gen_or(GEN x)
{
	assert(x);

	load_cond(x);
	emit.opcode.oc = x->cond;
	emit.opcode.imm = x->val2;
	emit_offs(&emit);
	x->val2 = emit.pc - 1;
	gen_fix_jumps(x->val);
	x->val = 0;
}

/* x OR y */
void gen_or2(GEN x, GEN y)
{
	assert(x);
	assert(y);

	load_cond(y);
	x->val = y->val;
	merge_jumps(&x->val2, y->val2);
	x->cond = y->cond;
}

/* x & */
void gen_and(GEN x)
{
	assert(x);

	load_cond(x);
	int cond = x->cond;
	negate_cond(&cond);
	emit.opcode.oc = cond;
	emit.opcode.imm = x->val;
	emit_offs(&emit);
	x->val = emit.pc - 1;
	gen_fix_jumps(x->val2);
	x->val2 = 0;
}

/* x & y */
void gen_and2(GEN x, GEN y)
{
	assert(x);
	assert(y);

	load_cond(y);
	merge_jumps(&x->val, y->val);
	x->val2 = y->val2;
	x->cond = y->cond;
}

static void op2(GEN x, GEN y, int op)
{
	load(x);
	if (y->mode == CLASS_CONST) {
		op = (op - MUL) / (BIT_REG_MASK + 1) + MULI;
		emit.opcode.oc = op;
		emit.opcode.a = x->reg;
		emit.opcode.b = x->reg;
		emit.opcode.imm = y->val;
		emit_a_b_imm(&emit);
	} else {
		load(y);
		emit.opcode.oc = op;
		emit.opcode.a = x->reg;
		emit.opcode.b = x->reg;
		emit.opcode.imm = y->reg;
		emit_a_b_c(&emit);
		free_reg(y->reg);
	}
}

/* x * y */
void gen_mul(GEN x, GEN y)
{
	assert(x);
	assert(y);

	if (x->mode == CLASS_CONST && y->mode == CLASS_CONST) {
		x->val *= y->val;
	} else {
		op2(x, y, MUL);
	}
}

/* x DIV y */
void gen_div(GEN x, GEN y)
{
	assert(x);
	assert(y);

	if (x->mode == CLASS_CONST && y->mode == CLASS_CONST && y->val) {
		x->val /= y->val;
	} else {
		op2(x, y, DIV);
	}
}

static int64_t modulo(int64_t x, int64_t y) {
	int64_t m = x % y;
	if (m < 0L) {
		m = (y < 0) ? m - y : m + y;
	}
	return m;
}

/* x MOD y */
void gen_mod(GEN x, GEN y)
{
	assert(x);
	assert(y);

	if (x->mode == CLASS_CONST && y->mode == CLASS_CONST && y->val) {
		x->val = modulo(x->val, y->val);
	} else {
		op2(x, y, MOD);
	}
}

/* x + y */
void gen_add(GEN x, GEN y)
{
	assert(x);
	assert(y);

	if (x->mode == CLASS_CONST && y->mode == CLASS_CONST) {
		x->val += y->val;
	} else {
		op2(x, y, ADD);
	}
}

/* x - y */
void gen_sub(GEN x, GEN y)
{
	assert(x);
	assert(y);

	if (x->mode == CLASS_CONST && y->mode == CLASS_CONST) {
		x->val -= y->val;
	} else {
		op2(x, y, SUB);
	}
}

/* x := y */
void gen_store(GEN x, GEN y)
{
	assert(x);
	assert(y);

	load(y);
	if (x->reg == PC) {
		x->val = x->val - (emit.pc + 1) * INSTR_SIZE;
	}
	if (x->size == 1) {
//printf("gen_store y->reg=%i x->val=%lli\n", y->reg, x->val);
		emit.opcode.oc = STB;
		emit.opcode.a = y->reg;
		emit.opcode.b = x->reg;
		emit.opcode.imm = x->val;
		emit_a_b_imm(&emit);
	} else if (x->size == 2) {
		emit.opcode.oc = STW;
		emit.opcode.a = y->reg;
		emit.opcode.b = x->reg;
		emit.opcode.imm = x->val;
		emit_a_b_imm(&emit);
	} else if (x->size == 4) {
		emit.opcode.oc = STD;
		emit.opcode.a = y->reg;
		emit.opcode.b = x->reg;
		emit.opcode.imm = x->val;
		emit_a_b_imm(&emit);
	} else { /* (x->size == 8) */
		emit.opcode.oc = STQ;
		emit.opcode.a = y->reg;
		emit.opcode.b = x->reg;
		emit.opcode.imm = x->val;
		emit_a_b_imm(&emit);
	}
	free_reg(x->reg);
	free_reg(y->reg);
}

/* x := x[y] */
void gen_index(GEN x, GEN y, int arr_len, int size)
{
	if (y->mode == CLASS_CONST) {
		x->val += y->val * size;
	} else {
		load(y);
		if (size > 1) {
			emit.opcode.oc = MULI;
			emit.opcode.a = y->reg;
			emit.opcode.b = y->reg;
			emit.opcode.imm = size;
			emit_a_b_imm(&emit);
		}
		emit.opcode.oc = ADD;
		emit.opcode.a = y->reg;
		emit.opcode.b = y->reg;
		emit.opcode.imm = x->reg;
		emit_a_b_c(&emit);
		free_reg(x->reg);
		if (x->reg == PC) {
			x->val -= emit.pc * INSTR_SIZE;
		}
		x->reg = y->reg;
	}
}

/* x := x.y */
void gen_field(GEN x, int addr)
{
	x->val += addr;
}

void gen_bcall(int64_t addr)
{
	emit.opcode.oc = CALL;
	emit.opcode.imm = (addr - (int64_t)emit.base) / INSTR_SIZE - emit.pc - 1;
	emit_offs(&emit);
}

void gen_fcall(int *label)
{
	emit.opcode.oc = CALL;
	emit.opcode.imm = *label;
	emit_offs(&emit);
	*label = emit.pc - 1;
}

void gen_bjump(int label)
{
	emit.opcode.oc = JUMP;
	emit.opcode.imm = label / INSTR_SIZE - emit.pc - 1;
	emit_offs(&emit);
}

void gen_fjump(int *label)
{
	emit.opcode.oc = JUMP;
	emit.opcode.imm = *label;
	emit_offs(&emit);
	*label = emit.pc - 1;
}

void gen_cond_bjump(GEN x, int label)
{
	load_cond(x);
	int cond = x->cond;
	negate_cond(&cond);
	emit.opcode.oc = cond;
	emit.opcode.imm = label / INSTR_SIZE - emit.pc - 1;
	emit_offs(&emit);
	gen_fix_jumps(x->val2);
	fix_jumps_with(x->val, label);
}

void gen_cond_fjump(GEN x)
{
	load_cond(x);
	int cond = x->cond;
	negate_cond(&cond);
	emit.opcode.oc = cond;
	emit.opcode.imm = x->val;
	emit_offs(&emit);
	free_reg(x->reg);
	gen_fix_jumps(x->val2);
	x->val = emit.pc - 1;
}
