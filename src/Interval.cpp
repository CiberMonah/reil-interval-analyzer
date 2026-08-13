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
    Interval res;

    res.lower = std::min({      lhs.lower * rhs.lower, 
                                lhs.upper * rhs.lower,
                                lhs.lower * rhs.upper,
                                lhs.upper * rhs.upper
                        });
    res.upper = std::max({      lhs.lower * rhs.lower, 
                                lhs.upper * rhs.lower,
                                lhs.lower * rhs.upper,
                                lhs.upper * rhs.upper
                        });

    return res;
}

Interval join(const Interval &lhs, const Interval &rhs) {
    Interval res;

    res.lower = std::min ({lhs.lower,
                            rhs.lower});

    res.upper = std::max ({lhs.upper,
                            rhs.upper});


    return res;
}

