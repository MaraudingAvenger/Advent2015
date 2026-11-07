#include <algorithm>
#include <array>
#include <format>
#include <functional>
#include <print>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "../common/utils.h"

namespace {

constexpr int GridSize = 1000;

enum class Action { TurnOn, TurnOff, Toggle };

// Corners are inclusive on both ends.
struct Instruction {
    Action action;
    utils::Point from;
    utils::Point to;
};

// "turn on 599,989 through 806,993"
// "turn off 370,39 through 425,839"
// "toggle 461,550 through 564,900"
Instruction parseInstruction(std::string_view line) {
    // Matching on the full prefix sidesteps "turn on" being two words but "toggle" being one.
    static constexpr std::array<std::pair<std::string_view, Action>, 3> prefixes{{
        {"turn on ", Action::TurnOn},
        {"turn off ", Action::TurnOff},
        {"toggle ", Action::Toggle},
    }};

    const auto it = std::ranges::find_if(prefixes, [line](const auto& p) { return line.starts_with(p.first); });
    if (it == prefixes.end()) {
        throw std::invalid_argument(std::format("unknown command: '{}'", line));
    }
    const auto [x1, y1, x2, y2] = utils::extractInts<4>(line);
    return {it->second, {x1, y1}, {x2, y2}};
}

// Both parts walk the same rectangles and differ only in what an action does to one light,
// so the walk is shared and the per-light rule is passed in.
template <typename Rule>
utils::Grid<int> applyInstructions(std::span<const Instruction> instructions, Rule rule) {
    // int rather than a byte: part 2 brightness only goes up, and a uint8_t would wrap at 255.
    utils::Grid<int> lights(GridSize, GridSize);

    for (const auto& [action, from, to] : instructions) {
        const auto rows = std::views::iota(from.y, to.y + 1);
        const auto cols = std::views::iota(from.x, to.x + 1);
        for (const auto [y, x] : std::views::cartesian_product(rows, cols)) {
            rule(action, lights[x, y]);
        }
    }
    return lights;
}

}  // namespace

int main() {
    const auto instructions = utils::readFileToLines(utils::getInputFilePath("inputs/day06.txt"))
        | std::views::transform(parseInstruction)
        | std::ranges::to<std::vector>();

    const auto onOff = applyInstructions(instructions, [](Action action, int& light) {
        switch (action) {
            case Action::TurnOn:  light = 1; break;
            case Action::TurnOff: light = 0; break;
            case Action::Toggle:  light ^= 1; break;
        }
    });
    std::println("part 1: {}", std::ranges::count(onOff.cells(), 1));

    const auto brightness = applyInstructions(instructions, [](Action action, int& light) {
        switch (action) {
            case Action::TurnOn:  light += 1; break;
            case Action::TurnOff: light = std::max(light - 1, 0); break;
            case Action::Toggle:  light += 2; break;
        }
    });
    std::println("part 2: {}", std::ranges::fold_left(brightness.cells(), 0LL, std::plus{}));
}
