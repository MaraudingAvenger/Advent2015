#include <algorithm>
#include <format>
#include <functional>
#include <limits>
#include <print>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "../common/utils.h"

namespace {

struct Road {
    std::string_view from;
    std::string_view to;
    int distance;
};

// "Faerun to Tristram = 65"
Road parseRoad(std::string_view line) {
    const auto tokens = utils::splitString(line);
    if (tokens.size() != 5 || tokens[1] != "to" || tokens[3] != "=") {
        throw std::invalid_argument(std::format("malformed road: '{}'", line));
    }
    return {tokens[0], tokens[2], utils::parseInt(tokens[4])};
}

}  // namespace

int main() {
    const auto lines = utils::readFileToLines(utils::getInputFilePath("inputs/day09.txt"));
    const auto roads = lines | std::views::transform(parseRoad) | std::ranges::to<std::vector>();

    // Numbering the cities 0..n-1 lets distances live in a matrix and makes a route nothing more
    // than a permutation of those numbers.
    std::unordered_map<std::string_view, int> cityIndex;
    for (const auto& road : roads) {
        cityIndex.try_emplace(road.from, static_cast<int>(cityIndex.size()));
        cityIndex.try_emplace(road.to, static_cast<int>(cityIndex.size()));
    }
    const int cityCount = static_cast<int>(cityIndex.size());

    // A missing road would silently read as distance 0 and make a bogus route look shortest.
    if (std::cmp_not_equal(roads.size(), cityCount * (cityCount - 1) / 2)) {
        throw std::runtime_error("expected exactly one road between every pair of cities");
    }

    utils::Grid<int> distance(cityCount, cityCount);
    for (const auto& road : roads) {
        const int a = cityIndex.at(road.from);
        const int b = cityIndex.at(road.to);
        distance[a, b] = distance[b, a] = road.distance;  // roads run both ways
    }

    // Visiting every city once is a travelling-salesman problem, which has no efficient exact
    // algorithm; but with 8 cities there are only 8! = 40320 orders, so trying them all is instant.
    // Both parts score the same routes, so one pass over the permutations tracks both extremes.
    // (There is no permutations view in C++23, so next_permutation drives a plain loop.)
    const auto routeLength = [&](const std::vector<int>& route) {
        return std::ranges::fold_left(
            route | std::views::pairwise_transform([&](int a, int b) { return distance[a, b]; }),
            0, std::plus{});
    };

    auto route = std::views::iota(0, cityCount) | std::ranges::to<std::vector>();
    int shortest = std::numeric_limits<int>::max();
    int longest = std::numeric_limits<int>::min();
    do {
        const int length = routeLength(route);
        shortest = std::min(shortest, length);
        longest = std::max(longest, length);
    } while (std::ranges::next_permutation(route).found);

    std::println("part 1: {}", shortest);
    std::println("part 2: {}", longest);
}
