#include <algorithm>
#include <print>

#include "../common/utils.h"

namespace {

int wrappingPaperArea(int l, int w, int h) {
    const int lw = l * w;
    const int wh = w * h;
    const int hl = h * l;
    return 2 * (lw + wh + hl) + std::min({lw, wh, hl});
}

int ribbonLength(int l, int w, int h) {
    const int smallestPerimeter = 2 * std::min({l + w, w + h, h + l});
    return smallestPerimeter + l * w * h;
}

}  // namespace

int main() {
    const auto lines = utils::readFileToLines(utils::getInputFilePath("inputs/day02.txt"));

    int totalArea = 0;
    int totalRibbon = 0;
    for (const auto& line : lines) {
        // "2x3x4"
        const auto [l, w, h] = utils::extractInts<3>(line);
        totalArea += wrappingPaperArea(l, w, h);
        totalRibbon += ribbonLength(l, w, h);
    }
    std::println("part 1: {}", totalArea);
    std::println("part 2: {}", totalRibbon);
}
