#include "Parser.h"

#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

Opcode parseOpcode(const std::string& token) {
    if (token == "add") {
        return Opcode::Add;
    }
    if (token == "sub") {
        return Opcode::Sub;
    }
    if (token == "mul") {
        return Opcode::Mul;
    }
    if (token == "str") {
        return Opcode::Str;
    }
    if (token == "jge") {
        return Opcode::Jge;
    }
    if (token == "jg") {
        return Opcode::Jg;
    }
    if (token == "jle") {
        return Opcode::Jle;
    }
    if (token == "jl") {
        return Opcode::Jl;
    }
    if (token == "jmp") {
        return Opcode::Jmp;
    }
    if (token == "nop") {
        return Opcode::Nop;
    }

    throw std::runtime_error("Unknown opcode: " + token);
}

bool isInteger(const std::string& token) {
    if (token.empty()) {
        return false;
    }

    std::size_t pos = 0;

    if (token[0] == '-') {
        if (token.size() == 1) {
            return false;
        }

        pos = 1;
    }

    for (; pos < token.size(); ++pos) {
        if (token[pos] < '0' || token[pos] > '9') {
            return false;
        }
    }

    return true;
}

Operand parseOperand(const std::string& token) {
    if (isInteger(token)) {
        return static_cast<int64_t>(std::stoll(token));
    }

    return Register{token};
}

Instruction parseInstruction(
    const std::string& line,
    std::size_t address
) {
    std::istringstream stream(line);

    std::string opcodeToken;
    stream >> opcodeToken;

    Instruction instruction{
        .address = address,
        .opcode = parseOpcode(opcodeToken),
        .arg1 = std::nullopt,
        .arg2 = std::nullopt,
        .result = std::nullopt
    };

    std::string arg1;
    std::string arg2;
    std::string result;

    switch (instruction.opcode) {
    case Opcode::Add:
    case Opcode::Sub:
    case Opcode::Mul:
    case Opcode::Jge:
    case Opcode::Jg:
    case Opcode::Jle:
    case Opcode::Jl:
        if (!(stream >> arg1 >> arg2 >> result)) {
            throw std::runtime_error(
                "Expected three operands: " + line
            );
        }

        instruction.arg1 = parseOperand(arg1);
        instruction.arg2 = parseOperand(arg2);
        instruction.result = parseOperand(result);
        break;

    case Opcode::Str:
        if (!(stream >> arg1 >> result)) {
            throw std::runtime_error(
                "Expected two operands: " + line
            );
        }

        instruction.arg1 = parseOperand(arg1);
        instruction.result = parseOperand(result);
        break;

    case Opcode::Jmp:
        if (!(stream >> arg1)) {
            throw std::runtime_error(
                "Expected jump target: " + line
            );
        }

        instruction.arg1 = parseOperand(arg1);
        break;

    case Opcode::Nop:
        break;
    }

    std::string extra;
    if (stream >> extra) {
        throw std::runtime_error("Unexpected operand: " + extra);
    }

    return instruction;
}

} // namespace

std::vector<Instruction> parseReil(const std::string& filename) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        throw std::runtime_error(
            "Failed to open REIL file: " + filename
        );
    }

    std::vector<Instruction> program;

    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;

        const std::size_t commentPos = line.find('#');

        if (commentPos != std::string::npos) {
            line.erase(commentPos);
        }

        if (line.find_first_not_of(" \t\r\n") == std::string::npos) {
            continue;
        }

        try {
            program.push_back(
                parseInstruction(line, lineNumber)
            );
        } catch (const std::exception& error) {
            throw std::runtime_error(
                "Error at line " +
                std::to_string(lineNumber) +
                ": " +
                error.what()
            );
        }
    }

    return program;
}
