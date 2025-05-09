/*

Executes emitted opcodes.
- Provides 16 64-bit registers.
- Needs a modulename_provider for the ASSERT sys call

*/

#define WORD_SIZE 8
#define INSTR_SIZE 4

extern int64_t reg[16];

typedef char *(*modulename_provider)(void);

extern void cpu_decode_instr(OPCODE opc);
extern void cpu_execute(int entry_point, modulename_provider find_modulename);
