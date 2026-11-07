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

bool partOneIsNice(std::string_view s) {
    return hasNoForbiddenPairs(s) && hasThreeVowels(s) && hasDoubleLetter(s);
}

// must contain a pair of any two letters that appears at least twice without overlapping
// Iterates by index rather than over windows because the search for a repeat has to start
// after the pair itself; starting any earlier lets a pair match itself or overlap ("aaa").
bool hasRepeatedPair(std::string_view s) {
    const std::size_t pairCount = s.size() < 2 ? 0 : s.size() - 1;
    return std::ranges::any_of(std::views::iota(0uz, pairCount), [s](std::size_t i) {
        return s.substr(i + 2).contains(s.substr(i, 2));
    });
}

// must contain two of the same letter with exactly one letter between them
bool hasRepeatedLetterWithOneBetween(std::string_view s) {
    return std::ranges::any_of(s | std::views::slide(3), [](auto window) {
        return window[0] == window[2];
    });
}

bool partTwoIsNice(std::string_view s) {
    return hasRepeatedPair(s) && hasRepeatedLetterWithOneBetween(s);
}

}

int main() {
    const auto lines = utils::readFileToLines(utils::getInputFilePath("inputs/day05.txt"));
    std::println("part 1: {}", std::ranges::count_if(lines, partOneIsNice));
    std::println("part 2: {}", std::ranges::count_if(lines, partTwoIsNice));
}
