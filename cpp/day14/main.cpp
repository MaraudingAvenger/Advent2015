#include <algorithm>
#include <cstdlib>
#include <print>
#include <ranges>
#include <string_view>
#include <vector>

#include "../common/utils.h"

constexpr int RaceSeconds = 2503;

struct Reindeer {
    std::string_view name;
    int speed;
    int duration;
    int rest;

    // t is 0-based: second t is the (t+1)th second of the race.
    constexpr bool flyingAt(int t) const { return t % (duration + rest) < duration; }

    // Closed form: whole fly/rest cycles, plus however much of the next fly window fits.
    constexpr int distanceAfter(int seconds) const {
        const auto [cycles, leftover] = std::div(seconds, duration + rest);
        return (cycles * duration + std::min(leftover, duration)) * speed;
    }
};

// "Comet can fly 14 km/s for 10 seconds, but then must rest for 127 seconds."
Reindeer parseReindeer(std::string_view line) {
    const auto [speed, duration, rest] = utils::extractInts<3>(line);
    return {line.substr(0, line.find(' ')), speed, duration, rest};
}

int main() {
    const auto lines = utils::readFileToLines(utils::getInputFilePath("inputs/day14.txt"));
    const auto reindeer = lines | std::views::transform(parseReindeer) | std::ranges::to<std::vector>();

    const auto maxDistance = std::ranges::max(reindeer | std::views::transform([](const Reindeer& r) {
        return r.distanceAfter(RaceSeconds);
    }));
    std::println("part 1: {}", maxDistance);

    // Part 2 has no closed form (the lead changes hands), so simulate second by second.
    // zip pairs each reindeer's running total with the reindeer itself; the structured binding
    // holds references, so `d += ...` and `++p` write straight back into the vectors.
    std::vector<int> distance(reindeer.size());
    std::vector<int> points(reindeer.size());
    for (int t = 0; t < RaceSeconds; ++t) {
        for (auto [d, r] : std::views::zip(distance, reindeer)) {
            if (r.flyingAt(t)) d += r.speed;
        }
        const int lead = std::ranges::max(distance);
        for (auto [p, d] : std::views::zip(points, distance)) {
            if (d == lead) ++p;
        }
    }
    std::println("part 2: {}", std::ranges::max(points));
}
