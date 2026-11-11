#include <functional>
#include <print>
#include <ranges>
#include <string>
#include <string_view>

#include "../common/utils.h"

namespace {

// "1211" -> runs {1}{2}{11} -> "one 1, one 2, two 1s" -> "111221".
// chunk_by with equal_to splits the string into runs of identical digits, which is exactly
// the grouping look-and-say describes.
std::string lookAndSay(std::string_view digits) {
    std::string result;
    result.reserve(digits.size() * 2);  // the sequence grows ~30% per step, so this rarely reallocates
    for (const auto run : digits | std::views::chunk_by(std::ranges::equal_to{})) {
        // Not std::format_to(back_inserter(result), ...): this runs millions of times, and
        // pushing through a back_inserter one char at a time made the whole day ~4x slower.
        result += std::to_string(run.size());
        result += run.front();
    }
    return result;
}

std::string lookAndSay(std::string digits, int times) {
    for (int i = 0; i < times; ++i) {
        digits = lookAndSay(digits);
    }
    return digits;
}

}  // namespace

int main() {
    const auto input = utils::readFileToString(utils::getInputFilePath("inputs/day10.txt"));

    // Trimmed so a trailing newline can't become part of the sequence.
    const auto after40 = lookAndSay(std::string{utils::trim(input)}, 40);
    std::println("part 1: {}", after40.size());

    const auto after50 = lookAndSay(after40, 10);
    std::println("part 2: {}", after50.size());
}
