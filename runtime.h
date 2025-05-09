/*

Loads, saves and executes bytecode modules.
- Allocates memory for programcode and stack. 

Memory layout:

h = header
v = static variables
p = programcode
last_module = last module pointer

                                                    reg[SP]
                                                     |
                                                <--- |
                                                     |
|  Module 1          |  Module 2          | ...     Stack
------------------------------------------- ... -----------
|  h1  |  v1  |  p1  |  h2  |  v2  |  p2  | ...
                     |
                    last_module

Header
	prev : Address previous Module

Size
	h : fixed sized
	v : known at end of compilation
	p : known at end of compilation

*/

#define MAX_LEN_IDENTIFIER 32

struct HEADER {
	char modulename[MAX_LEN_IDENTIFIER];
	struct HEADER *prev;
	int64_t prog_addr;
	int var_size;
	int prog_size;
	int entry_point;
	int dummy;
};

extern int64_t mem_start;
extern int64_t mem_size;
extern struct HEADER *last_module;

extern void run_init(void);
extern int run_align(int addr, int size);
extern int64_t run_create_module(char *name, int var_size);
extern void run_exec_module(int prog_size, int entry_point);
extern int run_load_file(char *modulename);
