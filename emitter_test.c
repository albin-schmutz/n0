#include <assert.h>
#include <stdint.h>
#include "opcode.h"
#include "emitter.h"

static void test_offs(void)
{
	int prog_space[4];
	struct EMITTER emit;

	emit_init(&emit, (int64_t)prog_space);

	emit.opcode.oc = JUMP;
	emit.opcode.imm = 666;
	emit_offs(&emit);

	emit.opcode.oc = BLS;
	emit.opcode.imm = -4;
	emit_offs(&emit);

	assert(prog_space[0] == 21314);
	assert(prog_space[1] == -122);
}

static void test_a_b_imm(void)
{
	int prog_space[5];
	struct EMITTER emit;

	emit_init(&emit, (int64_t)prog_space);

	emit.opcode.oc = SUBI;
	emit.opcode.a = 1;
	emit.opcode.b = 2;
	emit.opcode.imm = -1000000000;
	emit_a_b_imm(&emit);

	emit.opcode.oc = PSH;
	emit.opcode.a = 2;
	emit.opcode.b = 3;
	emit.opcode.imm = 100;
	emit_a_b_imm(&emit);

	emit.opcode.oc = MOV;
	emit.opcode.a = 5;
	emit.opcode.b = 9;
	emit.opcode.imm = 0;
	emit_a_b_imm(&emit);

	assert(prog_space[0] == 50208);
	assert(*((int64_t*)&prog_space[1]) == -1000000000);
	assert(prog_space[3] == 820814);
	assert(prog_space[4] == 815776);
}

static void test_a_b_c(void)
{
	int prog_space[4];
	struct EMITTER emit;

	emit_init(&emit, (int64_t)prog_space);

	emit.opcode.oc = SUB;
	emit.opcode.a = 1;
	emit.opcode.b = 2;
	emit.opcode.imm = 3;
	emit_a_b_c(&emit);

	assert(prog_space[0] == 665152);
}

int main(void)
{
	test_offs();
	test_a_b_imm();
	test_a_b_c();
	return 0;
}
