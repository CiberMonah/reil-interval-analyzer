#include "Interval.h"
#include <algorithm>

Interval add(const Interval& lhs, const Interval& rhs) {
    Interval res;

    res.lower = lhs.lower + rhs.lower;
    res.upper = lhs.upper + rhs.upper;

    return res;
}

Interval sub(const Interval& lhs, const Interval& rhs) {
    Interval res;

    res.lower = lhs.lower - rhs.upper;
    res.upper = lhs.upper - rhs.lower;

    return res;
}

Interval mul(const Interval& lhs, const Interval& rhs) {
    const int64_t lowerLower = lhs.lower * rhs.lower;
    const int64_t lowerUpper = lhs.lower * rhs.upper;
    const int64_t upperLower = lhs.upper * rhs.lower;
    const int64_t upperUpper = lhs.upper * rhs.upper;

    return {
        std::min({lowerLower, lowerUpper, upperLower, upperUpper}),
        std::max({lowerLower, lowerUpper, upperLower, upperUpper})
    };
}

Interval join(const Interval &lhs, const Interval &rhs) {
    Interval res;

    res.lower = std::min ({lhs.lower,
                            rhs.lower});

    res.upper = std::max ({lhs.upper,
                            rhs.upper});


    return res;
}
