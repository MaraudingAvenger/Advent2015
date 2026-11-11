#include <algorithm>
#include <format>
#include <functional>
#include <print>
#include <ranges>
#include <stdexcept>
#include <string_view>

#include "../common/utils.h"

namespace {

// Number of characters the literal holds in memory once its escapes are decoded:
//   \"   -> 1 char (double quote)
//   \\   -> 1 char (backslash)
//   \xAB -> 1 char (hex code)
// A plain index loop because each escape consumes a different number of characters.
std::size_t decodedLength(std::string_view literal) {
    if (literal.size() < 2 || !literal.starts_with('"') || !literal.ends_with('"')) {
        throw std::invalid_argument(std::format("not a quoted literal: {}", literal));
    }
    const auto body = literal.substr(1, literal.size() - 2);

    std::size_t length = 0;
    for (std::size_t i = 0; i < body.size(); ++length) {
        if (body[i] != '\\') {
            i += 1;
        } else if (i + 1 < body.size() && body[i + 1] == 'x') {
            i += 4;
        } else {
            i += 2;
        }
    }
    return length;
}

// Encoding adds surrounding quotes, and a backslash in front of every quote or backslash.
std::size_t encodedLength(std::string_view literal) {
    const auto needsEscape = std::ranges::count_if(literal, [](char c) { return c == '"' || c == '\\'; });
    return literal.size() + static_cast<std::size_t>(needsEscape) + 2;
}

}  // namespace

int main() {
    const auto lines = utils::readFileToLines(utils::getInputFilePath("inputs/day08.txt"));

    // fold_left's result type follows the initial value, so 0uz keeps the sums in size_t.
    const auto codeMinusMemory = lines
        | std::views::transform([](std::string_view s) { return s.size() - decodedLength(s); });
    std::println("part 1: {}", std::ranges::fold_left(codeMinusMemory, 0uz, std::plus{}));

    const auto encodedMinusCode = lines
        | std::views::transform([](std::string_view s) { return encodedLength(s) - s.size(); });
    std::println("part 2: {}", std::ranges::fold_left(encodedMinusCode, 0uz, std::plus{}));
}
