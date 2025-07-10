/*
 * TINYEXPRX - Tiny recursive descent parser and evaluation engine for
 * complex numbers in C
 *
 * Copyright (c) 2025-2025 Saeid Emami(Chatterbump LLC)
 *
 * This software is based on TINYEXPR by Lewis Van Winkle
 * (http://CodePlea.com).
 *
 */

#ifndef TINYEXPRX_H
#define TINYEXPRX_H

#ifdef __cplusplus
extern "C" {
#endif

typedef double _Complex d_cx;

typedef struct tx_expr {
    int type;
    union {d_cx value; const d_cx *bound; const void *function;};
    void *parameters[1];
} tx_expr;


enum {
    TX_VARIABLE = 0,

    TX_FUNCTION0 = 8, TX_FUNCTION1, TX_FUNCTION2, TX_FUNCTION3,
    TX_FUNCTION4, TX_FUNCTION5, TX_FUNCTION6,

    TX_CLOSURE0 = 16, TX_CLOSURE1, TX_CLOSURE2, TX_CLOSURE3,
    TX_CLOSURE4, TX_CLOSURE5, TX_CLOSURE6,

    TX_FLAG_PURE = 32
};

typedef struct tx_variable {
    const char *name;
    const void *address;
    int type;
    void *context;
} tx_variable;



/* Parses the input expression, evaluates it, and frees it. */
/* Returns NaN on error. */
d_cx tx_interp(const char *expression, int *error);

/* Parses the input expression and binds variables. */
/* Returns NULL on error. */
tx_expr *tx_compile(const char *expression, const tx_variable *variables, int var_count, int *error);

/* Evaluates the expression. */
d_cx tx_eval(const tx_expr *n);

/* Prints debugging information on the syntax tree. */
void tx_print(const tx_expr *n);

/* Prints d_cx number. */
void tx_print_num(const d_cx *n);

/* Frees the expression. */
/* This is safe to call on NULL pointers. */
void tx_free(tx_expr *n);


#ifdef __cplusplus
}
#endif

#endif /*TINYEXPRX_H*/
