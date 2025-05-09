/*

Implements symbol tables for types, symbols and modules.

*/

#define MAX_TYPES 50
#define MAX_SYMBOLS 200
#define MAX_MODULES 50

#define FORM_BOOL 1
#define FORM_CHAR 2
#define FORM_INT 3
#define FORM_ARRAY 10
#define FORM_RECORD 11

typedef struct TYPE *TYPE;

struct TYPE {
	int size;
	int length; /* array length */
	int form;
	int owner; /* symbol type */
	int base; /* array base (TYPE) or record fields (SYMBOL) */
};

typedef struct SYMBOL *SYMBOL;

struct SYMBOL {
	char name[32];
	int64_t val;
	int val2;
	int val3;
	int next;
	int clazz;
	int scope; /*
		CLASS_SCOPE: parent scope
		CLASS_IMPORT: imported module scope
		CLASS_PROC: procedure scope
		other: owner module */
	int type;
	int level;
	int is_public;
};

typedef struct MODULE *MODULE;

struct MODULE {
	int64_t prog_addr;
	int scope;
	int loaded;
};

extern int tbl_level;

extern int tbl_boolean_type;
extern int tbl_char_type;
extern int tbl_short_int_type;
extern int tbl_integer_type;
extern int tbl_long_int_type;

extern struct ALLOC_FREE tbl_af_types;
extern struct TYPE tbl_types[MAX_TYPES];

extern struct ALLOC_FREE tbl_af_symbols;
extern struct SYMBOL tbl_symbols[MAX_SYMBOLS];

extern int tbl_marker;
extern int tbl_top_scope;

extern struct MODULE tbl_modules[MAX_MODULES];

extern void tbl_init(void);
extern TYPE tbl_create_type(int *t, int form, int size);
extern void tbl_set_marker(char *id);
extern void tbl_find_symbol(int *s);
extern void tbl_open_scope();
extern void tbl_dealloc_scope(int scope, int private_only);
extern void tbl_close_scope(void);
extern SYMBOL tbl_def_symbol(int *s, int clazz, int m);
extern SYMBOL tbl_use_symbol(int *s, int imp);
extern int tbl_open_module(char *name);
