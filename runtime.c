#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "opcode.h"
#include "cpu.h"
#include "big_endian.h"
#include "runtime.h"

#define TOTAL_PAGES 4096
#define MEMORY_PAGE_SIZE 4096

#define mem32(a) *((int*)(a))

int64_t mem_start;
int64_t mem_size;

struct HEADER *last_module;

static int64_t free_mem;

void run_init(void)
{
	mem_size = TOTAL_PAGES * MEMORY_PAGE_SIZE; 
	mem_start =  (int64_t)malloc(mem_size);
	if (!mem_start) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	free_mem = mem_start;
	reg[SP] = mem_start + mem_size;
	reg[PC] = reg[SP];
}

int run_align(int addr, int size)
{
	assert(addr >= 0);
	assert(size > 0);

	int remainder = addr % size;
	return remainder ? addr += (size - remainder) : addr;
}

static char *find_modulename(void)
{
	struct HEADER *h = last_module;
	while (h && (void*)h > (void*)reg[PC]) h = h->prev;
	return h->modulename;
}

int64_t run_create_module(char *name, int var_size)
{
	assert(var_size <= 0);

	struct HEADER *header = (void*)free_mem;
	header->prev = last_module;
	header->var_size = var_size;
	header->prog_size = 0;
	last_module = header;
	strcpy(header->modulename, name);
	free_mem = (int64_t)header + sizeof(struct HEADER) - var_size;
	header->prog_addr = free_mem;

	return free_mem;
}

void run_exec_module(int prog_size, int entry_point)
{
	assert(last_module);

	if (last_module->var_size == 0 && prog_size == 0) {
		free_mem = free_mem - sizeof(struct HEADER);
		last_module = last_module->prev;
	} else {
		last_module->prog_size = prog_size;
		if (prog_size) {
			last_module->prog_size = run_align(last_module->prog_size, WORD_SIZE);
			reg[PC] = free_mem;
			free_mem += last_module->prog_size;
		}
		last_module->entry_point = entry_point;
		if (entry_point != -1) {
			cpu_execute(entry_point, find_modulename);
		}
	}
}

static int read_ident(FILE *file, char *ident)
{
	int n = fread(ident, 1, MAX_LEN_IDENTIFIER, file);
	return n == MAX_LEN_IDENTIFIER;
}

static int read_i32(FILE *file, int32_t *i)
{
	unsigned char buf[4];
	int n = fread(buf, 1, 4, file);
	*i = be_to_i32(buf);
	return n == 4;
}

static int read_i64(FILE *file, int64_t *i)
{
	unsigned char b[8];
	int n = fread(b, 1, 8, file);
	*i = be_to_i64(b);
	return n == 8;
}

static int read_prog(FILE *file, struct HEADER *header)
{
	struct OPCODE opc;
	int64_t pc;

	pc = run_create_module(header->modulename, header->var_size);
	int64_t end = pc + header->prog_size;
	while (pc < end) {
		if (!read_i32(file, &opc.instr)) return 0;
		cpu_decode_instr(&opc);
		mem32(pc) = opc.instr;
		pc += INSTR_SIZE;
		if (opc.is64) {
			if (!read_i64(file, (int64_t*)pc)) return 0;
			pc += WORD_SIZE;
		}
	}
	run_exec_module(header->prog_size, header->entry_point);

	return 1;
}

int run_load_file(char *modulename)
{
	assert(modulename);

	char filename[MAX_LEN_IDENTIFIER + 8];
	struct HEADER header;

	strcpy(filename, modulename);
	strcat(filename, ".nx0");
	FILE *file = fopen(filename, "rb");
	if (!file) return 0;

	while (!feof(file) && read_ident(file, header.modulename)) {
		if (!(read_i32(file, &header.var_size) &&
			read_i32(file, &header.prog_size) &&
			read_i32(file, &header.entry_point))) return 0;
		if (!read_prog(file, &header)) return 0;
	}
	return 1;
}
