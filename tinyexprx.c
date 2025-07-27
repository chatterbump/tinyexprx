/*
 * TINYEXPRX - Tiny recursive descent parser and evaluation engine for
 * complex numbers in C
 *
 * Copyright (c) 2025-2025 Saeid Emami(Chatterbump LLC)
 *
 * This software is based on TINYEXPR by Lewis Van Winkle
 * (http://CodePlea.com).
 * # TinyExprX
An expression parer for complex numbers

TinyExprX is a parser and evaluation engine that supports complex number arithmetic.

TinyExprX supports standard complex number presentations, e.g. 5+3I where I = √-1, and complex number operators and expressions, e.g. e^(I*pi)+1. TinyExprX supports
standard C99 complex library and runtime binding of complex number variables.

This parser is based on TinyExpr (https://github.com/codeplea/tinyexpr) by Lewis Van Winkle.

A couple of changes are fundamental changes are implemented in TinyExprX compared with TinyExpr:

- TinyExprX understands complex numbers like 3+2I and complex variables can be passed and evaluated at runtime,
- The evaluation result of an expression in TinyExprX is a complex number that can be cast into a C complex type or a C++ std::complex<double> or a double[2].
- A number of expressions are undefined or uncommon in complex arithmetic and removed in TinyExprX, including factorial and combinatorial functions NCR and NPR.
  log always has natural base ln is not defined.
 *
 */

// Noe: In his parser, a^b^c = (a^b)^c and -a^b = (-a)^b
// log is natural as it is common in complex analysis

#include "tinyexprx.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <complex.h>
#include <ctype.h>

#ifndef NAN
#define NAN (0.0/0.0)
#endif

#ifndef INFINITY
#define INFINITY (1.0/0.0)
#endif


typedef d_cx (*tx_fun2)(d_cx, d_cx);

enum {
    TOK_NULL = TX_CLOSURE6+1, TOK_ERROR, TOK_END, TOK_SEP,
    TOK_OPEN, TOK_CLOSE, TOK_NUMBER_R, TOK_NUMBER_I, TOK_VARIABLE, TOK_INFIX
};


enum {TX_CONSTANT = 1};


typedef struct state {
    const char *start;
    const char *next;
    int type;
    union {double value; const d_cx *bound; const void *function;};
    void *context;

    const tx_variable *lookup;
    int lookup_len;
} state;


#define TYPE_MASK(TYPE) ((TYPE)&0x0000001F)

#define IS_PURE(TYPE) (((TYPE) & TX_FLAG_PURE) != 0)
#define IS_FUNCTION(TYPE) (((TYPE) & TX_FUNCTION0) != 0)
#define IS_CLOSURE(TYPE) (((TYPE) & TX_CLOSURE0) != 0)
#define ARITY(TYPE) ( ((TYPE) & (TX_FUNCTION0 | TX_CLOSURE0)) ? ((TYPE) & 0x00000007) : 0 )
#define NEW_EXPR(type, ...) new_expr((type), (const tx_expr*[]){__VA_ARGS__})
#define CHECK_NULL(ptr, ...) if ((ptr) == NULL) { __VA_ARGS__; return NULL; }

static tx_expr *new_expr(const int type, const tx_expr *parameters[]) {
    const int arity = ARITY(type);
    const int psize = sizeof(void*) * arity;
    const int size = (sizeof(tx_expr) - sizeof(void*)) + psize + (IS_CLOSURE(type) ? sizeof(void*) : 0);
    tx_expr *ret = malloc(size);
    CHECK_NULL(ret);

    memset(ret, 0, size);
    if (arity && parameters) {
        memcpy(ret->parameters, parameters, psize);
    }
    ret->type = type;
    ret->bound = 0;
    return ret;
}


void tx_free_parameters(tx_expr *n) {
    if (!n) return;
    switch (TYPE_MASK(n->type)) {
    case TX_FUNCTION6: case TX_CLOSURE6: tx_free(n->parameters[5]);     /* Falls through. */
    case TX_FUNCTION5: case TX_CLOSURE5: tx_free(n->parameters[4]);     /* Falls through. */
    case TX_FUNCTION4: case TX_CLOSURE4: tx_free(n->parameters[3]);     /* Falls through. */
    case TX_FUNCTION3: case TX_CLOSURE3: tx_free(n->parameters[2]);     /* Falls through. */
    case TX_FUNCTION2: case TX_CLOSURE2: tx_free(n->parameters[1]);     /* Falls through. */
    case TX_FUNCTION1: case TX_CLOSURE1: tx_free(n->parameters[0]);
    }
}


void tx_free(tx_expr *n) {
    if (!n) return;
    tx_free_parameters(n);
    free(n);
}

// all functions need to return complex
static d_cx i(void) {return I;}
static d_cx pi(void) {return 3.14159265358979323846;}
static d_cx e(void) {return 2.71828182845904523536;}
static d_cx infinity() {return INFINITY;}

static d_cx _cabs(d_cx a) {return cabs(a);}
static d_cx _carg(d_cx a) {return carg(a);}
static d_cx _cimag(d_cx a) {return cimag(a);}
static d_cx _creal(d_cx a) {return creal(a);}


static const tx_variable functions[] = {
    /* must be in alphabetical order */
    {"I", i,            TX_FUNCTION0 | TX_FLAG_PURE, 0},
    {"abs", _cabs,      TX_FUNCTION1 | TX_FLAG_PURE, 0},
    {"acos", cacos,     TX_FUNCTION1 | TX_FLAG_PURE, 0},
    {"acosh", cacosh,   TX_FUNCTION1 | TX_FLAG_PURE, 0},
    {"arg", _carg,      TX_FUNCTION1 | TX_FLAG_PURE, 0},
    {"asin", casin,     TX_FUNCTION1 | TX_FLAG_PURE, 0},
    {"asinh", casinh,   TX_FUNCTION1 | TX_FLAG_PURE, 0},
    {"atan", catan,     TX_FUNCTION1 | TX_FLAG_PURE, 0},
    {"atanh", catanh,   TX_FUNCTION1 | TX_FLAG_PURE, 0},
    {"conj", conj,      TX_FUNCTION1 | TX_FLAG_PURE, 0},
    {"cos", ccos,       TX_FUNCTION1 | TX_FLAG_PURE, 0},
    {"cosh", ccosh,     TX_FUNCTION1 | TX_FLAG_PURE, 0},
    {"e", e,            TX_FUNCTION0 | TX_FLAG_PURE, 0},
    {"exp", cexp,       TX_FUNCTION1 | TX_FLAG_PURE, 0},
    {"imag", _cimag,    TX_FUNCTION1 | TX_FLAG_PURE, 0},
    {"inf", infinity,   TX_FUNCTION0 | TX_FLAG_PURE, 0},
    {"log", clog,       TX_FUNCTION1 | TX_FLAG_PURE, 0},
    {"pi", pi,          TX_FUNCTION0 | TX_FLAG_PURE, 0},
    {"pow", cpow,       TX_FUNCTION2 | TX_FLAG_PURE, 0},
    {"real", _creal,    TX_FUNCTION1 | TX_FLAG_PURE, 0},
    {"sin", csin,       TX_FUNCTION1 | TX_FLAG_PURE, 0},
    {"sinh", csinh,     TX_FUNCTION1 | TX_FLAG_PURE, 0},
    {"sqrt", csqrt,     TX_FUNCTION1 | TX_FLAG_PURE, 0},
    {"tan", ctan,       TX_FUNCTION1 | TX_FLAG_PURE, 0},
    {"tanh", ctanh,     TX_FUNCTION1 | TX_FLAG_PURE, 0},
    {0, 0, 0, 0}
};


static const tx_variable *find_builtin(const char *name, int len) {
    int imin = 0;
    int imax = sizeof(functions) / sizeof(tx_variable) - 2;

    /*Binary search.*/
    while (imax >= imin) {
        const int i = (imin + ((imax-imin)/2));
        int c = strncmp(name, functions[i].name, len);
        if (!c) c = '\0' - functions[i].name[len];
        if (c == 0) {
            return functions + i;
        } else if (c > 0) {
            imin = i + 1;
        } else {
            imax = i - 1;
        }
    }
    return 0;
}


static const tx_variable *find_lookup(const state *s, const char *name, int len) {
    int iters;
    const tx_variable *var;
    if (!s->lookup) return 0;

    for (var = s->lookup, iters = s->lookup_len; iters; ++var, --iters) {
        if (strncmp(name, var->name, len) == 0 && var->name[len] == '\0') {
            return var;
        }
    }
    return 0;
}


static d_cx add(d_cx a, d_cx b) {return a+b;}
static d_cx sub(d_cx a, d_cx b) {return a-b;}
static d_cx mul(d_cx a, d_cx b) {return a*b;}
static d_cx divide(d_cx a, d_cx b) {return a/b;}
static d_cx negate(d_cx a) {return -a;}
static d_cx comma(d_cx a, d_cx b) {(void)a; return b;}


void tx_next_token(state *s) {
    s->type = TOK_NULL;

    do {

        if (!*s->next){
            s->type = TOK_END;
            return;
        }

        /* Try reading a number. */
        if ((s->next[0] >= '0' && s->next[0] <= '9') || s->next[0] == '.') {
            s->value = strtod(s->next, (char**)&s->next);
            if (s->next[0] == 'I') {
                s->type = TOK_NUMBER_I;
                s->next++;
            }
            else
                s->type = TOK_NUMBER_R;
        } else {
            /* Look for a variable or builtin function call. */
            if (isalpha(s->next[0])) {
                const char *start;
                start = s->next;
                while (isalpha(s->next[0]) || isdigit(s->next[0]) || (s->next[0] == '_')) s->next++;
                
                const tx_variable *var = find_lookup(s, start, s->next - start);
                if (!var) var = find_builtin(start, s->next - start);

                if (!var) {
                    s->type = TOK_ERROR;
                } else {
                    switch(TYPE_MASK(var->type))
                    {
                    case TX_VARIABLE:
                        s->type = TOK_VARIABLE;
                        s->bound = var->address;
                        break;

                    case TX_CLOSURE0: case TX_CLOSURE1: case TX_CLOSURE2: case TX_CLOSURE3:         /* Falls through. */
                    case TX_CLOSURE4: case TX_CLOSURE5: case TX_CLOSURE6:                           /* Falls through. */
                        s->context = var->context;                                                  /* Falls through. */

                    case TX_FUNCTION0: case TX_FUNCTION1: case TX_FUNCTION2: case TX_FUNCTION3:     /* Falls through. */
                    case TX_FUNCTION4: case TX_FUNCTION5: case TX_FUNCTION6:                        /* Falls through. */
                        s->type = var->type;
                        s->function = var->address;
                        break;
                    }
                }

            } else {
                /* Look for an operator or special character. */
                switch (s->next++[0]) {
                case '+': s->type = TOK_INFIX; s->function = add; break;
                case '-': s->type = TOK_INFIX; s->function = sub; break;
                case '*': s->type = TOK_INFIX; s->function = mul; break;
                case '/': s->type = TOK_INFIX; s->function = divide; break;
                case '^': s->type = TOK_INFIX; s->function = cpow; break;
                case '(': s->type = TOK_OPEN; break;
                case ')': s->type = TOK_CLOSE; break;
                case ',': s->type = TOK_SEP; break;
                case ' ': case '\t': case '\n': case '\r': break;
                default: s->type = TOK_ERROR; break;
                }
            }
        }
    } while (s->type == TOK_NULL);
}


static tx_expr *list(state *s);
static tx_expr *expr(state *s);
static tx_expr *power(state *s);

static tx_expr *base(state *s) {
    /* <base>      =    <constant> | <variable> | <function-0> {"(" ")"} | <function-1> <power> | <function-X> "(" <expr> {"," <expr>} ")" | "(" <list> ")" */
    tx_expr *ret;
    int arity;

    switch (TYPE_MASK(s->type)) {
    case TOK_NUMBER_R:
        ret = new_expr(TX_CONSTANT, 0);
        CHECK_NULL(ret);

        ret->value = s->value;
        tx_next_token(s);
        break;

    case TOK_NUMBER_I:
        ret = new_expr(TX_CONSTANT, 0);
        CHECK_NULL(ret);

        ret->value = s->value*I;
        tx_next_token(s);
        break;

    case TOK_VARIABLE:
        ret = new_expr(TX_VARIABLE, 0);
        CHECK_NULL(ret);

        ret->bound = s->bound;
        tx_next_token(s);
        break;

    case TX_FUNCTION0:
    case TX_CLOSURE0:
        ret = new_expr(s->type, 0);
        CHECK_NULL(ret);

        ret->function = s->function;
        if (IS_CLOSURE(s->type)) ret->parameters[0] = s->context;
        tx_next_token(s);
        if (s->type == TOK_OPEN) {
            tx_next_token(s);
            if (s->type != TOK_CLOSE) {
                s->type = TOK_ERROR;
            } else {
                tx_next_token(s);
            }
        }
        break;

    case TX_FUNCTION1:
    case TX_CLOSURE1:
        ret = new_expr(s->type, 0);
        CHECK_NULL(ret);

        ret->function = s->function;
        if (IS_CLOSURE(s->type)) ret->parameters[1] = s->context;
        tx_next_token(s);
        ret->parameters[0] = power(s);
        CHECK_NULL(ret->parameters[0], tx_free(ret));
        break;

    case TX_FUNCTION2: case TX_FUNCTION3: case TX_FUNCTION4:
    case TX_FUNCTION5: case TX_FUNCTION6:
    case TX_CLOSURE2: case TX_CLOSURE3: case TX_CLOSURE4:
    case TX_CLOSURE5: case TX_CLOSURE6:
        arity = ARITY(s->type);

        ret = new_expr(s->type, 0);
        CHECK_NULL(ret);

        ret->function = s->function;
        if (IS_CLOSURE(s->type)) ret->parameters[arity] = s->context;
        tx_next_token(s);

        if (s->type != TOK_OPEN) {
            s->type = TOK_ERROR;
        } else {
            int i;
            for(i = 0; i < arity; i++) {
                tx_next_token(s);
                ret->parameters[i] = expr(s);
                CHECK_NULL(ret->parameters[i], tx_free(ret));

                if(s->type != TOK_SEP) {
                    break;
                }
            }
            if(s->type != TOK_CLOSE || i != arity - 1) {
                s->type = TOK_ERROR;
            } else {
                tx_next_token(s);
            }
        }

        break;

    case TOK_OPEN:
        tx_next_token(s);
        ret = list(s);
        CHECK_NULL(ret);

        if (s->type != TOK_CLOSE) {
            s->type = TOK_ERROR;
        } else {
            tx_next_token(s);
        }
        break;

    default:
        ret = new_expr(0, 0);
        CHECK_NULL(ret);

        s->type = TOK_ERROR;
        ret->value = NAN;
        break;
    }

    return ret;
}


static tx_expr *power(state *s) {
    /* <power>     =    {("-" | "+")} <base> */
    int sign = 1;
    while (s->type == TOK_INFIX && (s->function == add || s->function == sub)) {
        if (s->function == sub) sign = -sign;
        tx_next_token(s);
    }

    tx_expr *ret;

    if (sign == 1) {
        ret = base(s);
    } else {
        tx_expr *b = base(s);
        CHECK_NULL(b);

        ret = NEW_EXPR(TX_FUNCTION1 | TX_FLAG_PURE, b);
        CHECK_NULL(ret, tx_free(b));

        ret->function = negate;
    }

    return ret;
}


static tx_expr *factor(state *s) {
    /* <factor>    =    <power> {"^" <power>} */
    tx_expr *ret = power(s);
    CHECK_NULL(ret);

    while (s->type == TOK_INFIX && (s->function == cpow)) {
        tx_fun2 t = s->function;
        tx_next_token(s);
        tx_expr *p = power(s);
        CHECK_NULL(p, tx_free(ret));

        tx_expr *prev = ret;
        ret = NEW_EXPR(TX_FUNCTION2 | TX_FLAG_PURE, ret, p);
        CHECK_NULL(ret, tx_free(p), tx_free(prev));

        ret->function = t;
    }

    return ret;
}


static tx_expr *term(state *s) {
    /* <term>      =    <factor> {("*" | "/") <factor>} */
    tx_expr *ret = factor(s);
    CHECK_NULL(ret);

    while (s->type == TOK_INFIX && (s->function == mul || s->function == divide)) {
        tx_fun2 t = s->function;
        tx_next_token(s);
        tx_expr *f = factor(s);
        CHECK_NULL(f, tx_free(ret));

        tx_expr *prev = ret;
        ret = NEW_EXPR(TX_FUNCTION2 | TX_FLAG_PURE, ret, f);
        CHECK_NULL(ret, tx_free(f), tx_free(prev));

        ret->function = t;
    }

    return ret;
}


static tx_expr *expr(state *s) {
    /* <expr>      =    <term> {("+" | "-") <term>} */
    tx_expr *ret = term(s);
    CHECK_NULL(ret);

    while (s->type == TOK_INFIX && (s->function == add || s->function == sub)) {
        tx_fun2 t = s->function;
        tx_next_token(s);
        tx_expr *te = term(s);
        CHECK_NULL(te, tx_free(ret));

        tx_expr *prev = ret;
        ret = NEW_EXPR(TX_FUNCTION2 | TX_FLAG_PURE, ret, te);
        CHECK_NULL(ret, tx_free(te), tx_free(prev));

        ret->function = t;
    }

    return ret;
}


static tx_expr *list(state *s) {
    /* <list>      =    <expr> {"," <expr>} */
    tx_expr *ret = expr(s);
    CHECK_NULL(ret);

    while (s->type == TOK_SEP) {
        tx_next_token(s);
        tx_expr *e = expr(s);
        CHECK_NULL(e, tx_free(ret));

        tx_expr *prev = ret;
        ret = NEW_EXPR(TX_FUNCTION2 | TX_FLAG_PURE, ret, e);
        CHECK_NULL(ret, tx_free(e), tx_free(prev));

        ret->function = comma;
    }

    return ret;
}


#define TX_FUN(...) ((d_cx(*)(__VA_ARGS__))n->function)
#define M(e) tx_eval(n->parameters[e])


d_cx tx_eval(const tx_expr *n) {
    if (!n) return NAN;

    switch(TYPE_MASK(n->type)) {
    case TX_CONSTANT: return n->value;
    case TX_VARIABLE: return *n->bound;

    case TX_FUNCTION0: case TX_FUNCTION1: case TX_FUNCTION2: case TX_FUNCTION3:
    case TX_FUNCTION4: case TX_FUNCTION5: case TX_FUNCTION6:
        switch(ARITY(n->type)) {
        case 0: return TX_FUN(void)();
        case 1: return TX_FUN(d_cx)(M(0));
        case 2: return TX_FUN(d_cx, d_cx)(M(0), M(1));
        case 3: return TX_FUN(d_cx, d_cx, d_cx)(M(0), M(1), M(2));
        case 4: return TX_FUN(d_cx, d_cx, d_cx, d_cx)(M(0), M(1), M(2), M(3));
        case 5: return TX_FUN(d_cx, d_cx, d_cx, d_cx, d_cx)(M(0), M(1), M(2), M(3), M(4));
        case 6: return TX_FUN(d_cx, d_cx, d_cx, d_cx, d_cx, d_cx)(M(0), M(1), M(2), M(3), M(4), M(5));
        default: return NAN;
        }

    case TX_CLOSURE0: case TX_CLOSURE1: case TX_CLOSURE2: case TX_CLOSURE3:
    case TX_CLOSURE4: case TX_CLOSURE5: case TX_CLOSURE6:
        switch(ARITY(n->type)) {
        case 0: return TX_FUN(void*)(n->parameters[0]);
        case 1: return TX_FUN(void*, d_cx)(n->parameters[1], M(0));
        case 2: return TX_FUN(void*, d_cx, d_cx)(n->parameters[2], M(0), M(1));
        case 3: return TX_FUN(void*, d_cx, d_cx, d_cx)(n->parameters[3], M(0), M(1), M(2));
        case 4: return TX_FUN(void*, d_cx, d_cx, d_cx, d_cx)(n->parameters[4], M(0), M(1), M(2), M(3));
        case 5: return TX_FUN(void*, d_cx, d_cx, d_cx, d_cx, d_cx)(n->parameters[5], M(0), M(1), M(2), M(3), M(4));
        case 6: return TX_FUN(void*, d_cx, d_cx, d_cx, d_cx, d_cx, d_cx)(n->parameters[6], M(0), M(1), M(2), M(3), M(4), M(5));
        default: return NAN;
        }

    default: return NAN;
    }
}

#undef TX_FUN
#undef M

static void optimize(tx_expr *n) {
    /* Evaluates as much as possible. */
    if (n->type == TX_CONSTANT) return;
    if (n->type == TX_VARIABLE) return;

    /* Only optimize out functions flagged as pure. */
    if (IS_PURE(n->type)) {
        const int arity = ARITY(n->type);
        int known = 1;
        int i;
        for (i = 0; i < arity; ++i) {
            optimize(n->parameters[i]);
            if (((tx_expr*)(n->parameters[i]))->type != TX_CONSTANT) {
                known = 0;
            }
        }
        if (known) {
            const d_cx value = tx_eval(n);
            tx_free_parameters(n);
            n->type = TX_CONSTANT;
            n->value = value;
        }
    }
}


tx_expr *tx_compile(const char *expression, const tx_variable *variables, int var_count, int *error) {
    state s;
    s.start = s.next = expression;
    s.lookup = variables;
    s.lookup_len = var_count;

    tx_next_token(&s);
    tx_expr *root = list(&s);
    if (root == NULL) {
        if (error) *error = -1;
        return NULL;
    }

    if (s.type != TOK_END) {
        tx_free(root);
        if (error) {
            *error = (s.next - s.start);
            if (*error == 0) *error = 1;
        }
        return 0;
    } else {
        optimize(root);
        if (error) *error = 0;
        return root;
    }
}


d_cx tx_interp(const char *expression, int *error) {
    tx_expr *n = tx_compile(expression, 0, 0, error);

    d_cx ret;
    if (n) {
        ret = tx_eval(n);
        tx_free(n);
    } else {
        ret = NAN;
    }
    return ret;
}


static void pn (const tx_expr *n, int depth) {
    int i, arity;
    printf("%*s", depth, "");

    switch(TYPE_MASK(n->type)) {
    case TX_CONSTANT: tx_print_num(&n->value); break;
    case TX_VARIABLE: printf("bound %p\n", n->bound); break;

    case TX_FUNCTION0: case TX_FUNCTION1: case TX_FUNCTION2: case TX_FUNCTION3:
    case TX_FUNCTION4: case TX_FUNCTION5: case TX_FUNCTION6:
    case TX_CLOSURE0: case TX_CLOSURE1: case TX_CLOSURE2: case TX_CLOSURE3:
    case TX_CLOSURE4: case TX_CLOSURE5: case TX_CLOSURE6:
        arity = ARITY(n->type);
        printf("f%d", arity);
        for(i = 0; i < arity; i++) {
            printf(" %p", n->parameters[i]);
        }
        printf("\n");
        for(i = 0; i < arity; i++) {
            pn(n->parameters[i], depth + 1);
        }
        break;
    }
}


void tx_print(const tx_expr *n) {
    pn(n, 0);
}


void tx_print_num(const d_cx* n) {
    if (cimag(*n) == 0)
        printf("%f\n", creal(*n));
    else
        printf("%f%s%fI\n", creal(*n),(cimag(*n)>0.0)?"+":"", cimag(*n));
}
