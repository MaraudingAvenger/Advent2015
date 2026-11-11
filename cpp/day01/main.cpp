#include <algorithm>
#include <cstddef>
#include <functional>
#include <print>
#include <ranges>
#include <stdexcept>
#include <string_view>

#include "../common/utils.h"

namespace {

constexpr int step(char c) {
    switch (c) {
        case '(': return 1;
        case ')': return -1;
        default:  return 0;
    }
}

// 1-based position of the first move that takes Santa to floor -1.
// C++23 has no lazy running-sum view (no views::scan / inclusive_scan view), so this stays a
// loop; enumerate supplies the position alongside each character.
std::ptrdiff_t firstBasementPosition(std::string_view moves) {
    int floor = 0;
    for (const auto [i, c] : moves | std::views::enumerate) {
        floor += step(c);
        if (floor == -1) {
            return i + 1;
        }
    }
    throw std::runtime_error("Santa never reaches the basement");
}

}  // namespace

int main() {
    const auto moves = utils::readFileToString(utils::getInputFilePath("inputs/day01.txt"));

    std::println("part 1: {}", std::ranges::fold_left(moves | std::views::transform(step), 0, std::plus{}));
    std::println("part 2: {}", firstBasementPosition(moves));
}
