#include "Analyser.h"

#include <algorithm>
#include <optional>
#include <queue>
#include <stdexcept>
#include <utility>

namespace {

bool canBeTrue(Opcode opcode, const Interval& lhs, const Interval& rhs) {
    switch (opcode) {
    case Opcode::Jge: return lhs.upper >= rhs.lower;
    case Opcode::Jg: return lhs.upper > rhs.lower;
    case Opcode::Jle: return lhs.lower <= rhs.upper;
    case Opcode::Jl: return lhs.lower < rhs.upper;
    default: throw std::runtime_error("Expected conditional jump");
    }
}

bool canBeFalse(Opcode opcode, const Interval& lhs, const Interval& rhs) {
    switch (opcode) {
    case Opcode::Jge: return lhs.lower < rhs.upper;
    case Opcode::Jg: return lhs.lower <= rhs.upper;
    case Opcode::Jle: return lhs.upper > rhs.lower;
    case Opcode::Jl: return lhs.upper >= rhs.lower;
    default: throw std::runtime_error("Expected conditional jump");
    }
}

void narrow(State& state, const Operand& operand, const Interval& constraint) {
    const auto* reg = std::get_if<Register>(&operand);
    if (reg == nullptr) {
        return;
    }

    Interval& value = state.at(reg->name);
    value.lower = std::max(value.lower, constraint.lower);
    value.upper = std::min(value.upper, constraint.upper);
}

bool sameRegister(const Operand& lhs, const Operand& rhs) {
    const auto* lhsRegister = std::get_if<Register>(&lhs);
    const auto* rhsRegister = std::get_if<Register>(&rhs);
    return lhsRegister != nullptr && rhsRegister != nullptr &&
           lhsRegister->name == rhsRegister->name;
}

} // namespace

Analyser::Analyser(const std::vector<Instruction>& program)
    : program_(program) {

    for (std::size_t i = 0; i < program_.size(); ++i) {
        const auto [it, inserted] =
            addressToIndex_.emplace(program_[i].address, i);

        if (!inserted) {
            throw std::runtime_error(
                "Duplicate REIL address: " +
                std::to_string(program_[i].address)
            );
        }
    }
}

Interval Analyser::getValue(
    const Operand& operand,
    const State& state
) const {
    if (const auto* immediate = std::get_if<int64_t>(&operand)) {
        return {*immediate, *immediate};
    }

    const auto& reg = std::get<Register>(operand);

    const auto it = state.find(reg.name);

    if (it == state.end()) {
        throw std::runtime_error(
            "Undefined register: " + reg.name
        );
    }

    return it->second;
}

const std::string& Analyser::getRegisterName(
    const Operand& operand
) const {
    const auto* reg = std::get_if<Register>(&operand);

    if (reg == nullptr) {
        throw std::runtime_error(
            "Expected register operand"
        );
    }

    return reg->name;
}

std::size_t Analyser::getTargetIndex(
    const Operand& operand
) const {
    const auto* target = std::get_if<int64_t>(&operand);

    if (target == nullptr || *target < 0) {
        throw std::runtime_error(
            "Expected non-negative jump target"
        );
    }

    const auto it =
        addressToIndex_.find(static_cast<std::size_t>(*target));

    if (it == addressToIndex_.end()) {
        throw std::runtime_error(
            "Unknown jump target: " +
            std::to_string(*target)
        );
    }

    return it->second;
}

bool Analyser::mergeState(
    State& destination,
    const State& source
) const {
    bool changed = false;

    for (auto it = destination.begin(); it != destination.end();) {
        const auto sourceIt = source.find(it->first);

        if (sourceIt == source.end()) {
            it = destination.erase(it);
            changed = true;
            continue;
        }

        const Interval merged = join(it->second, sourceIt->second);

        if (merged.lower != it->second.lower ||
            merged.upper != it->second.upper) {

            it->second = merged;
            changed = true;
        }

        ++it;
    }

    return changed;
}

void Analyser::analyseConditionalJump(
    const Instruction& instruction,
    std::size_t pc,
    const State& state,
    const std::function<void(std::size_t, const State&)>& enqueue
) const {
    const Interval lhs = getValue(*instruction.arg1, state);
    const Interval rhs = getValue(*instruction.arg2, state);
    const std::size_t target = getTargetIndex(*instruction.result);
    const bool operandsAreEqual =
        sameRegister(*instruction.arg1, *instruction.arg2);
    const bool trueBranchPossible = operandsAreEqual
        ? instruction.opcode == Opcode::Jge || instruction.opcode == Opcode::Jle
        : canBeTrue(instruction.opcode, lhs, rhs);
    const bool falseBranchPossible = operandsAreEqual
        ? instruction.opcode == Opcode::Jg || instruction.opcode == Opcode::Jl
        : canBeFalse(instruction.opcode, lhs, rhs);

    if (trueBranchPossible) {
        State branch = state;

        switch (instruction.opcode) {
        case Opcode::Jge:
            narrow(branch, *instruction.arg1, {rhs.lower, lhs.upper});
            narrow(branch, *instruction.arg2, {rhs.lower, lhs.upper});
            break;
        case Opcode::Jg:
            narrow(branch, *instruction.arg1, {rhs.lower + 1, lhs.upper});
            narrow(branch, *instruction.arg2, {rhs.lower, lhs.upper - 1});
            break;
        case Opcode::Jle:
            narrow(branch, *instruction.arg1, {lhs.lower, rhs.upper});
            narrow(branch, *instruction.arg2, {lhs.lower, rhs.upper});
            break;
        case Opcode::Jl:
            narrow(branch, *instruction.arg1, {lhs.lower, rhs.upper - 1});
            narrow(branch, *instruction.arg2, {lhs.lower + 1, rhs.upper});
            break;
        default:
            break;
        }

        enqueue(target, branch);
    }

    if (falseBranchPossible) {
        State branch = state;

        switch (instruction.opcode) {
        case Opcode::Jge:
            narrow(branch, *instruction.arg1, {lhs.lower, rhs.upper - 1});
            narrow(branch, *instruction.arg2, {lhs.lower + 1, rhs.upper});
            break;
        case Opcode::Jg:
            narrow(branch, *instruction.arg1, {lhs.lower, rhs.upper});
            narrow(branch, *instruction.arg2, {lhs.lower, rhs.upper});
            break;
        case Opcode::Jle:
            narrow(branch, *instruction.arg1, {rhs.lower + 1, lhs.upper});
            narrow(branch, *instruction.arg2, {rhs.lower, lhs.upper - 1});
            break;
        case Opcode::Jl:
            narrow(branch, *instruction.arg1, {rhs.lower, lhs.upper});
            narrow(branch, *instruction.arg2, {rhs.lower, lhs.upper});
            break;
        default:
            break;
        }

        enqueue(pc + 1, branch);
    }
}

Interval Analyser::analyse(const Interval& input) {
    if (program_.empty()) {
        throw std::runtime_error("Empty REIL program");
    }

    std::unordered_map<std::size_t, State> inputStates;
    std::queue<std::size_t> worklist;

    std::optional<Interval> finalResult;

    State initialState;
    initialState.emplace("arg0", input);

    inputStates.emplace(0, initialState);
    worklist.push(0);

    auto finish = [&](const State& state) {
        const auto it = state.find("ret");

        if (it == state.end()) {
            throw std::runtime_error(
                "Program finished without ret value"
            );
        }

        if (!finalResult.has_value()) {
            finalResult = it->second;
        } else {
            finalResult = join(*finalResult, it->second);
        }
    };

    auto enqueue = [&](std::size_t pc, const State& state) {
        if (pc >= program_.size()) {
            finish(state);
            return;
        }

        auto [it, inserted] =
            inputStates.emplace(pc, state);

        if (inserted) {
            worklist.push(pc);
            return;
        }

        if (mergeState(it->second, state)) {
            worklist.push(pc);
        }
    };

    while (!worklist.empty()) {
        const std::size_t pc = worklist.front();
        worklist.pop();

        State state = inputStates.at(pc);
        const Instruction& instruction = program_[pc];

        switch (instruction.opcode) {
        case Opcode::Add: {
            const Interval lhs =
                getValue(*instruction.arg1, state);
            const Interval rhs =
                getValue(*instruction.arg2, state);

            const std::string& destination =
                getRegisterName(*instruction.result);

            state[destination] = add(lhs, rhs);

            enqueue(pc + 1, state);
            break;
        }

        case Opcode::Sub: {
            const Interval lhs =
                getValue(*instruction.arg1, state);
            const Interval rhs =
                getValue(*instruction.arg2, state);

            const std::string& destination =
                getRegisterName(*instruction.result);

            state[destination] = sub(lhs, rhs);

            enqueue(pc + 1, state);
            break;
        }

        case Opcode::Mul: {
            const Interval lhs =
                getValue(*instruction.arg1, state);
            const Interval rhs =
                getValue(*instruction.arg2, state);

            const std::string& destination =
                getRegisterName(*instruction.result);

            state[destination] = mul(lhs, rhs);

            enqueue(pc + 1, state);
            break;
        }

        case Opcode::Str: {
            const Interval value =
                getValue(*instruction.arg1, state);

            const std::string& destination =
                getRegisterName(*instruction.result);

            state[destination] = value;

            enqueue(pc + 1, state);
            break;
        }

        case Opcode::Jmp: {
            const std::size_t target =
                getTargetIndex(*instruction.arg1);

            enqueue(target, state);
            break;
        }

        case Opcode::Jge:
        case Opcode::Jg:
        case Opcode::Jle:
        case Opcode::Jl:
            analyseConditionalJump(instruction, pc, state, enqueue);
            break;

        case Opcode::Nop:
            enqueue(pc + 1, state);
            break;
        }
    }

    if (!finalResult.has_value()) {
        throw std::runtime_error(
            "Program has no reachable exit"
        );
    }

    return *finalResult;
}
