#include "Analyser.h"
#include "CFG.h"
#include "Interval.h"
#include "Parser.h"

#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void checkInterval(Interval actual, Interval expected, const std::string& message) {
    check(actual.lower == expected.lower && actual.upper == expected.upper, message);
}

Operand reg(const char* name) {
    return Register{name};
}

Instruction instruction(std::size_t address, Opcode opcode,
                        std::optional<Operand> arg1 = std::nullopt,
                        std::optional<Operand> arg2 = std::nullopt,
                        std::optional<Operand> result = std::nullopt) {
    return {address, opcode, std::move(arg1), std::move(arg2), std::move(result)};
}

void testIntervals() {
    checkInterval(add({1, 3}, {4, 6}), {5, 9}, "interval add");
    checkInterval(sub({1, 3}, {4, 6}), {-5, -1}, "interval sub");
    checkInterval(mul({2, 3}, {4, 5}), {8, 15}, "positive multiplication");
    checkInterval(mul({-3, -2}, {4, 5}), {-15, -8}, "negative multiplication");
    checkInterval(mul({-2, 3}, {-4, 5}), {-12, 15}, "multiplication across zero");
    checkInterval(join({1, 4}, {-2, 3}), {-2, 4}, "interval join");
}

void testParser() {
    const auto program = parseReil(std::string(TEST_SOURCE_DIR) + "/parser.reil");
    check(program.size() == 5, "parser instruction count");
    check(program[0].address == 3 && program[4].address == 8,
          "comments and empty lines preserve source addresses");
    check(program[0].opcode == Opcode::Jge && program[1].opcode == Opcode::Jg &&
          program[2].opcode == Opcode::Jle && program[3].opcode == Opcode::Jl,
          "all conditional opcodes parse");
    check(std::get<int64_t>(*program[0].arg2) == -3, "negative immediate parses");
    check(std::get<int64_t>(*program[0].result) == 8,
          "jump line address is preserved");

    try {
        parseReil(std::string(TEST_SOURCE_DIR) + "/parser_invalid.reil");
        check(false, "parser rejects an extra operand");
    } catch (const std::runtime_error&) {
    }
}

void testCFG() {
    const std::vector<Instruction> program{
        instruction(10, Opcode::Str, int64_t{0}, {}, reg("ret")),
        instruction(20, Opcode::Jge, reg("arg0"), int64_t{3}, int64_t{50}),
        instruction(30, Opcode::Jmp, int64_t{60}),
        instruction(50, Opcode::Str, int64_t{1}, {}, reg("ret")),
        instruction(60, Opcode::Nop)
    };
    const CFG cfg(program);
    check(cfg.nodes()[0].successors[0].node == 1, "fallthrough edge");
    check(cfg.nodes()[1].successors.size() == 2 &&
          cfg.nodes()[1].successors[0].node == 3 &&
          cfg.nodes()[1].successors[1].node == 2,
          "conditional true and false edges use address mapping");
    check(cfg.nodes()[2].successors[0].node == 4, "unconditional jump edge");
    check(cfg.nodes()[4].predecessors.size() == 2, "predecessors are constructed");
    check(cfg.topologicalOrder().size() == program.size(), "topological order covers CFG");

    const std::vector<Instruction> cyclic{
        instruction(1, Opcode::Nop), instruction(2, Opcode::Jmp, int64_t{1})};
    try {
        CFG(cyclic).topologicalOrder();
        check(false, "cycle must be rejected");
    } catch (const CFGCycleError& error) {
        check(std::string(error.what()) == "CFG contains a cycle", "cycle diagnostic");
    }
}

std::vector<Instruction> branchProgram(Opcode opcode) {
    return {
        instruction(1, opcode, reg("arg0"), int64_t{3}, int64_t{4}),
        instruction(2, Opcode::Str, int64_t{0}, {}, reg("ret")),
        instruction(3, Opcode::Jmp, int64_t{5}),
        instruction(4, Opcode::Str, int64_t{1}, {}, reg("ret")),
        instruction(5, Opcode::Nop)};
}

void testAnalysis() {
    {
        const std::vector<Instruction> program{
            instruction(1, Opcode::Add, reg("arg0"), int64_t{2}, reg("t0")),
            instruction(2, Opcode::Mul, reg("t0"), int64_t{3}, reg("ret"))};
        Analyser analyser(program);
        checkInterval(analyser.analyse({1, 2}), {9, 12}, "straight-line arithmetic");
        check(analyser.inputStates().size() == 2 && analyser.outputStates().size() == 2,
              "IN and OUT are stored per instruction");
    }
    {
        auto program = branchProgram(Opcode::Jge);
        Analyser analyser(program);
        checkInterval(analyser.analyse({2, 2}), {0, 0}, "conditional always false");
    }
    {
        auto program = branchProgram(Opcode::Jge);
        Analyser analyser(program);
        checkInterval(analyser.analyse({4, 4}), {1, 1}, "conditional always true");
    }
    {
        auto program = branchProgram(Opcode::Jge);
        Analyser analyser(program);
        checkInterval(analyser.analyse({2, 4}), {0, 1}, "both branches and join");
        checkInterval(analyser.inputStates()[4]->at("ret"), {0, 1}, "joined IN state");
    }

    const auto supplied = parseReil(
        std::string(PROJECT_SOURCE_DIR_PATH) + "/examples/testcase.reil");
    Analyser first(supplied);
    checkInterval(first.analyse({0, 2}), {0, 2}, "supplied testcase [0,2]");
    Analyser second(supplied);
    checkInterval(second.analyse({1, 5}), {1, 5}, "supplied testcase [1,5]");
}

} // namespace

int main() {
    try {
        testIntervals();
        testParser();
        testCFG();
        testAnalysis();
    } catch (const std::exception& error) {
        std::cerr << "Unexpected exception: " << error.what() << '\n';
        return 1;
    }
    return failures == 0 ? 0 : 1;
}
