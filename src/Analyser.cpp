#include "Analyser.h"

#include <algorithm>
#include <optional>
#include <queue>
#include <stdexcept>
#include <utility>

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

    for (const auto& [name, interval] : source) {
        auto [it, inserted] =
            destination.emplace(name, interval);

        if (inserted) {
            changed = true;
            continue;
        }

        const Interval merged = join(it->second, interval);

        if (merged.lower != it->second.lower ||
            merged.upper != it->second.upper) {

            it->second = merged;
            changed = true;
        }
    }

    return changed;
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

        case Opcode::Jge: {
            /*
             * Пока поддерживаем форму, которая есть
             * в тестовом задании:
             *
             *     jge register immediate target
             *
             * Например:
             *
             *     jge arg0 3 10
             */

            const auto* reg =
                std::get_if<Register>(&*instruction.arg1);

            const auto* threshold =
                std::get_if<int64_t>(&*instruction.arg2);

            if (reg == nullptr || threshold == nullptr) {
                throw std::runtime_error(
                    "Jge currently expects: "
                    "register immediate target"
                );
            }

            const auto stateIt = state.find(reg->name);

            if (stateIt == state.end()) {
                throw std::runtime_error(
                    "Undefined register in Jge: " +
                    reg->name
                );
            }

            const Interval current = stateIt->second;

            const std::size_t target =
                getTargetIndex(*instruction.result);

            /*
             * TRUE:
             *
             * x >= threshold
             */
            if (current.upper >= *threshold) {
                State trueState = state;

                trueState[reg->name] = {
                    std::max(current.lower, *threshold),
                    current.upper
                };

                enqueue(target, trueState);
            }

            /*
             * FALSE:
             *
             * x < threshold
             *
             * Так как у нас целые числа:
             *
             * x <= threshold - 1
             */
            if (current.lower < *threshold) {
                State falseState = state;

                falseState[reg->name] = {
                    current.lower,
                    std::min(
                        current.upper,
                        *threshold - 1
                    )
                };

                enqueue(pc + 1, falseState);
            }

            break;
        }

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