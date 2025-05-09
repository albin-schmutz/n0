#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "source.h"
#include "opcode.h"
#include "system.h"
#include "cpu.h"
#include "big_endian.h"
#include "runtime.h"
#include "parser.h"

#define mem32(a) *((int*)(a))

static void write_ident(FILE *file, char *ident)
{
	int n = fwrite(ident, 1, MAX_LEN_IDENTIFIER, file);
	assert(n == MAX_LEN_IDENTIFIER);
}

static void write_i32(FILE *file, int32_t i)
{
	unsigned char b[4];
	be_from_i32(b, i);
	int n = fwrite(b, 1, 4, file);
	assert(n == 4);
}

static void write_i64(FILE *file, int64_t i)
{
	unsigned char b[8];
	be_from_i64(b, i);
	int n = fwrite(b, 1, 8, file);
	assert(n == 8);
}

static void write_prog(FILE *file, int64_t pc, int prog_size)
{
	struct OPCODE opc;
	int64_t end = pc + prog_size;
	while (pc < end) {
		opc.instr = mem32(pc);
		write_i32(file, opc.instr);
		cpu_decode_instr(&opc);
		pc += INSTR_SIZE;
		if (opc.is64) {
			write_i64(file, *((int64_t*)pc));
			pc += WORD_SIZE;
		}
	}
}

static void write_modules(FILE *file, struct HEADER *header)
{
	if (header) {
		write_modules(file, header->prev);
		write_ident(file, header->modulename);
		write_i32(file, header->var_size);
		write_i32(file, header->prog_size);
		write_i32(file, header->entry_point);
		write_prog(file, header->prog_addr, header->prog_size);
	}
}

static void write_file(void)
{
	char filename[MAX_LEN_IDENTIFIER + 8];
	if (last_module) {
		strcpy(filename, last_module->modulename);
		strcat(filename, ".nx0");
		FILE *file = fopen(filename, "wb");
		assert(file);
		write_modules(file, last_module);
		fclose(file);
	}
}

int main(int argc, char *argv[])
{
	assert(argc >= 2);

	char *module = argv[1];

	sys_init(argc - 1, &argv[1]);
	run_init();
	if (!run_load_file(module)) {
		parser(module);
		write_file();
	}
	return 0;
}
