# REIL interval analyzer

Small interval analyzer for the simplified REIL dialect used in the assignment.
It takes an interval for `arg0` and calculates the possible interval for `ret`.

Supported instructions: `add`, `sub`, `mul`, `str`, `jmp`, `nop`, `jge`, `jg`,
`jle` and `jl`.

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

The result can be wider than the exact range. This is normal for interval
analysis because it does not keep relations between registers. Arithmetic is
also expected to fit in `int64_t`.
