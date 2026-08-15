#pragma once

#include "Instruction.h"
#include "Interval.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

using State = std::unordered_map<std::string, Interval>;

class Analyser {
public:
    explicit Analyser(const std::vector<Instruction>& program);

    Interval analyse(const Interval& input);

private:
    const std::vector<Instruction>& program_;

    std::unordered_map<std::size_t, std::size_t> addressToIndex_;

    Interval getValue(
        const Operand& operand,
        const State& state
    ) const;

    const std::string& getRegisterName(
        const Operand& operand
    ) const;

    std::size_t getTargetIndex(
        const Operand& operand
    ) const;

    bool mergeState(
        State& destination,
        const State& source
    ) const;
};