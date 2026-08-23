#pragma once

#include "Instruction.h"

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <vector>

class CFGCycleError : public std::runtime_error {
public:
    CFGCycleError() : std::runtime_error("CFG contains a cycle") {}
};

struct CFGEdge {
    std::size_t node;
    std::optional<bool> condition;
};

struct CFGNode {
    std::size_t instructionIndex;
    std::vector<CFGEdge> predecessors;
    std::vector<CFGEdge> successors;
};

class CFG {
public:
    explicit CFG(const std::vector<Instruction>& program);

    const std::vector<CFGNode>& nodes() const;
    std::vector<std::size_t> topologicalOrder() const;

private:
    std::vector<CFGNode> nodes_;
};
