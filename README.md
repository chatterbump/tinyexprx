# TinyExprX
An expression parer for complex numbers

TinyExprX is a parser and evaluation engine that supports complex number arithmetic.

TinyExprX supports standard complex number presentations, e.g. 5+3I where I = √-1, and complex number operators and expressions, e.g. e^(I*pi)+1. TinyExprX supports
standard C99 complex library and runtime binding of complex number variables.

This parser is based on TinyExpr (https://github.com/codeplea/tinyexpr) by Lewis Van Winkle. 

A couple of changes are fundamental changes are implemented in TinyExprX compared with TinyExpr:

- TinyExprX understands complex numbers like 3+2I and complex variables can be passed and evaluated at runtime,
- The evaluation result of an expression in TinyExprX is a complex number that can be cast into a C complex type or a C++ std::complex<double> or a double[2].
- A number of expressions are undefined or uncommon in complex arithmetic and removed in TinyExprX, including factorial and combinatorial functions NCR and NPR. log always has natural base and ln is not defined.
  


