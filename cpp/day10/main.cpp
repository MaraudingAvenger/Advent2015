#include <print>
#include <string>

#include "../common/utils.h"

int main() {
    auto text = utils::readFileToString(utils::getInputFilePath("inputs/day10.txt"));

    // generate a look-and-say sequence based on the input text
    auto lookAndSay = [](const std::string& input) {
        std::string result;
        for (size_t i = 0; i < input.size(); ) {
            char c = input[i];
            size_t count = 0;
            while (i < input.size() && input[i] == c) {
                ++count;
                ++i;
            }
            result += std::to_string(count) + c;
        }
        return result;
    };

    std::string result = text;

    // apply the look-and-say sequence 40 times
    for (int i = 0; i < 40; ++i) {
        result = lookAndSay(result);
    }

    // print the length of the resulting sequence
    std::println("part 1: {}", result.size());

    // apply the look-and-say sequence 50 times for part 2
    for (int i = 0; i < 10; ++i) {
        result = lookAndSay(result);
    }

    std::println("part 2: {}", result.size());

    return 0;
}
