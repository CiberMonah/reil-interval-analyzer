#pragma once

#include "CFG.h"
#include "Instruction.h"
#include "Interval.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

using State = std::unordered_map<std::string, Interval>;

class Analyser {
public:
    explicit Analyser(const std::vector<Instruction>& program);

    Interval analyse(const Interval& input);
    const std::vector<std::optional<State>>& inputStates() const;
    const std::vector<std::optional<State>>& outputStates() const;

private:
    const std::vector<Instruction>& program_;
    CFG cfg_;
    std::vector<std::optional<State>> inputStates_;
    std::vector<std::optional<State>> outputStates_;

    Interval valueOf(const Operand& operand, const State& state) const;
    State transfer(const Instruction& instruction, const State& input) const;
    std::optional<State> refineEdge(
        const Instruction& instruction,
        const State& state,
        bool branch) const;
};
