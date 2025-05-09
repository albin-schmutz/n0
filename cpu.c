#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "system.h"
#include "opcode.h"
#include "cpu.h"

#define mem8(a) *((unsigned char*)(a))
#define mem16(a) *((int16_t*)(a))
#define mem32(a) *((int32_t*)(a))
#define mem64(a) *((int64_t*)(a))
#define imm64(a) a = mem64(reg[PC])

int64_t reg[16];

void cpu_decode_instr(OPCODE opc)
{
	assert(opc);

	opc->oc = opc->instr & BIT_OC_MASK;
	if (opc->oc == 0) {
		opc->oc = 32 + (opc->instr >> (BIT_OC + BIT_REG + BIT_REG));
		opc->is64 = opc->oc >= MOVI2 && opc->oc <= STQ2;
		opc->a = (opc->instr >> BIT_OC) & BIT_REG_MASK;
		opc->b = (opc->instr >> (BIT_OC + BIT_REG)) & BIT_REG_MASK;
	} else if (opc->oc <= BGT) {
		opc->is64 = 0;
		opc->imm = opc->instr >> BIT_OC;
	} else {
		opc->is64 = 0;
		opc->a = (opc->instr >> BIT_OC) & BIT_REG_MASK;
		opc->b = (opc->instr >> (BIT_OC + BIT_REG)) & BIT_REG_MASK;
		opc->imm = opc->instr >> (BIT_OC + BIT_REG + BIT_REG);
	}
}

void cpu_execute(int entry_point, modulename_provider find_modulename)
{
	assert(entry_point >= 0);

	struct OPCODE opc = { 0, 0, 0, 0, 0, 0};

	reg[PC] += entry_point;

	/* push return address 0 */
	reg[SP] -= WORD_SIZE;
	mem64(reg[SP]) = 0;
	while (reg[PC]) {
		opc.instr = mem32(reg[PC]);
		reg[PC] += INSTR_SIZE;
		cpu_decode_instr(&opc);
		switch (opc.oc) {
		case JUMP:
			reg[PC] += opc.imm * INSTR_SIZE;
			break;
		case CALL:
			reg[BR] = reg[PC];
			reg[PC] += opc.imm * INSTR_SIZE;
			break;
		case BEQ:
			if (reg[BR] == 0) reg[PC] += opc.imm * INSTR_SIZE;
			break;
		case BNE:
			if (reg[BR] != 0) reg[PC] += opc.imm * INSTR_SIZE;
			break;
		case BLS:
			if (reg[BR] < 0) reg[PC] += opc.imm * INSTR_SIZE;
			break;
		case BGE:
			if (reg[BR] >= 0) reg[PC] += opc.imm * INSTR_SIZE;
			break;
		case BLE:
			if (reg[BR] <= 0) reg[PC] += opc.imm * INSTR_SIZE;
			break;
		case BGT:
			if (reg[BR] > 0) reg[PC] += opc.imm * INSTR_SIZE;
			break;
		case POP:
			reg[opc.a] = mem64(reg[opc.b]);
			reg[opc.b] += opc.imm;
			break;
		case PSH:
			reg[opc.b] += opc.imm;
			mem64(reg[opc.b]) = reg[opc.a];
			break;
		case MOVI:
			reg[opc.a] = opc.imm;
			break;
		case MULI:
			reg[opc.a] = reg[opc.b] * opc.imm;
			break;
		case DIVI:
			reg[opc.a] = reg[opc.b] / opc.imm;
			break;
		case MODI:
			reg[opc.a] = reg[opc.b] % opc.imm;
			break;
		case ADDI:
			reg[opc.a] = reg[opc.b] + opc.imm;
			break;
		case SUBI:
			reg[opc.a] = reg[opc.b] - opc.imm;
			break;
		case LDB:
			reg[opc.a] = mem8(reg[opc.b] + opc.imm);
			break;
		case LDW:
			reg[opc.a] = mem16(reg[opc.b] + opc.imm);
			break;
		case LDD:
			reg[opc.a] = mem32(reg[opc.b] + opc.imm);
			break;
		case LDQ:
			reg[opc.a] = mem64(reg[opc.b] + opc.imm);
			break;
		case STB:
			mem8(reg[opc.b] + opc.imm) = reg[opc.a];
			break;
		case STW:
			mem16(reg[opc.b] + opc.imm) = reg[opc.a];
			break;
		case STD:
			mem32(reg[opc.b] + opc.imm) = reg[opc.a];
			break;
		case STQ:
			mem64(reg[opc.b] + opc.imm) = reg[opc.a];
			break;
		case MOVI2:
			imm64(opc.imm);
			reg[opc.a] = opc.imm;
			reg[PC] += WORD_SIZE;
			break;
		case MULI2:
			imm64(opc.imm);
			reg[opc.a] = reg[opc.b] * opc.imm;
			reg[PC] += WORD_SIZE;
			break;
		case DIVI2:
			imm64(opc.imm);
			reg[opc.a] = reg[opc.b] / opc.imm;
			reg[PC] += WORD_SIZE;
			break;
		case MODI2:
			imm64(opc.imm);
			reg[opc.a] = reg[opc.b] % opc.imm;
			reg[PC] += WORD_SIZE;
			break;
		case ADDI2:
			imm64(opc.imm);
			reg[opc.a] = reg[opc.b] + opc.imm;
			reg[PC] += WORD_SIZE;
			break;
		case SUBI2:
			imm64(opc.imm);
			reg[opc.a] = reg[opc.b] - opc.imm;
			reg[PC] += WORD_SIZE;
			break;
		case LDB2:
			imm64(opc.imm);
			reg[opc.a] = mem8(reg[opc.b] + opc.imm);
			reg[PC] += WORD_SIZE;
			break;
		case LDW2:
			imm64(opc.imm);
			reg[opc.a] = mem16(reg[opc.b] + opc.imm);
			reg[PC] += WORD_SIZE;
			break;
		case LDD2:
			imm64(opc.imm);
			reg[opc.a] = mem32(reg[opc.b] + opc.imm);
			reg[PC] += WORD_SIZE;
			break;
		case LDQ2:
			imm64(opc.imm);
			reg[opc.a] = mem64(reg[opc.b] + opc.imm);
			reg[PC] += WORD_SIZE;
			break;
		case STB2:
			imm64(opc.imm);
			mem8(reg[opc.b] + opc.imm) = reg[opc.a];
			reg[PC] += WORD_SIZE;
			break;
		case STW2:
			imm64(opc.imm);
			mem16(reg[opc.b] + opc.imm) = reg[opc.a];
			reg[PC] += WORD_SIZE;
			break;
		case STD2:
			imm64(opc.imm);
			mem32(reg[opc.b] + opc.imm) = reg[opc.a];
			reg[PC] += WORD_SIZE;
			break;
		case STQ2:
			imm64(opc.imm);
			mem64(reg[opc.b] + opc.imm) = reg[opc.a];
			reg[PC] += WORD_SIZE;
			break;
		case MUL + 0:
			reg[0] = reg[opc.a] * reg[opc.b];
			break;
		case MUL + 1:
			reg[1] = reg[opc.a] * reg[opc.b];
			break;
		case MUL + 2:
			reg[2] = reg[opc.a] * reg[opc.b];
			break;
		case MUL + 3:
			reg[3] = reg[opc.a] * reg[opc.b];
			break;
		case MUL + 4:
			reg[4] = reg[opc.a] * reg[opc.b];
			break;
		case MUL + 5:
			reg[5] = reg[opc.a] * reg[opc.b];
			break;
		case MUL + 6:
			reg[6] = reg[opc.a] * reg[opc.b];
			break;
		case MUL + 7:
			reg[7] = reg[opc.a] * reg[opc.b];
			break;
		case MUL + 8:
			reg[8] = reg[opc.a] * reg[opc.b];
			break;
		case MUL + 9:
			reg[9] = reg[opc.a] * reg[opc.b];
			break;
		case MUL + 10:
			reg[10] = reg[opc.a] * reg[opc.b];
			break;
		case MUL + 11:
			reg[11] = reg[opc.a] * reg[opc.b];
			break;
		case MUL + 12:
			reg[12] = reg[opc.a] * reg[opc.b];
			break;
		case MUL + 13:
			reg[13] = reg[opc.a] * reg[opc.b];
			break;
		case MUL + 14:
			reg[14] = reg[opc.a] * reg[opc.b];
			break;
		case MUL + 15:
			reg[15] = reg[opc.a] * reg[opc.b];
			break;
		case DIV + 0:
			reg[0] = reg[opc.a] / reg[opc.b];
			break;
		case DIV + 1:
			reg[1] = reg[opc.a] / reg[opc.b];
			break;
		case DIV + 2:
			reg[2] = reg[opc.a] / reg[opc.b];
			break;
		case DIV + 3:
			reg[3] = reg[opc.a] / reg[opc.b];
			break;
		case DIV + 4:
			reg[4] = reg[opc.a] / reg[opc.b];
			break;
		case DIV + 5:
			reg[5] = reg[opc.a] / reg[opc.b];
			break;
		case DIV + 6:
			reg[6] = reg[opc.a] / reg[opc.b];
			break;
		case DIV + 7:
			reg[7] = reg[opc.a] / reg[opc.b];
			break;
		case DIV + 8:
			reg[8] = reg[opc.a] / reg[opc.b];
			break;
		case DIV + 9:
			reg[9] = reg[opc.a] / reg[opc.b];
			break;
		case DIV + 10:
			reg[10] = reg[opc.a] / reg[opc.b];
			break;
		case DIV + 11:
			reg[11] = reg[opc.a] / reg[opc.b];
			break;
		case DIV + 12:
			reg[12] = reg[opc.a] / reg[opc.b];
			break;
		case DIV + 13:
			reg[13] = reg[opc.a] / reg[opc.b];
			break;
		case DIV + 14:
			reg[14] = reg[opc.a] / reg[opc.b];
			break;
		case DIV + 15:
			reg[15] = reg[opc.a] / reg[opc.b];
			break;
		case MOD + 0:
			reg[0] = reg[opc.a] % reg[opc.b];
			break;
		case MOD + 1:
			reg[1] = reg[opc.a] % reg[opc.b];
			break;
		case MOD + 2:
			reg[2] = reg[opc.a] % reg[opc.b];
			break;
		case MOD + 3:
			reg[3] = reg[opc.a] % reg[opc.b];
			break;
		case MOD + 4:
			reg[4] = reg[opc.a] % reg[opc.b];
			break;
		case MOD + 5:
			reg[5] = reg[opc.a] % reg[opc.b];
			break;
		case MOD + 6:
			reg[6] = reg[opc.a] % reg[opc.b];
			break;
		case MOD + 7:
			reg[7] = reg[opc.a] % reg[opc.b];
			break;
		case MOD + 8:
			reg[8] = reg[opc.a] % reg[opc.b];
			break;
		case MOD + 9:
			reg[9] = reg[opc.a] % reg[opc.b];
			break;
		case MOD + 10:
			reg[10] = reg[opc.a] % reg[opc.b];
			break;
		case MOD + 11:
			reg[11] = reg[opc.a] % reg[opc.b];
			break;
		case MOD + 12:
			reg[12] = reg[opc.a] % reg[opc.b];
			break;
		case MOD + 13:
			reg[13] = reg[opc.a] % reg[opc.b];
			break;
		case MOD + 14:
			reg[14] = reg[opc.a] % reg[opc.b];
			break;
		case MOD + 15:
			reg[15] = reg[opc.a] % reg[opc.b];
			break;
		case ADD + 0:
			reg[0] = reg[opc.a] + reg[opc.b];
			break;
		case ADD + 1:
			reg[1] = reg[opc.a] + reg[opc.b];
			break;
		case ADD + 2:
			reg[2] = reg[opc.a] + reg[opc.b];
			break;
		case ADD + 3:
			reg[3] = reg[opc.a] + reg[opc.b];
			break;
		case ADD + 4:
			reg[4] = reg[opc.a] + reg[opc.b];
			break;
		case ADD + 5:
			reg[5] = reg[opc.a] + reg[opc.b];
			break;
		case ADD + 6:
			reg[6] = reg[opc.a] + reg[opc.b];
			break;
		case ADD + 7:
			reg[7] = reg[opc.a] + reg[opc.b];
			break;
		case ADD + 8:
			reg[8] = reg[opc.a] + reg[opc.b];
			break;
		case ADD + 9:
			reg[9] = reg[opc.a] + reg[opc.b];
			break;
		case ADD + 10:
			reg[10] = reg[opc.a] + reg[opc.b];
			break;
		case ADD + 11:
			reg[11] = reg[opc.a] + reg[opc.b];
			break;
		case ADD + 12:
			reg[12] = reg[opc.a] + reg[opc.b];
			break;
		case ADD + 13:
			reg[13] = reg[opc.a] + reg[opc.b];
			break;
		case ADD + 14:
			reg[14] = reg[opc.a] + reg[opc.b];
			break;
		case ADD + 15:
			reg[15] = reg[opc.a] + reg[opc.b];
			break;
		case SUB + 0:
			reg[0] = reg[opc.a] - reg[opc.b];
			break;
		case SUB + 1:
			reg[1] = reg[opc.a] - reg[opc.b];
			break;
		case SUB + 2:
			reg[2] = reg[opc.a] - reg[opc.b];
			break;
		case SUB + 3:
			reg[3] = reg[opc.a] - reg[opc.b];
			break;
		case SUB + 4:
			reg[4] = reg[opc.a] - reg[opc.b];
			break;
		case SUB + 5:
			reg[5] = reg[opc.a] - reg[opc.b];
			break;
		case SUB + 6:
			reg[6] = reg[opc.a] - reg[opc.b];
			break;
		case SUB + 7:
			reg[7] = reg[opc.a] - reg[opc.b];
			break;
		case SUB + 8:
			reg[8] = reg[opc.a] - reg[opc.b];
			break;
		case SUB + 9:
			reg[9] = reg[opc.a] - reg[opc.b];
			break;
		case SUB + 10:
			reg[10] = reg[opc.a] - reg[opc.b];
			break;
		case SUB + 11:
			reg[11] = reg[opc.a] - reg[opc.b];
			break;
		case SUB + 12:
			reg[12] = reg[opc.a] - reg[opc.b];
			break;
		case SUB + 13:
			reg[13] = reg[opc.a] - reg[opc.b];
			break;
		case SUB + 14:
			reg[14] = reg[opc.a] - reg[opc.b];
			break;
		case SUB + 15:
			reg[15] = reg[opc.a] - reg[opc.b];
			break;
		case MOV:
			reg[opc.a] = reg[opc.b];
			break;
		case NEG:
			reg[opc.a] = -reg[opc.b];
			break;
		case NOT:
			reg[opc.a] = reg[opc.b] ? 0 : 1;
			break;
		case ASH:
			if (reg[opc.b] < 0) {
				reg[opc.a] >>= -reg[opc.b];
			} else {
				reg[opc.a] <<= reg[opc.b];
			}
			break;
		case BITA:
			reg[opc.a] &= reg[opc.b];
			break;
		case BITO:
			reg[opc.a] |= reg[opc.b];
			break;
		case SYS + 0:
			printf("Assertion failed in module %s line %i\n",
				find_modulename(), (int)reg[opc.a]);
			exit(1);
			break;
		case SYS + 1: case SYS + 2: case SYS + 3: case SYS + 4:
		case SYS + 5: case SYS + 6: case SYS + 7: case SYS + 8:
		case SYS + 9: case SYS + 10: case SYS + 11: case SYS + 12:
		case SYS + 13: case SYS + 14: case SYS + 15: case SYS + 16:
			sys_call(opc.oc - SYS, (int64_t*)reg[opc.a], (void*)reg[opc.b]);
			break;
		}
	}
}
