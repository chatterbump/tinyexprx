# TinyExprX
An expression parer for complex numbers

TinyExprX is a parser and evaluation engine that supports complex number arithmetic.

TinyExprX supports complex numbers, e.g. 5+3I where I = √-1, and complex number operators and expressions, e.g. e^(I*pi)+1. TinyExprX supports
standard C99 complex library and runtime binding of complex number variables.

This parser is based on TinyExpr (https://github.com/codeplea/tinyexpr) by Lewis Van Winkle. 

A couple of changes are fundamental changes are implemented in TinyExprX compared with TinyExpr:

- TinyExprX understands complex numbers like 3+2I and complex variables can be passed and evaluated at runtime,
- The evaluation result of an expression in TinyExprX is a complex number that can be cast into a C complex type or a C++ std::complex<double> or a double[2].
- A number of expressions are undefined or uncommon in complex arithmetic and removed in TinyExprX, including factorial and combinatorial functions NCR and NPR. log always has natural base and ln is not defined.
  
## Features

- **C99 with no dependencies**.
- Single source file and header file.
- Simple and fast.
- Implements standard operators precedence.
- Exposes standard C complex library functions (sin, sqrt, log, etc.).
- Can add custom functions and variables easily.
- Can bind variables at eval-time.
- Released under the MIT license - free for nearly any use.
- Easy to use and integrate with your code.
- Thread-safe, provided that your *malloc* is.

## Building

TinyExprX is self-contained in two files: `tinyexprx.c` and `tinyexprx.h`. To use
TinyExprX, simply add those two files to your project.

## Short Example

Here is a minimal example to evaluate a complex value expression at runtime.

```C
    _Complex double r = tx_interp("(1+2I)*(1-3I)", 0);
    printf("%f+%fI\n", creal(r), cimag(r)); /* 7.000000+-1.000000I */
    tx_print_num(&r); /* Prints 7.000000-1.000000I */
```


