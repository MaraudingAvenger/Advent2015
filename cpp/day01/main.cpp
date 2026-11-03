#include <print>
#include <numeric>
#include <ranges>
#include <vector>

#include "../common/utils.h"

int main() {
    auto line = utils::readFileToString(utils::getInputFilePath("inputs/day01.txt"));

    auto part1 = std::ranges::count(line, '(') - std::ranges::count(line, ')');
    std::println("part 1: {}", part1);

    auto floors = std::vector<int>{};
    int floor = 0;
    for (char c : line) {
        floor += (c == '(') ? 1 : (c == ')') ? -1 : 0;
        floors.push_back(floor);
    }
    auto it = std::ranges::find(floors, -1);
    std::println("part 2: {}", std::distance(floors.begin(), it) + 1);
}
