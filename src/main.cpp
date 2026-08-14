#include "Parser.h"

#include <iostream>
#include <variant>

void printOperand(const Operand& operand) {
    if (const auto* value = std::get_if<int64_t>(&operand)) {
        std::cout << *value;
        return;
    }

    const auto& reg = std::get<Register>(operand);
    std::cout << reg.name;
}

int main() {
    const auto program = parseReil("examples/testcase.reil");

    std::cout << "Parsed instructions: "
              << program.size()
              << '\n';

    for (const auto& instruction : program) {
        std::cout << static_cast<int>(instruction.opcode) << ' ';

        if (instruction.arg1) {
            printOperand(*instruction.arg1);
            std::cout << ' ';
        }

        if (instruction.arg2) {
            printOperand(*instruction.arg2);
            std::cout << ' ';
        }

        if (instruction.result) {
            printOperand(*instruction.result);
        }

        std::cout << '\n';
    }
}