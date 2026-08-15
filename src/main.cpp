#include "Analyser.h"
#include "Parser.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr
            << "Usage: "
            << argv[0]
            << " <reil-file> <lower> <upper>\n";

        return 1;
    }

    try {
        const std::string filename = argv[1];

        const int64_t lower = std::stoll(argv[2]);
        const int64_t upper = std::stoll(argv[3]);

        if (lower > upper) {
            throw std::runtime_error(
                "Lower bound must not be greater than upper bound"
            );
        }

        const auto program = parseReil(filename);

        Analyser analyser(program);

        const Interval input{lower, upper};
        const Interval result = analyser.analyse(input);

        std::cout
            << "Input:  ["
            << input.lower << ", "
            << input.upper << "]\n"
            << "Output: ["
            << result.lower << ", "
            << result.upper << "]\n";

    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}