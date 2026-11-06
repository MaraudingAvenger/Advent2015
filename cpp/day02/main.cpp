#include <print>
#include <algorithm>
#include <ranges>
#include <charconv>

#include "../common/utils.h"

int wrappingPaperArea(int l, int w, int h) {
    int lw = l * w;
    int wh = w * h;
    int hl = h * l;
    return 2 * (lw + wh + hl) + std::min({lw, wh, hl});
}

int ribbonLength(int l, int w, int h) {
    int perim1 = 2 * (l + w);
    int perim2 = 2 * (w + h);
    int perim3 = 2 * (h + l);
    int smallestPerim = std::min({perim1, perim2, perim3});
    int volume = l * w * h;
    return smallestPerim + volume;
}

int main() {
    auto lines = utils::readFileToLines(utils::getInputFilePath("inputs/day02.txt"));

    auto parseLine = [](std::string_view sv) -> std::tuple<int, int, int> {
        int l, w, h;
        std::from_chars(sv.data(), sv.data() + sv.size(), l);
        sv.remove_prefix(sv.find('x') + 1);
        std::from_chars(sv.data(), sv.data() + sv.size(), w);
        sv.remove_prefix(sv.find('x') + 1);
        std::from_chars(sv.data(), sv.data() + sv.size(), h);
        return {l, w, h};
    };

    int totalArea = 0;
    for (auto& line : lines) {
        auto [l, w, h] = parseLine(line);
        totalArea += wrappingPaperArea(l, w, h);
    }
    std::println("part 1: {}", totalArea);

    int totalRibbon = 0;
    for (auto& line : lines) {
        auto [l, w, h] = parseLine(line);
        totalRibbon += ribbonLength(l, w, h);
    }
    std::println("part 2: {}", totalRibbon);
}
