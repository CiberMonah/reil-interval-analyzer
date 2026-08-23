#include "CFG.h"

#include <cstdint>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace {

bool isConditional(Opcode opcode) {
    return opcode == Opcode::Jge || opcode == Opcode::Jg ||
           opcode == Opcode::Jle || opcode == Opcode::Jl;
}

std::size_t jumpAddress(const Operand& operand) {
    const auto* address = std::get_if<int64_t>(&operand);
    if (address == nullptr || *address < 0) {
        throw std::runtime_error("Expected non-negative jump target");
    }
    return static_cast<std::size_t>(*address);
}

} // namespace

CFG::CFG(const std::vector<Instruction>& program) {
    std::unordered_map<std::size_t, std::size_t> addressToIndex;
    nodes_.reserve(program.size());

    for (std::size_t i = 0; i < program.size(); ++i) {
        if (!addressToIndex.emplace(program[i].address, i).second) {
            throw std::runtime_error(
                "Duplicate REIL address: " + std::to_string(program[i].address));
        }
        nodes_.push_back({i, {}, {}});
    }

    auto addEdge = [&](std::size_t from, std::size_t to,
                       std::optional<bool> condition = std::nullopt) {
        nodes_[from].successors.push_back({to, condition});
        nodes_[to].predecessors.push_back({from, condition});
    };

    auto targetIndex = [&](const Operand& operand) {
        const std::size_t address = jumpAddress(operand);
        const auto it = addressToIndex.find(address);
        if (it == addressToIndex.end()) {
            throw std::runtime_error(
                "Unknown jump target: " + std::to_string(address));
        }
        return it->second;
    };

    for (std::size_t i = 0; i < program.size(); ++i) {
        const Instruction& instruction = program[i];
        if (instruction.opcode == Opcode::Jmp) {
            addEdge(i, targetIndex(*instruction.arg1));
        } else if (isConditional(instruction.opcode)) {
            addEdge(i, targetIndex(*instruction.result), true);
            if (i + 1 < program.size()) {
                addEdge(i, i + 1, false);
            }
        } else if (i + 1 < program.size()) {
            addEdge(i, i + 1);
        }
    }
}

const std::vector<CFGNode>& CFG::nodes() const {
    return nodes_;
}

std::vector<std::size_t> CFG::topologicalOrder() const {
    std::vector<std::size_t> indegree(nodes_.size());
    std::queue<std::size_t> ready;
    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        indegree[i] = nodes_[i].predecessors.size();
        if (indegree[i] == 0) {
            ready.push(i);
        }
    }

    std::vector<std::size_t> order;
    order.reserve(nodes_.size());
    while (!ready.empty()) {
        const std::size_t node = ready.front();
        ready.pop();
        order.push_back(node);
        for (const CFGEdge& edge : nodes_[node].successors) {
            if (--indegree[edge.node] == 0) {
                ready.push(edge.node);
            }
        }
    }

    if (order.size() != nodes_.size()) {
        throw CFGCycleError();
    }
    return order;
}
