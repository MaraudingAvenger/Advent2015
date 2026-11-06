#include <algorithm>
#include <array>
#include <print>
#include <ranges>
#include <string_view>

#include "../common/utils.h"

namespace {

// must not contain "ab", "cd", "pq", or "xy"
bool hasNoForbiddenPairs(std::string_view s) {
    static constexpr std::array<std::string_view, 4> forbidden{"ab", "cd", "pq", "xy"};
    return std::ranges::none_of(forbidden, [s](std::string_view pair) { return s.contains(pair); });
}

// must contain at least three vowels
bool hasThreeVowels(std::string_view s) {
    constexpr std::string_view vowels = "aeiou";
    return std::ranges::count_if(s, [vowels](char c) { return vowels.contains(c); }) >= 3;
}

// must contain at least one letter that appears twice in a row
bool hasDoubleLetter(std::string_view s) {
    return std::ranges::any_of(s | std::views::pairwise, [](auto pair) {
        auto [a, b] = pair;
        return a == b;
    });
}

bool isNice(std::string_view s) {
    return hasNoForbiddenPairs(s) && hasThreeVowels(s) && hasDoubleLetter(s);
}

}

int main() {
    const auto lines = utils::readFileToLines(utils::getInputFilePath("inputs/day05.txt"));
    std::println("part 1: {}", std::ranges::count_if(lines, isNice));
}
