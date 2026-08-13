#pragma once

#include <cstdint>

struct Interval {
    int64_t lower;
    int64_t upper;
};

Interval add(const Interval& lhs, const Interval& rhs);
Interval sub(const Interval& lhs, const Interval& rhs);
Interval mul(const Interval& lhs, const Interval& rhs);
Interval join(const Interval& lhs, const Interval& rhs);