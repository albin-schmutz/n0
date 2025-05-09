#include <assert.h>
#include <stdint.h>
#include "opcode.h"
#include "emitter.h"

void emit_init(EMITTER emit, int64_t start)
{
	assert(emit);
	assert(start);

	emit->base = (int*)start;
	emit->pc = 0;
}

void emit_offs(EMITTER emit)
{
	assert(emit);
	assert(emit->opcode.oc >= JUMP);
	assert(emit->opcode.oc <= BGT);
	assert(emit->opcode.imm >= -67108864);
	assert(emit->opcode.imm < 67108864);

	emit->opcode.instr = emit->opcode.oc + (emit->opcode.imm << BIT_OC);
	emit->base[emit->pc++] = emit->opcode.instr;
}

void emit_a_b_imm(EMITTER emit)
{
	assert(emit);
	assert(emit->opcode.oc >= POP);
	assert(emit->opcode.a >= 0);
	assert(emit->opcode.a <= BIT_REG_MASK);
	assert(emit->opcode.b >= 0);
	assert(emit->opcode.b <= BIT_REG_MASK);

	if (emit->opcode.oc <= STQ) {
		if (emit->opcode.imm >= -262144 && emit->opcode.imm < 262144) {
			emit->opcode.instr = emit->opcode.oc +
				(emit->opcode.a << BIT_OC) +
				(emit->opcode.b << (BIT_OC + BIT_REG)) +
				(emit->opcode.imm << (BIT_OC + BIT_REG + BIT_REG));
			emit->opcode.is64 = 0;
		} else {
			emit->opcode.instr = (emit->opcode.a << BIT_OC) +
				(emit->opcode.b << (BIT_OC + BIT_REG)) +
				((emit->opcode.oc - 16) << (BIT_OC + BIT_REG + BIT_REG));
			emit->opcode.is64 = 1;
		}
	} else {
		emit->opcode.instr = (emit->opcode.a << BIT_OC) +
			(emit->opcode.b << (BIT_OC + BIT_REG)) +
			((emit->opcode.oc - 32) << (BIT_OC + BIT_REG + BIT_REG));
		emit->opcode.is64 = emit->opcode.oc <= STQ2;
	}

	emit->base[emit->pc++] = emit->opcode.instr;
	if (emit->opcode.is64) {
		*((int64_t*)(&emit->base[emit->pc])) = emit->opcode.imm;
		emit->pc += 2;
	}
}

void emit_a_b_c(EMITTER emit)
{
	assert(emit);
	assert(emit->opcode.oc >= MUL);
	assert(emit->opcode.oc <= SYS);
	assert(emit->opcode.a >= 0);
	assert(emit->opcode.a <= BIT_REG_MASK);
	assert(emit->opcode.b >= 0);
	assert(emit->opcode.b <= BIT_REG_MASK);
	assert(emit->opcode.imm >= 0);
	assert(emit->opcode.imm <= BIT_REG_MASK);

	emit->opcode.oc += emit->opcode.a;
	emit->opcode.a = emit->opcode.b;
	emit->opcode.b = emit->opcode.imm;
	emit->opcode.imm = 0;

	emit_a_b_imm(emit);
}
