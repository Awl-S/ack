/* $Header$ */

typedef char byte;
typedef char bool;
typedef struct line line_t;
typedef struct line *line_p;
typedef struct sym sym_t;
typedef struct sym *sym_p;
typedef struct num num_t;
typedef struct num *num_p;
typedef struct arg arg_t;
typedef struct arg *arg_p;
typedef struct argbytes argb_t;
typedef struct argbytes *argb_p;
typedef struct regs reg_t;
typedef struct regs *reg_p;
#ifdef LONGOFF
typedef long offset;
#else
typedef short offset;
#endif
