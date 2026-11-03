#include <print>
#include <set>
#include <ranges>
#include <utility>

#include "../common/utils.h"

namespace {
constexpr std::pair<int, int> move(char c) {
    int dx = 0, dy = 0;
    switch (c) {
        case '^': dy = 1; break;
        case 'v': dy = -1; break;
        case '>': dx = 1; break;
        case '<': dx = -1; break;
        default: break;
    }
    return {dx, dy};
}

std::set<std::pair<int, int>> visit(std::string_view line, std::ptrdiff_t step) {
    std::set<std::pair<int, int>> visited;
    std::pair current{0, 0};
    visited.insert(current);
    for (char c : line | std::views::stride(step)) {
        current.first += move(c).first;
        current.second += move(c).second;
        visited.insert(current);
    }
    return visited;
}
}  // namespace

int main() {
    auto line = utils::readFileToString(utils::getInputFilePath("inputs/day03.txt"));

    auto part1 = visit(line, 1);
    std::println("part 1: {}", part1.size());

    auto part2 = visit(line, 2);
    std::println("part 2: {}", part2.size());
}
