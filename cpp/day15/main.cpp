#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <generator>
#include <print>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

#include "../common/utils.h"

constexpr int Teaspoons = 100;
constexpr int CalorieTarget = 500;

struct Ingredient {
    std::string_view name;
    int capacity;
    int durability;
    int flavor;
    int texture;
    int calories;
};

constexpr Ingredient parseIngredient(std::string_view line) {
    const auto [capacity, durability, flavor, texture, calories] = utils::extractInts<5>(line);
    return {line.substr(0, line.find(':')), capacity, durability, flavor, texture, calories};
}

// The properties that count toward the score; calories only matter for part 2's filter.
// A pointer-to-member names a field without naming an object, so one function can total any of them.
constexpr std::array ScoredProperties{
    &Ingredient::capacity, &Ingredient::durability, &Ingredient::flavor, &Ingredient::texture,
};

// Yields every way to split `teaspoons` among amounts.size() ingredients (amounts must be non-empty).
// Each split is written into `amounts` in place and yielded as a read-only view of it, so nothing is
// allocated per split. The last ingredient always takes whatever is left, so every split is exact.
// elements_of passes the recursive generator's yields straight through to our caller.
std::generator<std::span<const int>> splits(std::span<int> amounts, int teaspoons, std::size_t i = 0) {
    if (i + 1 == amounts.size()) {
        amounts[i] = teaspoons;
        co_yield amounts;
        co_return;
    }
    for (int n = 0; n <= teaspoons; ++n) {
        amounts[i] = n;
        co_yield std::ranges::elements_of(splits(amounts, teaspoons - n, i + 1));
    }
}

// Sum of amount * property over all ingredients. zip_transform walks both ranges in lockstep, and
// std::invoke applies the pointer-to-member, so `property` can be any int field of Ingredient.
int total(std::span<const Ingredient> ingredients, std::span<const int> amounts, int Ingredient::*property) {
    return std::ranges::fold_left(
        std::views::zip_transform([property](const Ingredient& ing, int n) { return n * std::invoke(property, ing); },
                                  ingredients, amounts),
        0, std::plus{});
}

// Product of the scored totals, each floored at zero. 64-bit because four totals of up to
// 100 * (max property) can overflow an int in the worst case.
std::int64_t score(std::span<const Ingredient> ingredients, std::span<const int> amounts) {
    return std::ranges::fold_left(ScoredProperties | std::views::transform([&](auto property) {
                                      return std::int64_t{std::max(0, total(ingredients, amounts, property))};
                                  }),
                                  std::int64_t{1}, std::multiplies{});
}

int main() {
    const auto lines = utils::readFileToLines(utils::getInputFilePath("inputs/day15.txt"));
    const auto ingredients = lines
        | std::views::transform(parseIngredient)
        | std::ranges::to<std::vector>();

    // One pass answers both parts: every split is a candidate for part 1, and the 500-calorie
    // ones are also candidates for part 2.
    std::vector<int> amounts(ingredients.size());
    std::int64_t best = 0;
    std::int64_t bestAtTarget = 0;
    for (const auto split : splits(amounts, Teaspoons)) {
        const auto s = score(ingredients, split);
        best = std::max(best, s);
        if (total(ingredients, split, &Ingredient::calories) == CalorieTarget) {
            bestAtTarget = std::max(bestAtTarget, s);
        }
    }

    std::println("part 1: {}", best);
    std::println("part 2: {}", bestAtTarget);
}
