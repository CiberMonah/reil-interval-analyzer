#include "Analyser.h"

#include <algorithm>
#include <queue>
#include <stdexcept>

namespace {

enum class Relation { Ge, Gt, Le, Lt };

Relation relationFor(Opcode opcode, bool branch) {
    switch (opcode) {
    case Opcode::Jge: return branch ? Relation::Ge : Relation::Lt;
    case Opcode::Jg: return branch ? Relation::Gt : Relation::Le;
    case Opcode::Jle: return branch ? Relation::Le : Relation::Gt;
    case Opcode::Jl: return branch ? Relation::Lt : Relation::Ge;
    default: throw std::runtime_error("Expected conditional jump");
    }
}

bool possible(Relation relation, const Interval& lhs, const Interval& rhs) {
    switch (relation) {
    case Relation::Ge: return lhs.upper >= rhs.lower;
    case Relation::Gt: return lhs.upper > rhs.lower;
    case Relation::Le: return lhs.lower <= rhs.upper;
    case Relation::Lt: return lhs.lower < rhs.upper;
    }
    return false;
}

bool sameRegister(const Operand& lhs, const Operand& rhs) {
    const auto* left = std::get_if<Register>(&lhs);
    const auto* right = std::get_if<Register>(&rhs);
    return left != nullptr && right != nullptr && left->name == right->name;
}

void narrow(State& state, const Operand& operand, Interval constraint) {
    const auto* reg = std::get_if<Register>(&operand);
    if (reg == nullptr) {
        return;
    }
    auto it = state.find(reg->name);
    if (it == state.end()) {
        throw std::runtime_error("Undefined register: " + reg->name);
    }
    it->second.lower = std::max(it->second.lower, constraint.lower);
    it->second.upper = std::min(it->second.upper, constraint.upper);
}

State joinStates(const State& lhs, const State& rhs) {
    State result;
    for (const auto& [name, interval] : lhs) {
        const auto it = rhs.find(name);
        if (it != rhs.end()) {
            result.emplace(name, join(interval, it->second));
        }
    }
    return result;
}

bool equalStates(const State& lhs, const State& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (const auto& [name, interval] : lhs) {
        const auto it = rhs.find(name);
        if (it == rhs.end() || it->second.lower != interval.lower ||
            it->second.upper != interval.upper) {
            return false;
        }
    }
    return true;
}

const std::string& registerName(const Operand& operand) {
    const auto* reg = std::get_if<Register>(&operand);
    if (reg == nullptr) {
        throw std::runtime_error("Expected register destination");
    }
    return reg->name;
}

} // namespace

Analyser::Analyser(const std::vector<Instruction>& program)
    : program_(program), cfg_(program) {}

Interval Analyser::valueOf(const Operand& operand, const State& state) const {
    if (const auto* immediate = std::get_if<int64_t>(&operand)) {
        return {*immediate, *immediate};
    }
    const std::string& name = std::get<Register>(operand).name;
    const auto it = state.find(name);
    if (it == state.end()) {
        throw std::runtime_error("Undefined register: " + name);
    }
    return it->second;
}

State Analyser::transfer(const Instruction& instruction, const State& input) const {
    State output = input;
    switch (instruction.opcode) {
    case Opcode::Add:
        output[registerName(*instruction.result)] =
            add(valueOf(*instruction.arg1, input), valueOf(*instruction.arg2, input));
        break;
    case Opcode::Sub:
        output[registerName(*instruction.result)] =
            sub(valueOf(*instruction.arg1, input), valueOf(*instruction.arg2, input));
        break;
    case Opcode::Mul:
        output[registerName(*instruction.result)] =
            mul(valueOf(*instruction.arg1, input), valueOf(*instruction.arg2, input));
        break;
    case Opcode::Str:
        output[registerName(*instruction.result)] = valueOf(*instruction.arg1, input);
        break;
    case Opcode::Jge:
    case Opcode::Jg:
    case Opcode::Jle:
    case Opcode::Jl:
    case Opcode::Jmp:
    case Opcode::Nop:
        break;
    }
    return output;
}

std::optional<State> Analyser::refineEdge(
    const Instruction& instruction, const State& state, bool branch) const {
    const Operand& lhsOperand = *instruction.arg1;
    const Operand& rhsOperand = *instruction.arg2;
    const Relation relation = relationFor(instruction.opcode, branch);
    const Interval lhs = valueOf(lhsOperand, state);
    const Interval rhs = valueOf(rhsOperand, state);

    if (sameRegister(lhsOperand, rhsOperand)) {
        const bool strict = relation == Relation::Gt || relation == Relation::Lt;
        return strict ? std::nullopt : std::optional<State>(state);
    }
    if (!possible(relation, lhs, rhs)) {
        return std::nullopt;
    }

    State refined = state;
    switch (relation) {
    case Relation::Ge: {
        const Interval commonRange{rhs.lower, lhs.upper};
        narrow(refined, lhsOperand, commonRange);
        narrow(refined, rhsOperand, commonRange);
        break;
    }
    case Relation::Gt: {
        const Interval lhsConstraint{rhs.lower + 1, lhs.upper};
        const Interval rhsConstraint{rhs.lower, lhs.upper - 1};
        narrow(refined, lhsOperand, lhsConstraint);
        narrow(refined, rhsOperand, rhsConstraint);
        break;
    }
    case Relation::Le: {
        const Interval commonRange{lhs.lower, rhs.upper};
        narrow(refined, lhsOperand, commonRange);
        narrow(refined, rhsOperand, commonRange);
        break;
    }
    case Relation::Lt: {
        const Interval lhsConstraint{lhs.lower, rhs.upper - 1};
        const Interval rhsConstraint{lhs.lower + 1, rhs.upper};
        narrow(refined, lhsOperand, lhsConstraint);
        narrow(refined, rhsOperand, rhsConstraint);
        break;
    }
    }
    return refined;
}

Interval Analyser::analyse(const Interval& input) {
    if (program_.empty()) {
        throw std::runtime_error("Empty REIL program");
    }

    inputStates_.assign(program_.size(), std::nullopt);
    outputStates_.assign(program_.size(), std::nullopt);

    auto computeInput = [&](std::size_t nodeIndex) {
        std::optional<State> inputState;
        if (nodeIndex == 0) {
            inputState = State{{"arg0", input}};
        }

        for (const CFGEdge& edge : cfg_.nodes()[nodeIndex].predecessors) {
            if (!outputStates_[edge.node].has_value()) {
                continue;
            }
            std::optional<State> incoming = edge.condition.has_value()
                ? refineEdge(program_[edge.node], *outputStates_[edge.node], *edge.condition)
                : outputStates_[edge.node];
            if (!incoming.has_value()) {
                continue;
            }
            inputState = inputState.has_value()
                ? std::optional<State>(joinStates(*inputState, *incoming))
                : incoming;
        }

        return inputState;
    };

    try {
        const auto order = cfg_.topologicalOrder();
        for (const std::size_t nodeIndex : order) {
            std::optional<State> inputState = computeInput(nodeIndex);

            if (!inputState.has_value()) {
                continue;
            }
            inputStates_[nodeIndex] = inputState;
            outputStates_[nodeIndex] = transfer(program_[nodeIndex], *inputState);
        }
    } catch (const CFGCycleError&) {
        std::queue<std::size_t> worklist;
        std::vector<bool> queued(program_.size(), false);
        worklist.push(0);
        queued[0] = true;

        while (!worklist.empty()) {
            const std::size_t nodeIndex = worklist.front();
            worklist.pop();
            queued[nodeIndex] = false;

            std::optional<State> inputState = computeInput(nodeIndex);
            if (!inputState.has_value()) {
                continue;
            }

            const State outputState = transfer(program_[nodeIndex], *inputState);
            const bool outputChanged = !outputStates_[nodeIndex].has_value() ||
                !equalStates(*outputStates_[nodeIndex], outputState);
            inputStates_[nodeIndex] = std::move(inputState);

            if (!outputChanged) {
                continue;
            }
            outputStates_[nodeIndex] = outputState;

            for (const CFGEdge& edge : cfg_.nodes()[nodeIndex].successors) {
                if (!queued[edge.node]) {
                    worklist.push(edge.node);
                    queued[edge.node] = true;
                }
            }
        }
    }

    std::optional<Interval> result;
    for (std::size_t i = 0; i < cfg_.nodes().size(); ++i) {
        if (!cfg_.nodes()[i].successors.empty() || !outputStates_[i].has_value()) {
            continue;
        }
        const auto it = outputStates_[i]->find("ret");
        if (it == outputStates_[i]->end()) {
            throw std::runtime_error("Program finished without ret value");
        }
        result = result.has_value() ? join(*result, it->second) : it->second;
    }
    if (!result.has_value()) {
        throw std::runtime_error("Program has no reachable exit");
    }
    return *result;
}

const std::vector<std::optional<State>>& Analyser::inputStates() const {
    return inputStates_;
}

const std::vector<std::optional<State>>& Analyser::outputStates() const {
    return outputStates_;
}
