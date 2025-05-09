#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "opcode.h"
#include "emitter.h"
#include "cpu.h"
#include "runtime.h"

static void test_align(void)
{
	assert(run_align(3, 8) == 8);
	assert(run_align(100, 8) == 104);
	assert(run_align(105, 8) == 112);

	assert(run_align(99, 4) == 100);
	assert(run_align(100, 4) == 100);
	assert(run_align(101, 4) == 104);

	assert(run_align(0, 1) == 0);
	assert(run_align(0, 2) == 0);
	assert(run_align(1, 2) == 2);
	assert(run_align(2, 2) == 2);
}

static void test_create_modules(void)
{
	int64_t prog_addr;
	struct EMITTER emit;
	int header_size = sizeof(struct HEADER);

	prog_addr = run_create_module("tmpM1", -16);
	emit_init(&emit, prog_addr);

	emit.opcode.oc = MOVI;
	emit.opcode.a = 0;
	emit.opcode.b = 0;
	emit.opcode.imm = 42;
	emit_a_b_imm(&emit);

	emit.opcode.oc = MOV;
	emit.opcode.a = PC;
	emit.opcode.b = BR;
	emit.opcode.imm = 0;
	emit_a_b_imm(&emit);

	run_exec_module(8, -1);

	prog_addr = run_create_module("tmpM2", 0);
	emit_init(&emit, prog_addr);

	emit.opcode.oc = CALL;
	emit.opcode.imm = header_size / -INSTR_SIZE - 3;
	emit_offs(&emit);

	emit.opcode.oc = POP;
	emit.opcode.a = PC;
	emit.opcode.b = SP;
	emit.opcode.imm = 0;
	emit_a_b_imm(&emit);

	run_exec_module(8, 0);

	assert(reg[0] == 42);
}

static void test_load_modules(void)
{
	unsigned char bytes[] = {
		0x74, 0x6D, 0x70, 0x4D, 0x31, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x08,
		0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x05, 0x40, 0x10,
		0x00, 0x0C, 0x7D, 0xE0, 0x74, 0x6D, 0x70, 0x4D,
		0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
		0xFF, 0xFF, 0xFD, 0xA3, 0x00, 0x00, 0x1B, 0xED
    };

	FILE *file = fopen("tmpM2.nx0", "wb");
	assert(file);
    int n = fwrite(bytes, 1, sizeof(bytes), file);
	assert(n == sizeof(bytes));
	fclose(file);

	reg[0] = 0;
	assert(run_load_file("tmpM2"));
	assert(reg[0] == 42);
}

int main(void)
{
	test_align();
	run_init();
	test_create_modules();
	run_init();
	test_load_modules();

	return 0;
}
