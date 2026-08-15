# REIL interval analyzer

Small interval analyzer for the simplified REIL dialect used in the assignment.
It takes an interval for `arg0` and calculates the possible interval for `ret`.

Supported instructions: `add`, `sub`, `mul`, `str`, `jmp`, `nop`, `jge`, `jg`,
`jle` and `jl`.

## Analysis

Each instruction is a CFG node. Normal instructions fall through, `jmp` has
only its target successor, and conditional jumps have a true target and a false
fallthrough successor. Jump targets are REIL source line addresses, which the
CFG maps to instruction indices.

The first assignment assumes an acyclic CFG. Kahn's algorithm produces a
topological order and reports `CFG contains a cycle` otherwise. In that order,
the analyzer joins refined predecessor `OUT` states to obtain `IN`, then applies
the instruction transfer function to obtain `OUT`. Conditional constraints are
applied to edges, including register/register comparisons.

A register kept at a join must be defined on every reachable incoming edge.
This models definitely-defined registers without adding a separate undefined
element to the interval domain. Reading any other register is an error.

## Build

```sh
cmake -S . -B build
cmake --build build
```

## Run

```sh
./build/reil-analyzer examples/testcase.reil 0 10
```

The last two arguments are the lower and upper bounds of the input interval.

To run the tests:

```sh
ctest --test-dir build --output-on-failure
```

The result can be wider than the exact range. For example, multiplying a
`[-1,1]` temporary by itself treats the two operands independently and produces
`[-1,1]`, even though an exact square is non-negative. This loss of correlation
is normal for the interval domain. Arithmetic is expected to fit in `int64_t`;
overflow is not modeled.
