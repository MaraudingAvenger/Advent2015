#include <print>
#include <string>
#include <array>
#include <ranges>

#include "../common/utils.h"



bool hasNoForbiddenChars(std::string_view s) {
    static constexpr std::array<std::string_view, 3> forbidden{"i", "o", "l"};
    return std::ranges::none_of(forbidden, [s](std::string_view pair) { return s.contains(pair); });
}

bool hasIncreasingStraight(std::string_view s) {
    return std::ranges::any_of(
        s | std::views::adjacent<3>,
        [](auto triple) {
            auto [a, b, c] = triple;
            return b == a + 1 && c == b + 1;
        }
    );
}

bool hasTwoNonOverlappingPairs(std::string_view s) {
    auto pairLetters = s
        | std::views::pairwise
        | std::views::filter([](auto p) { auto [a, b] = p; return a == b; })
        | std::views::keys;   // just the doubled letter: "aabcc" -> 'a', 'c'

    // Any two neighbours that differ means there are at least two different pair letters
    return std::ranges::adjacent_find(pairLetters, std::ranges::not_equal_to{}) != pairLetters.end();
}

bool isValidPasswordPart1(std::string_view s) {
    return hasNoForbiddenChars(s) && hasIncreasingStraight(s) && hasTwoNonOverlappingPairs(s);
}

std::string nextPassword(std::string s) {
    for (int i = s.size() - 1; i >= 0; --i) {
        if (s[i] == 'z') {
            s[i] = 'a';
        } else {
            ++s[i];
            break;
        }
    }
    return s;
}

int main() {
    auto text = utils::readFileToString(utils::getInputFilePath("inputs/day11.txt"));

    std::string password = text;
    while (!isValidPasswordPart1(password)) {
        password = nextPassword(password);
    }
    std::println("part 1: {}", password);

    password = nextPassword(password);
    while (!isValidPasswordPart1(password)) {
        password = nextPassword(password);
    }
    std::println("part 2: {}", password);
}
