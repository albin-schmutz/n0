#include <assert.h>
#include <stdint.h>
#include "opcode.h"
#include "emitter.h"
#include "cpu.h"

static int64_t mem_size = 128;
static char mem[128];
static int64_t mem_start = (int64_t)mem;

static void finish(EMITTER emit)
{
	emit->opcode.oc = POP;
	emit->opcode.a = PC;
	emit->opcode.b = SP;
	emit->opcode.imm = 0;
	emit_a_b_imm(emit);
	reg[PC] = mem_start;
	reg[SP] = mem_start + mem_size;
	cpu_execute(0, 0);
}

static void test_minimal(void)
{
	struct EMITTER emit;

	emit_init(&emit, mem_start);
	finish(&emit);
	assert(reg[PC] == 0);
}

static void test_mov(void)
{
	struct EMITTER emit;

	emit_init(&emit, mem_start);

	emit.opcode.oc = MOVI;
	emit.opcode.a = 3;
	emit.opcode.b = 0;
	emit.opcode.imm = 123;
	emit_a_b_imm(&emit);

	emit.opcode.oc = MOV;
	emit.opcode.a = 0;
	emit.opcode.b = 3;
	emit.opcode.imm = 0;
	emit_a_b_imm(&emit);

	finish(&emit);
	assert(reg[0] == 123);
}

static void test_movi(void)
{
	struct EMITTER emit;

	emit_init(&emit, mem_start);

	emit.opcode.oc = MOVI;
	emit.opcode.a = 0;
	emit.opcode.b = 0;
	emit.opcode.imm = 123;
	emit_a_b_imm(&emit);

	emit.opcode.oc = MOVI;
	emit.opcode.a = 1;
	emit.opcode.b = 0;
	emit.opcode.imm = -1048576;
	emit_a_b_imm(&emit);

	finish(&emit);
	assert(reg[0] == 123);
	assert(reg[1] == -1048576);
}

static void test_movi2(void)
{
	struct EMITTER emit;

	emit_init(&emit, mem_start);

	emit.opcode.oc = MOVI;
	emit.opcode.a = 0;
	emit.opcode.b = 0;
	emit.opcode.imm = -1048577;
	emit_a_b_imm(&emit);

	emit.opcode.oc = MOVI;
	emit.opcode.a = 1;
	emit.opcode.b = 0;
	emit.opcode.imm = 1048575;
	emit_a_b_imm(&emit);

	emit.opcode.oc = MOVI;
	emit.opcode.a = 2;
	emit.opcode.b = 0;
	emit.opcode.imm = 0x7fffffffffffffff;
	emit_a_b_imm(&emit);

	emit.opcode.oc = MOVI;
	emit.opcode.a = 3;
	emit.opcode.b = 0;
	emit.opcode.imm = -0x8000000000000000;
	emit_a_b_imm(&emit);

	finish(&emit);
	assert(reg[0] == -1048577);
	assert(reg[1] == 1048575);
	assert(reg[2] == 0x7fffffffffffffff);
	assert(reg[3] == -0x8000000000000000);
}

static void test_loop(void)
{
	struct EMITTER emit;

	emit_init(&emit, mem_start);

	emit.opcode.oc = MOVI;
	emit.opcode.a = 0;
	emit.opcode.b = 0;
	emit.opcode.imm = 5;
	emit_a_b_imm(&emit);

	emit.opcode.oc = SUBI;
	emit.opcode.a = 0;
	emit.opcode.b = 0;
	emit.opcode.imm = 1;
	emit_a_b_imm(&emit);

	emit.opcode.oc = SUBI;
	emit.opcode.a = BR;
	emit.opcode.b = 0;
	emit.opcode.imm = 0;
	emit_a_b_imm(&emit);

	emit.opcode.oc = BNE;
	emit.opcode.imm = -3;
	emit_offs(&emit);

	finish(&emit);
	assert(reg[0] == 0);
}

static void test_stack(void)
{
	struct EMITTER emit;

	emit_init(&emit, mem_start);

	emit.opcode.oc = MOVI;
	emit.opcode.a = 0;
	emit.opcode.b = 0;
	emit.opcode.imm = -9999;
	emit_a_b_imm(&emit);

	emit.opcode.oc = PSH;
	emit.opcode.a = 0;
	emit.opcode.b = SP;
	emit.opcode.imm = -WORD_SIZE;
	emit_a_b_imm(&emit);

	emit.opcode.oc = POP;
	emit.opcode.a = 1;
	emit.opcode.b = SP;
	emit.opcode.imm = WORD_SIZE;
	emit_a_b_imm(&emit);

	finish(&emit);
	assert(reg[1] == -9999);
}

static void test_ld_st(void)
{
	struct EMITTER emit;

	emit_init(&emit, mem_start);

	emit.opcode.oc = MOVI;
	emit.opcode.a = 0;
	emit.opcode.b = 0;
	emit.opcode.imm = -12345;
	emit_a_b_imm(&emit);

	emit.opcode.oc = STQ;
	emit.opcode.a = 0;
	emit.opcode.b = SP;
	emit.opcode.imm = -16;
	emit_a_b_imm(&emit);

	emit.opcode.oc = LDQ;
	emit.opcode.a = 1;
	emit.opcode.b = SP;
	emit.opcode.imm = -16;
	emit_a_b_imm(&emit);

	emit.opcode.oc = MOVI;
	emit.opcode.a = 0;
	emit.opcode.b = 0;
	emit.opcode.imm = 255;
	emit_a_b_imm(&emit);

	emit.opcode.oc = STB;
	emit.opcode.a = 0;
	emit.opcode.b = SP;
	emit.opcode.imm = -17;
	emit_a_b_imm(&emit);

	emit.opcode.oc = LDB;
	emit.opcode.a = 1;
	emit.opcode.b = SP;
	emit.opcode.imm = -17;
	emit_a_b_imm(&emit);

	finish(&emit);
	assert(reg[1] == 255);
}

static void test_calc_i(void)
{
	struct EMITTER emit;

	emit_init(&emit, mem_start);

	emit.opcode.oc = MOVI;
	emit.opcode.a = 0;
	emit.opcode.b = 0;
	emit.opcode.imm = -12345;
	emit_a_b_imm(&emit);

	emit.opcode.oc = ADDI;
	emit.opcode.a = 1;
	emit.opcode.b = 0;
	emit.opcode.imm = 12346;
	emit_a_b_imm(&emit);

	emit.opcode.oc = MOVI;
	emit.opcode.a = 2;
	emit.opcode.b = 0;
	emit.opcode.imm = -0x8000000000000000;
	emit_a_b_imm(&emit);

	emit.opcode.oc = DIVI;
	emit.opcode.a = 3;
	emit.opcode.b = 2;
	emit.opcode.imm = 8;
	emit_a_b_imm(&emit);

	finish(&emit);
	assert(reg[0] == -12345);
	assert(reg[1] == 1);
	assert(reg[2] == -0x8000000000000000);
	assert(reg[3] == -0x1000000000000000);
}

static void test_calc(void)
{
	struct EMITTER emit;

	emit_init(&emit, mem_start);

	emit.opcode.oc = MOVI;
	emit.opcode.a = 2;
	emit.opcode.b = 0;
	emit.opcode.imm = 200;
	emit_a_b_imm(&emit);

	emit.opcode.oc = MOVI;
	emit.opcode.a = 3;
	emit.opcode.b = 0;
	emit.opcode.imm = -4000;
	emit_a_b_imm(&emit);

	emit.opcode.oc = DIV + 1;
	emit.opcode.a = 3;
	emit.opcode.b = 2;
	emit.opcode.imm = 0;
	emit_a_b_imm(&emit);

	finish(&emit);
	assert(reg[1] == -20);
}

int main(void)
{
	test_minimal();
	test_mov();
	test_movi();
	test_movi2();
	test_loop();
	test_stack();
	test_ld_st();
	test_calc_i();
	test_calc();

	return 0;
}
