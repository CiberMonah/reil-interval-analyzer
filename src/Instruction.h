#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>

struct Register {
    std::string name;
};

using Operand = std::variant<Register, int64_t>;

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
    std::size_t address;
    Opcode opcode;

    std::optional<Operand> arg1;
    std::optional<Operand> arg2;
    std::optional<Operand> result;
};