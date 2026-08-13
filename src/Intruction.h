#pragma once

#include <string>

enum class Opcode {
    Add,
    Sub,
    Mul,
    Str,
    Jge,
    Jmp,
    Nop
};

struct Instruction {
    Opcode opcode;

    std::string arg1;
    std::string arg2;
    std::string result;
};