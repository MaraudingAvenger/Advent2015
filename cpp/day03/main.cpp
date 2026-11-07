#include <print>
#include <ranges>
#include <unordered_set>

#include "../common/utils.h"

namespace {

constexpr utils::Point move(char c) {
    switch (c) {
        case '^': return utils::Up;
        case 'v': return utils::Down;
        case '>': return utils::Right;
        case '<': return utils::Left;
        default:  return {};  // trailing newline or other noise: stay put
    }
}

// Takes any range of moves so part 2 can hand each deliverer its own strided slice.
std::unordered_set<utils::Point> visit(std::ranges::input_range auto&& moves) {
    utils::Point current;
    std::unordered_set<utils::Point> visited{current};
    for (const char c : moves) {
        current += move(c);
        visited.insert(current);
    }
    return visited;
}

}  // namespace

int main() {
    const auto line = utils::readFileToString(utils::getInputFilePath("inputs/day03.txt"));

    std::println("part 1: {}", visit(line).size());

    // Santa takes the even moves and Robo-Santa the odd ones; a house counts if either visits.
    auto houses = visit(line | std::views::stride(2));
    houses.merge(visit(line | std::views::drop(1) | std::views::stride(2)));
    std::println("part 2: {}", houses.size());
}
