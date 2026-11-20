#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <optional>
#include <print>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "../common/utils.h"

// How part 2's "outdated retroencabulator" reading should be compared against what we remember.
enum class Match { Exact, Fewer, Greater };

struct TapeEntry {
    std::string_view compound;
    int count;
    Match match;
};

// The MFCSAM ticker tape. This one table is the single source of truth for compounds: each one's
// name, its reading, and how part 2 interprets that reading. A compound's position here is also
// its slot in Sue::compounds, so there's no separate enum or name list to keep in sync.
constexpr std::array Tape{
    TapeEntry{"children", 3, Match::Exact},
    TapeEntry{"cats", 7, Match::Greater},
    TapeEntry{"samoyeds", 2, Match::Exact},
    TapeEntry{"pomeranians", 3, Match::Fewer},
    TapeEntry{"akitas", 0, Match::Exact},
    TapeEntry{"vizslas", 0, Match::Exact},
    TapeEntry{"goldfish", 5, Match::Fewer},
    TapeEntry{"trees", 3, Match::Greater},
    TapeEntry{"cars", 2, Match::Exact},
    TapeEntry{"perfumes", 1, Match::Exact},
};

// The projection (&TapeEntry::compound) makes find compare the name field, not the whole entry.
constexpr std::size_t compoundIndex(std::string_view name) {
    const auto it = std::ranges::find(Tape, name, &TapeEntry::compound);
    if (it == Tape.end()) {
        throw std::invalid_argument(std::format("unknown compound '{}'", name));
    }
    return static_cast<std::size_t>(it - Tape.begin());
}

static_assert(compoundIndex("trees") == 7);  // evaluated at compile time

struct Sue {
    int number;
    std::array<std::optional<int>, Tape.size()> compounds;
};

// "Sue 1: goldfish: 9, cars: 0, samoyeds: 9"
// Whitespace tokens are {"Sue", "1:", "goldfish:", "9,", "cars:", "0,", ...}. After the first two,
// they come in (name, count) pairs, which views::chunk(2) groups; trim strips the punctuation.
Sue parseSue(std::string_view line) {
    const auto tokens = utils::splitString(line);
    if (tokens.size() < 2 || tokens.size() % 2 != 0 || tokens[0] != "Sue") {
        throw std::invalid_argument(std::format("malformed Sue: '{}'", line));
    }

    Sue sue{.number = utils::parseInt(utils::trim(tokens[1], ":")), .compounds = {}};
    for (const auto pair : tokens | std::views::drop(2) | std::views::chunk(2)) {
        auto& slot = sue.compounds[compoundIndex(utils::trim(pair[0], ":"))];
        if (slot) {
            throw std::invalid_argument(std::format("duplicate compound '{}' in '{}'", pair[0], line));
        }
        slot = utils::parseInt(utils::trim(pair[1], ","));
    }
    return sue;
}

// A Sue is a candidate if every compound we remember for her agrees with the tape. Compounds we
// don't remember can't rule her out. zip walks the tape and her slots side by side.
bool matches(const Sue& sue, auto agrees) {
    return std::ranges::all_of(std::views::zip(Tape, sue.compounds), [&](const auto& pair) {
        const auto& [entry, remembered] = pair;
        return !remembered || agrees(entry, *remembered);
    });
}

// The puzzle promises exactly one match; anything else means a parsing or logic bug, so say so.
int findSue(std::span<const Sue> sues, auto agrees) {
    const auto found = sues
        | std::views::filter([&](const Sue& sue) { return matches(sue, agrees); })
        | std::views::transform(&Sue::number)
        | std::ranges::to<std::vector>();
    if (found.size() != 1) {
        throw std::runtime_error(std::format("expected exactly one matching Sue, found {}", found));
    }
    return found.front();
}

int main() {
    const auto lines = utils::readFileToLines(utils::getInputFilePath("inputs/day16.txt"));
    const auto sues = lines | std::views::transform(parseSue) | std::ranges::to<std::vector>();

    constexpr auto exactly = [](const TapeEntry& entry, int remembered) { return remembered == entry.count; };

    constexpr auto retroencabulated = [](const TapeEntry& entry, int remembered) {
        switch (entry.match) {
            case Match::Exact: return remembered == entry.count;
            case Match::Fewer: return remembered < entry.count;
            case Match::Greater: return remembered > entry.count;
        }
        std::unreachable();  // C++23: tells the compiler every enum value was handled above
    };

    std::println("part 1: {}", findSue(sues, exactly));
    std::println("part 2: {}", findSue(sues, retroencabulated));
}
