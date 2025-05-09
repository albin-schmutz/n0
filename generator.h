/*

Generates bytecode.

*/

typedef struct GEN *GEN;

struct GEN {
	int64_t val;
	int64_t val2;
	int size;
	int cond;
	int reg;
	int mode;
	int type;
	int exp_const;
};

extern void gen_start(int64_t pc);
extern void gen_exit(void);
extern void gen_get_pc_rel(int *pc);
extern void gen_get_pc_abs(int64_t *pc);

extern void gen_enter(int var_size);
extern void gen_return(int par_size);

extern void gen_fix_jumps(int label);

extern void gen_param(GEN x, int is_ref);
extern void gen_deref(GEN x);

extern void gen_assert(GEN x, int line);
extern void gen_sys(int call_nr, GEN x, GEN y);
extern void gen_ash(GEN x, GEN y);
extern void gen_bita(GEN x, GEN y);
extern void gen_bito(GEN x, GEN y);

extern void gen_not(GEN x);
extern void gen_neg(GEN x);

extern void gen_eq(GEN x, GEN y);
extern void gen_neq(GEN x, GEN y);
extern void gen_lss(GEN x, GEN y);
extern void gen_leq(GEN x, GEN y);
extern void gen_gtr(GEN x, GEN y);
extern void gen_geq(GEN x, GEN y);

extern void gen_or(GEN x);
extern void gen_or2(GEN x, GEN y);
extern void gen_and(GEN x);
extern void gen_and2(GEN x, GEN y);

extern void gen_mul(GEN x, GEN y);
extern void gen_div(GEN x, GEN y);
extern void gen_mod(GEN x, GEN y);
extern void gen_add(GEN x, GEN y);
extern void gen_sub(GEN x, GEN y);

extern void gen_store(GEN x, GEN y);
extern void gen_index(GEN x, GEN y, int arr_len, int size);
extern void gen_field(GEN x, int addr);

extern void gen_bcall(int64_t addr);
extern void gen_fcall(int *label);
extern void gen_fjump(int *label);
extern void gen_bjump(int label);
extern void gen_cond_bjump(GEN x, int label);
extern void gen_cond_fjump(GEN x);
