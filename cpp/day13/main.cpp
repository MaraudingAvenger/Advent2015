#include <algorithm>
#include <format>
#include <functional>
#include <limits>
#include <print>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../common/utils.h"

namespace {

struct Preference {
    std::string_view person;
    std::string_view neighbor;
    int happiness;
};

// "Alice would gain 54 happiness units by sitting next to Bob."
//  [0]   [1]   [2]  [3] [4]       [5]   [6] [7]   [8] [9] [10]
Preference parsePreference(std::string_view line) {
    const auto tokens = utils::splitString(line);
    if (tokens.size() != 11 || (tokens[2] != "gain" && tokens[2] != "lose")) {
        throw std::invalid_argument(std::format("malformed preference: '{}'", line));
    }
    const int amount = utils::parseInt(tokens[3]);
    return {tokens[0], utils::trim(tokens[10], "."), tokens[2] == "gain" ? amount : -amount};
}

}  // namespace

int main() {
    const auto lines = utils::readFileToLines(utils::getInputFilePath("inputs/day13.txt"));
    const auto preferences = lines | std::views::transform(parsePreference) | std::ranges::to<std::vector>();

    // Number the guests 0..n-1 so happiness lives in a matrix and a seating is just a permutation.
    std::unordered_map<std::string_view, int> guestIndex;
    for (const auto& p : preferences) {
        guestIndex.try_emplace(p.person, static_cast<int>(guestIndex.size()));
    }
    const int guestCount = static_cast<int>(guestIndex.size());

    // Every guest has an opinion about every other guest; a gap would silently read as 0.
    if (std::cmp_not_equal(preferences.size(), guestCount * (guestCount - 1))) {
        throw std::runtime_error("expected a preference for every ordered pair of guests");
    }

    // Sitting together is mutual: Alice next to Bob applies Alice's opinion of Bob AND Bob's of
    // Alice. Folding both into one symmetric number means the seating loop never has to care
    // about direction. (.at() also catches a neighbor who never appears as a person.)
    utils::Grid<int> pairHappiness(guestCount, guestCount);
    for (const auto& [person, neighbor, happiness] : preferences) {
        const int a = guestIndex.at(person);
        const int b = guestIndex.at(neighbor);
        pairHappiness[a, b] += happiness;
        pairHappiness[b, a] += happiness;
    }

    // A round table has no head: rotating everyone one seat left gives the same neighbors. So
    // guest 0 stays in seat 0 and only the others are permuted, 7! = 5040 seatings instead of 8!.
    auto seating = std::views::iota(0, guestCount) | std::ranges::to<std::vector>();
    int bestTable = std::numeric_limits<int>::min();
    int bestWithMe = std::numeric_limits<int>::min();
    do {
        // The (i + 1) % n wraps the last seat back round to the first.
        const auto neighborPairs = std::views::iota(0, guestCount) | std::views::transform([&](int i) {
            return pairHappiness[seating[i], seating[(i + 1) % guestCount]];
        });
        const int total = std::ranges::fold_left(neighborPairs, 0, std::plus{});

        bestTable = std::max(bestTable, total);
        // Part 2: I'm worth 0 to everyone, so sitting down between two guests just deletes their
        // pair from the table. The best place for me is between the pair that likes each other least.
        bestWithMe = std::max(bestWithMe, total - std::ranges::min(neighborPairs));
    } while (std::ranges::next_permutation(seating | std::views::drop(1)).found);

    std::println("part 1: {}", bestTable);
    std::println("part 2: {}", bestWithMe);
}
