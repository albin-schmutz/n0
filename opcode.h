/*

This API defines an instruction set architecture.
It supports arithmetic, logical, memory, and control flow operations.
There are 16 64-bit registers, the last 4 are for special use:
- FP (Frame Pointer).
- SP (Stack Pointer).
- BR (Register for conditional branches).
- PC (Program Counter).
Instructions are encoded as 32-bit words.
They support different formats for operands and immediate values:
- Single opcode with immediate value for branch operations.
- Opcode with two registers and immediate value.
- Opcode 0 extended operations, including 64-bit immediate values.
Instruction categories.
Control Flow:
- JUMP, CALL, BEQ, BNE, etc., for branching based on conditions.
Stack Operations:
- POP and PSH
Arithmetic and Logical Operations:
- Immediate (ADDI, SUBI, etc.).
- Register-based (ADD, SUB, MUL, etc.) operations.
- Extended operations for 64-bit immediates (ADDI2, SUBI2, etc.).
Memory Operations:
- Load (LDB, LDW, LDD, etc.).
- Store (STB, STW, STD, etc.).
Bitwise and Special Operations:
- ASH, BITA, BITO, SYS for system calls.

*/

#define FP 12
#define SP 13
#define BR 14
#define PC 15

/* 0 opcode defined for extended operations, see last format */

/*

format oc imm

27                          5
--------------------------- -----
imm                         oc

*/

/* 1 - 11 */

#define JUMP 2 /* r[pc] += (imm * 4) */
#define CALL 3 /* r[br] = r[pc]; jump */

#define BEQ 4 /* if r[br] == 0 r[pc] += (imm * 4) */
#define BNE 5 /* if r[br] != 0 r[pc] += (imm * 4) */
#define BLS 6 /* if r[br] <  0 r[pc] += (imm * 4) */
#define BGE 7 /* if r[br] >= 0 r[pc] += (imm * 4) */
#define BLE 8 /* if r[br] <= 0 r[pc] += (imm * 4) */
#define BGT 9 /* if r[br] >  0 r[pc] += (imm * 4) */

/*

format oc a b imm

19                  4    4    5
------------------- ---- ---- -----
imm                 b    a    oc

 */

/* 12 - 15 */

#define POP 13 /* r[a] = mem64[r[b]]; r[b] += imm */
#define PSH 14 /* mem64[r[b]] = r[a]; r[b] += imm */

/* 16 - 31 */

#define MOVI 16 /* r[a] = imm */

#define MULI 18 /* r[a] = r[b] * imm */
#define DIVI 19 /* r[a] = r[b] / imm */
#define MODI 20 /* r[a] = r[b] % imm */
#define ADDI 21 /* r[a] = r[b] + imm */
#define SUBI 22 /* r[a] = r[b] - imm */

#define LDB 24 /* r[a] = mem08[r[b] + imm] */
#define LDW 25 /* r[a] = mem16[r[b] + imm] */
#define LDD 26 /* r[a] = mem32[r[b] + imm] */
#define LDQ 27 /* r[a] = mem64[r[b] + imm] */
#define STB 28 /* mem08[r[b] + imm] = r[a] */
#define STW 29 /* mem16[r[b] + imm] = r[a] */
#define STD 30 /* mem32[r[b] + imm] = r[a] */
#define STQ 31 /* mem64[r[b] + imm] = r[a] */

/*

format 0 a b op (imm64)

19                  4    4    5
------------------- ---- ---- -----
op                  b    a    0

*/

/* 32 - 47 (op: 0 - 15) */

#define MOVI2 (MOVI + 16) /* r[a] = imm64 */

#define MULI2 (MULI + 16) /* r[a] = r[b] * imm64 */
#define DIVI2 (DIVI + 16) /* r[a] = r[b] / imm64 */
#define MODI2 (MODI + 16) /* r[a] = r[b] % imm64 */
#define ADDI2 (ADDI + 16) /* r[a] = r[b] + imm64 */
#define SUBI2 (SUBI + 16) /* r[a] = r[b] - imm64 */

#define LDB2 (LDB + 16) /* r[a] = mem08[r[b] + imm64] */
#define LDW2 (LDW + 16) /* r[a] = mem16[r[b] + imm64] */
#define LDD2 (LDD + 16) /* r[a] = mem32[r[b] + imm64] */
#define LDQ2 (LDQ + 16) /* r[a] = mem64[r[b] + imm64] */
#define STB2 (STB + 16) /* mem08[r[b] + imm64] = r[a] */
#define STW2 (STW + 16) /* mem16[r[b] + imm64] = r[a] */
#define STD2 (STD + 16) /* mem32[r[b] + imm64] = r[a] */
#define STQ2 (STQ + 16) /* mem64[r[b] + imm64] = r[a] */

/* 48 - 127 (op: 16 - 95) */

#define MUL 48 /* r[0-15] = r[a] * r[b] */
#define DIV 64 /* r[0-15] = r[a] / r[b] */
#define MOD 80 /* r[0-15] = r[a] % r[b] */
#define ADD 96 /* r[0-15] = r[a] + r[b] */
#define SUB 112 /* r[0-15] = r[a] - r[b] */

/* 128 - n (op: 96 - n) */

#define ASH 128 /* r[a] = ASH(r[a], r[b]) */
#define BITA 129 /* r[a] = BITA(r[a], r[b]) */
#define BITO 130 /* r[a] = BITO(r[a], r[b]) */
#define MOV 131 /* r[a] = r[b] */
#define NEG 132 /* r[a] = -r[b] */
#define NOT 133 /* r[a] = ~r[b] */
#define SYS 134 /* sys */

#define BIT_OC 5 /* opcodes uses 5 bits */
#define BIT_OC_MASK 31
#define BIT_REG 4 /* register uses 4 bits */
#define BIT_REG_MASK 15

typedef struct OPCODE *OPCODE;

struct OPCODE {
	int64_t imm;
	int32_t instr;
	int oc;
	int a;
	int b;
	int is64;
};
