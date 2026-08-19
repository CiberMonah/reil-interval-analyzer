# REIL interval analyzer

This program reads simplified REIL code and finds the possible values of `ret`
for a given interval of `arg0`.

Supported instructions: `add`, `sub`, `mul`, `str`, `jmp`, `nop`, `jge`, `jg`,
`jle` and `jl`.

## Analysis

The parser reads the REIL file and keeps the original line number of every
instruction. These line numbers are used as jump targets.

The program then builds a control-flow graph (CFG). Every instruction is one
node. A normal instruction continues to the next line, `jmp` goes only to its
target, and a conditional jump has true and false paths.

Programs without loops are processed in topological order. Programs with loops
use a worklist: a node is processed again when its input state changes.

`IN` stores register intervals before an instruction, and `OUT` stores them
after the instruction. When several paths meet, their intervals are joined.
Jump conditions narrow the intervals on the corresponding path.

## Build

```sh
cmake -S . -B build
cmake --build build
```

## Run

```sh
./build/reil-analyzer examples/testcase.reil 0 10
```

Here `0` and `10` are the lower and upper bounds of `arg0`.

Loop example:

```sh
./build/reil-analyzer examples/testcase_loop.reil 0 1
```

To run the tests:

```sh
ctest --test-dir build --output-on-failure
```

Interval analysis may return a wider result than the exact set of values. It
does not remember relations between two uses of the same register. Arithmetic
is expected to fit in `int64_t`; overflow is not modeled. Loops whose intervals
grow forever require widening, which is not implemented yet.
