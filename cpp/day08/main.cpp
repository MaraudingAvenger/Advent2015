#include <format>
#include <print>
#include <stdexcept>
#include <string_view>

#include "../common/utils.h"

namespace {

// Number of characters the literal holds in memory once its escapes are decoded:
//   \"   -> 1 char (double quote)
//   \\   -> 1 char (backslash)
//   \xAB -> 1 char (hex code)
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

// The only chars that trigger are double quotes and backslashes when encoding.
std::size_t encodedLength(std::string_view literal) {
    std::size_t length = 2;  // account for the surrounding quotes
    for (char c : literal) {
        if (c == '"' || c == '\\') {
            length += 2;
        } else {
            length += 1;
        }
    }
    return length;
}

}  // namespace

int main() {
    const auto lines = utils::readFileToLines(utils::getInputFilePath("inputs/day08.txt"));

    std::size_t totalDifference = 0;
    for (const auto& line : lines) {
        totalDifference += line.size() - decodedLength(line);
    }
    std::println("part 1: {}", totalDifference);

    totalDifference = 0;
    for (const auto& line : lines) {
        totalDifference += encodedLength(line) - line.size();
    }
    std::println("part 2: {}", totalDifference);
}
