#include <algorithm>
#include <functional>
#include <print>
#include <ranges>
#include <string_view>
#include <vector>

#include "../common/utils.h"

struct Frame {
    bool isObject = false;  // '{' vs '['
    bool hasRed = false;    // saw a "red" value directly in this object
    int sum = 0;            // numbers in this container (including children that survived)
};

constexpr bool isWordChar(char c) {
    return c == '-' || (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

// Splits JSON into tokens: runs of word characters ("red", "-12") stay together, and every
// other character ('{', '"', ':', ...) becomes a token of its own. Lazy: nothing is copied or
// allocated, each token is a string_view into `json`.
constexpr auto tokenize(std::string_view json) {
    return json
        | std::views::chunk_by([](char a, char b) { return isWordChar(a) && isWordChar(b); })
        | std::views::transform([](auto chunk) { return std::string_view(chunk); });
}

constexpr int sumIgnoringRed(std::string_view json) {
    // Start with a root frame so there is always a parent to add into: no empty() checks needed.
    std::vector<Frame> stack{Frame{}};

    for (std::string_view token : tokenize(json)) {
        switch (token.front()) {
            case '{':
                stack.push_back({.isObject = true});
                break;
            case '[':
                stack.push_back({.isObject = false});
                break;
            case '}':
            case ']': {
                const Frame child = stack.back();
                stack.pop_back();
                if (!(child.isObject && child.hasRed)) {
                    stack.back().sum += child.sum;
                }
                break;
            }
            default:
                if (token == "red") {
                    // Only objects are poisoned; [1,"red",5] still counts.
                    stack.back().hasRed |= stack.back().isObject;
                } else if (token.front() == '-' || (token.front() >= '0' && token.front() <= '9')) {
                    stack.back().sum += utils::parseInt(token);
                }
                break;
        }
    }
    return stack.front().sum;
}

// // The puzzle's examples, checked by the compiler: if any of these break, the build fails.
// static_assert(sumIgnoringRed(R"([1,2,3])") == 6);
// static_assert(sumIgnoringRed(R"([1,{"c":"red","b":2},3])") == 4);
// static_assert(sumIgnoringRed(R"({"d":"red","e":[1,2,3,4],"f":5})") == 0);
// static_assert(sumIgnoringRed(R"([1,"red",5])") == 6);
// static_assert(sumIgnoringRed(R"({"a":[-1,{"b":"red","c":[100]}],"d":2})") == 1);

int main() {
    const auto text = utils::readFileToString(utils::getInputFilePath("inputs/day12.json"));

    std::println("part 1: {}", std::ranges::fold_left(utils::extractInts(text), 0, std::plus{}));
    std::println("part 2: {}", sumIgnoringRed(text));
}
