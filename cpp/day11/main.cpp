#include <algorithm>
#include <functional>
#include <print>
#include <ranges>
#include <string>
#include <string_view>

#include "../common/utils.h"

namespace {

bool hasNoForbiddenChars(std::string_view s) {
    return s.find_first_of("iol") == std::string_view::npos;
}

bool hasIncreasingStraight(std::string_view s) {
    return std::ranges::any_of(s | std::views::adjacent<3>, [](auto triple) {
        auto [a, b, c] = triple;
        return b == a + 1 && c == b + 1;
    });
}

bool hasTwoNonOverlappingPairs(std::string_view s) {
    auto pairLetters = s
        | std::views::pairwise
        | std::views::filter([](auto p) { auto [a, b] = p; return a == b; })
        | std::views::keys;   // just the doubled letter: "aabcc" -> 'a', 'c'

    // Any two neighbours that differ means there are at least two different pair letters
    return std::ranges::adjacent_find(pairLetters, std::ranges::not_equal_to{}) != pairLetters.end();
}

bool isValidPassword(std::string_view s) {
    return hasNoForbiddenChars(s) && hasIncreasingStraight(s) && hasTwoNonOverlappingPairs(s);
}

// Counts up in base 26 like an odometer: walk from the rightmost letter, turning each 'z' into
// 'a' (carry) until a letter can simply be bumped.
void increment(std::string& password) {
    for (char& c : password | std::views::reverse) {
        if (c != 'z') {
            ++c;
            return;
        }
        c = 'a';
    }
}

// do/while rather than while: the answer is the next valid password, never the current one.
std::string nextValidPassword(std::string password) {
    do {
        increment(password);
    } while (!isValidPassword(password));
    return password;
}

}  // namespace

int main() {
    const auto input = utils::readFileToString(utils::getInputFilePath("inputs/day11.txt"));

    const auto part1 = nextValidPassword(std::string{utils::trim(input)});
    std::println("part 1: {}", part1);
    std::println("part 2: {}", nextValidPassword(part1));
}
