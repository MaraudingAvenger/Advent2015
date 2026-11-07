#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <mdspan>
#include <ranges>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

// Everything here is `inline` so the header stays safe if a day ever grows a second .cpp file.

namespace utils {

// ---------------------------------------------------------------------------------------------
// Strings
//
// These return string_views into the input to avoid copying. The input therefore has to outlive
// the result, so each function also has a deleted overload for std::string temporaries: passing
// one is a compile error rather than a dangling view. Those overloads are constrained templates
// because a plain std::string&& overload would make string literals ambiguous (a literal
// converts to std::string as readily as to std::string_view); S only deduces to exactly
// std::string for an rvalue, so lvalues and literals still reach the string_view version.
// ---------------------------------------------------------------------------------------------

namespace detail {

constexpr bool isDigit(char c) { return c >= '0' && c <= '9'; }
inline bool isSpace(char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; }

// Turns a contiguous subrange (what split/chunk_by yield) back into a string_view.
inline constexpr auto toStringView = [](auto&& range) { return std::string_view(range); };

}  // namespace detail

// Splits on every delimiter and keeps empty fields, so "a,,b," gives {"a", "", "b", ""}.
// Like Python's str.split(sep), a field count is reliable even when fields can be blank.
// One difference: an empty input gives no fields at all, where Python gives {""}.
inline std::vector<std::string_view> splitString(std::string_view str, std::string_view delimiter) {
    return str | std::views::split(delimiter) | std::views::transform(detail::toStringView)
        | std::ranges::to<std::vector>();
}

inline std::vector<std::string_view> splitString(std::string_view str, char delimiter) {
    return str | std::views::split(delimiter) | std::views::transform(detail::toStringView)
        | std::ranges::to<std::vector>();
}

// Splits on runs of whitespace and drops empty fields, like Python's str.split(). Stray '\r'
// or double spaces therefore never produce phantom tokens.
inline std::vector<std::string_view> splitString(std::string_view str) {
    return str
        | std::views::chunk_by([](char a, char b) { return detail::isSpace(a) == detail::isSpace(b); })
        | std::views::filter([](auto chunk) { return !detail::isSpace(chunk.front()); })
        | std::views::transform(detail::toStringView)
        | std::ranges::to<std::vector>();
}

template <std::same_as<std::string> S>
std::vector<std::string_view> splitString(S&&, std::string_view) = delete;
template <std::same_as<std::string> S>
std::vector<std::string_view> splitString(S&&, char) = delete;
template <std::same_as<std::string> S>
std::vector<std::string_view> splitString(S&&) = delete;

inline constexpr std::string_view trim(std::string_view str, std::string_view chars = " \t\r\n") {
    const auto first = str.find_first_not_of(chars);
    if (first == std::string_view::npos) {
        return {};
    }
    return str.substr(first, str.find_last_not_of(chars) - first + 1);
}

template <std::same_as<std::string> S>
std::string_view trim(S&&, std::string_view = {}) = delete;

// Accepts any range of string-like elements (vector<string>, vector<string_view>, a view...).
template <std::ranges::input_range R>
std::string joinStrings(R&& strings, std::string_view delimiter) {
    return std::forward<R>(strings) | std::views::join_with(delimiter) | std::ranges::to<std::string>();
}

template <std::ranges::input_range R>
std::string joinStrings(R&& strings, char delimiter) {
    return joinStrings(std::forward<R>(strings), std::string_view(&delimiter, 1));
}

[[nodiscard]] inline std::string replace(std::string_view str, char from, char to) {
    std::string result{str};
    std::ranges::replace(result, from, to);
    return result;
}

[[nodiscard]] inline std::string replace(std::string_view str, std::string_view from, std::string_view to) {
    if (from.empty()) {
        return std::string{str};
    }
    std::string result;
    result.reserve(str.size());
    for (auto pos = str.find(from); pos != std::string_view::npos; pos = str.find(from)) {
        result.append(str.substr(0, pos));
        result.append(to);
        str.remove_prefix(pos + from.size());
    }
    result.append(str);
    return result;
}

// ---------------------------------------------------------------------------------------------
// Numbers
//
// Strict on purpose: the whole string must be the number. std::stoi/stod silently accept
// "12abc" and leading whitespace, which turns parser bugs into wrong answers instead of errors.
// from_chars also rejects a leading '+'.
// ---------------------------------------------------------------------------------------------

namespace detail {

template <typename T>
constexpr T parseNumber(std::string_view s) {
    T value{};
    const auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec != std::errc{} || ptr != s.data() + s.size()) {
        throw std::invalid_argument(std::format("cannot parse '{}' as a number", s));
    }
    return value;
}

}  // namespace detail

// Range-checked against T, so parseInt<std::uint16_t>("70000") throws rather than wrapping.
template <std::integral T = int>
constexpr T parseInt(std::string_view s) {
    return detail::parseNumber<T>(s);
}

template <std::floating_point T = double>
T parseFloat(std::string_view s) {
    return detail::parseNumber<T>(s);
}

// Pulls every integer out of a line, ignoring whatever text surrounds them:
//   "toggle 461,550 through 564,900" -> {461, 550, 564, 900}
//   "2x3x4"                          -> {2, 3, 4}
// chunk_by groups each run of digits (plus a leading '-' for signed T) into one chunk; chunks
// that don't end in a digit are the surrounding text and get filtered out. Like the regex
// -?\d+, "1-2" reads as {1, -2}, so use an unsigned T when '-' is only ever a separator.
template <std::integral T = int>
std::vector<T> extractInts(std::string_view s) {
    return s
        | std::views::chunk_by([](char a, char b) {
              return detail::isDigit(b) && (detail::isDigit(a) || (std::is_signed_v<T> && a == '-'));
          })
        | std::views::filter([](auto chunk) { return detail::isDigit(chunk.back()); })
        | std::views::transform([](auto chunk) { return parseInt<T>(std::string_view(chunk)); })
        | std::ranges::to<std::vector>();
}

// Fixed-count version for lines with a known shape, so the result can be destructured:
//   const auto [x1, y1, x2, y2] = utils::extractInts<4>(line);
// Throws if the line doesn't contain exactly N integers, which catches a misread format early.
template <std::size_t N, std::integral T = int>
std::array<T, N> extractInts(std::string_view s) {
    const auto values = extractInts<T>(s);
    if (values.size() != N) {
        throw std::invalid_argument(std::format("expected {} integers but found {} in '{}'", N, values.size(), s));
    }
    std::array<T, N> result{};
    std::ranges::copy(values, result.begin());
    return result;
}

// ---------------------------------------------------------------------------------------------
// 2D geometry
//
// Screen convention: x grows rightwards, y grows downwards, so row 0 is the first input line.
// Puzzles that describe "up" as +y (2015 day 3) can still use Point; they just shouldn't use the
// named directions below.
// ---------------------------------------------------------------------------------------------

struct Point {
    int x = 0;
    int y = 0;

    constexpr Point operator+(Point other) const { return {x + other.x, y + other.y}; }
    constexpr Point operator-(Point other) const { return {x - other.x, y - other.y}; }
    constexpr Point operator*(int k) const { return {x * k, y * k}; }
    constexpr Point& operator+=(Point other) { return *this = *this + other; }
    constexpr Point& operator-=(Point other) { return *this = *this - other; }

    // Ordering too, not just equality, so Points also work in std::set/std::map and sort.
    constexpr auto operator<=>(const Point&) const = default;
};

constexpr int manhattanDistance(Point a, Point b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

inline constexpr Point Up{0, -1};
inline constexpr Point Down{0, 1};
inline constexpr Point Left{-1, 0};
inline constexpr Point Right{1, 0};

// Clockwise from Up, so turning right is index + 1 and turning left is index + 3 (mod 4).
inline constexpr std::array<Point, 4> Neighbors4{Up, Right, Down, Left};
inline constexpr std::array<Point, 8> Neighbors8{
    Up, Up + Right, Right, Down + Right, Down, Down + Left, Left, Up + Left,
};

// A dense 2D grid stored row-major in one vector, viewed through std::mdspan.
// A vector rather than std::array because sizes come from the input at runtime, and because
// large grids (1000x1000 in 2015 day 6) would overflow the stack as a std::array.
// Indexing is x-first to match Point; view() exposes the raw mdspan, which is [row, col].
template <typename T>
class Grid {
    // vector<bool> packs bits and has no data(), so mdspan can't view it.
    static_assert(!std::same_as<T, bool>, "Grid<bool> is unsupported; use Grid<char> or Grid<std::uint8_t>");

public:
    Grid(int width, int height, const T& fill = T{})
        : width_(width), height_(height), cells_(area(width, height), fill) {}

    // Builds a grid from input lines, converting each character with `convert`:
    //   auto chars  = Grid<char>::fromLines(lines);
    //   auto digits = Grid<int>::fromLines(lines, [](char c) { return c - '0'; });
    template <std::ranges::input_range Lines, typename Convert = std::identity>
    static Grid fromLines(const Lines& lines, Convert convert = {}) {
        const int height = static_cast<int>(std::ranges::distance(lines));
        const int width = height == 0 ? 0 : static_cast<int>(std::string_view(*std::ranges::begin(lines)).size());
        Grid grid(width, height);
        for (const auto [y, line] : lines | std::views::enumerate) {
            const std::string_view row(line);
            if (std::cmp_not_equal(row.size(), width)) {
                throw std::invalid_argument(std::format("grid row {} has width {}, expected {}", y, row.size(), width));
            }
            for (const auto [x, c] : row | std::views::enumerate) {
                grid[static_cast<int>(x), static_cast<int>(y)] = static_cast<T>(std::invoke(convert, c));
            }
        }
        return grid;
    }

    int width() const { return width_; }
    int height() const { return height_; }

    bool contains(Point p) const { return p.x >= 0 && p.x < width_ && p.y >= 0 && p.y < height_; }

    // Deducing `this` gives const and non-const access from one definition: a const Grid
    // yields const T&, a mutable one T&.
    decltype(auto) operator[](this auto& self, Point p) { return self.view()[p.y, p.x]; }
    decltype(auto) operator[](this auto& self, int x, int y) { return self.view()[y, x]; }

    auto view(this auto& self) { return std::mdspan(self.cells_.data(), self.height_, self.width_); }

    // Flat access for whole-grid algorithms, e.g. std::ranges::count(grid.cells(), '#').
    auto cells(this auto& self) { return std::span(self.cells_); }

    // Every coordinate in row-major order, e.g. `for (Point p : grid.points())`.
    auto points() const {
        return std::views::cartesian_product(std::views::iota(0, height_), std::views::iota(0, width_))
            | std::views::transform([](auto yx) {
                  const auto [y, x] = yx;
                  return Point{x, y};
              });
    }

    // In-bounds neighbors of p, so edge handling doesn't have to be repeated at every call site.
    // `directions` is taken by value so the returned view owns it and can't dangle.
    template <std::size_t N = 4>
    auto neighbors(Point p, std::array<Point, N> directions = Neighbors4) const {
        return std::move(directions)
            | std::views::transform([p](Point d) { return p + d; })
            | std::views::filter([this](Point q) { return contains(q); });
    }

private:
    // Checked before the vector is sized: a negative dimension would otherwise convert to a huge
    // size_t and fail as bad_alloc instead of a clear error.
    static std::size_t area(int width, int height) {
        if (width < 0 || height < 0) {
            throw std::invalid_argument(std::format("invalid grid size {}x{}", width, height));
        }
        return static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    }

    int width_;
    int height_;
    std::vector<T> cells_;
};

// ---------------------------------------------------------------------------------------------
// Files
// ---------------------------------------------------------------------------------------------

// Walks up from the working directory, so a day runs the same from cpp/, cpp/build/ or the repo root.
inline std::filesystem::path getInputFilePath(const std::filesystem::path& inputFile) {
    for (auto dir = std::filesystem::current_path();; dir = dir.parent_path()) {
        if (const auto candidate = dir / inputFile; std::filesystem::exists(candidate)) {
            return candidate;
        }
        if (dir == dir.parent_path()) {
            throw std::runtime_error(std::format("{} not found in current or parent directories", inputFile.string()));
        }
    }
}

namespace detail {

// A missing or unreadable file should fail loudly, not look like an empty puzzle input.
inline std::ifstream openFile(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);
    if (!file) {
        throw std::runtime_error(std::format("cannot open '{}'", filePath.string()));
    }
    return file;
}

}  // namespace detail

inline std::vector<std::string> readFileToLines(const std::filesystem::path& filePath) {
    auto file = detail::openFile(filePath);
    std::vector<std::string> lines;
    for (std::string line; std::getline(file, line);) {
        // Text mode only strips '\r' on Windows; this keeps CRLF inputs working on Linux too.
        if (line.ends_with('\r')) {
            line.pop_back();
        }
        lines.push_back(std::move(line));
    }
    return lines;
}

inline std::string readFileToString(const std::filesystem::path& filePath) {
    auto file = detail::openFile(filePath);
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

}  // namespace utils

// Lets Point go straight into std::unordered_set / std::unordered_map.
template <>
struct std::hash<utils::Point> {
    std::size_t operator()(const utils::Point& p) const noexcept {
        const auto packed = (std::uint64_t{static_cast<std::uint32_t>(p.x)} << 32) | static_cast<std::uint32_t>(p.y);
        return std::hash<std::uint64_t>{}(packed);
    }
};

// std::println("{}", point) prints "(x, y)".
template <>
struct std::formatter<utils::Point> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    auto format(const utils::Point& p, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "({}, {})", p.x, p.y);
    }
};
