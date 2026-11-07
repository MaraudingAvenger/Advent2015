#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <format>
#include <optional>
#include <print>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>

#include "../common/utils.h"

namespace {

enum class Op { Assign, Not, And, Or, LShift, RShift };

// Either side of any gate can be a literal ("1 AND cx") or a wire ("lx -> a"),
// so an operand has to be able to hold both.
using Operand = std::variant<std::uint16_t, std::string>;

struct Gate {
    Op op;
    Operand lhs;
    std::optional<Operand> rhs;  // empty for the unary ops (Assign, Not)
};

// Each wire is driven by exactly one gate, so the target wire is a natural key.
using Circuit = std::unordered_map<std::string, Gate>;
using Cache = std::unordered_map<std::string, std::uint16_t>;

// Wire names are all lowercase letters, so a leading digit is enough to tell them apart.
// Parsing straight into uint16_t makes an out-of-range literal an error instead of a silent wrap.
Operand parseOperand(std::string_view s) {
    if (!s.empty() && std::isdigit(static_cast<unsigned char>(s.front()))) {
        return utils::parseInt<std::uint16_t>(s);
    }
    return std::string{s};
}

Op parseBinaryOp(std::string_view s) {
    static constexpr std::array<std::pair<std::string_view, Op>, 4> ops{{
        {"AND", Op::And},
        {"OR", Op::Or},
        {"LSHIFT", Op::LShift},
        {"RSHIFT", Op::RShift},
    }};

    const auto it = std::ranges::find(ops, s, &std::pair<std::string_view, Op>::first);
    if (it == ops.end()) {
        throw std::invalid_argument(std::format("unknown operator: '{}'", s));
    }
    return it->second;
}

// "123 -> x"  "lx -> a"  "NOT x -> h"  "x AND y -> d"
// The token count before "->" is enough to tell the three shapes apart.
std::pair<std::string, Gate> parseLine(std::string_view line) {
    const auto tokens = utils::splitString(line);
    if (tokens.size() < 3 || tokens[tokens.size() - 2] != "->") {
        throw std::invalid_argument(std::format("malformed line: '{}'", line));
    }

    std::string target{tokens.back()};
    const std::span<const std::string_view> expr(tokens.data(), tokens.size() - 2);

    switch (expr.size()) {
        case 1:
            return {std::move(target), {Op::Assign, parseOperand(expr[0]), std::nullopt}};
        case 2:
            if (expr[0] == "NOT") {
                return {std::move(target), {Op::Not, parseOperand(expr[1]), std::nullopt}};
            }
            break;
        case 3:
            return {std::move(target), {parseBinaryOp(expr[1]), parseOperand(expr[0]), parseOperand(expr[2])}};
    }
    throw std::invalid_argument(std::format("malformed expression: '{}'", line));
}

// Gates share inputs heavily, so the same wire is reachable through exponentially many paths.
// Without the cache, each path recomputes it and the evaluation never finishes in practice.
std::uint16_t evaluate(const Circuit& circuit, const std::string& wire, Cache& cache) {
    if (const auto hit = cache.find(wire); hit != cache.end()) {
        return hit->second;
    }

    const auto it = circuit.find(wire);
    if (it == circuit.end()) {
        throw std::invalid_argument(std::format("wire '{}' not found in circuit", wire));
    }
    const Gate& gate = it->second;

    auto signalOf = [&](const Operand& operand) -> std::uint16_t {
        if (const auto* literal = std::get_if<std::uint16_t>(&operand)) {
            return *literal;
        }
        return evaluate(circuit, std::get<std::string>(operand), cache);
    };

    // ~ and << promote uint16_t to int, so the casts keep the result within 16 bits.
    std::uint16_t result{};
    switch (gate.op) {
        case Op::Assign: result = signalOf(gate.lhs); break;
        case Op::Not:    result = static_cast<std::uint16_t>(~signalOf(gate.lhs)); break;
        case Op::And:    result = signalOf(gate.lhs) & signalOf(*gate.rhs); break;
        case Op::Or:     result = signalOf(gate.lhs) | signalOf(*gate.rhs); break;
        case Op::LShift: result = static_cast<std::uint16_t>(signalOf(gate.lhs) << signalOf(*gate.rhs)); break;
        case Op::RShift: result = signalOf(gate.lhs) >> signalOf(*gate.rhs); break;
    }

    cache.emplace(wire, result);
    return result;
}

std::string_view opName(Op op) {
    switch (op) {
        case Op::Assign: return "";
        case Op::Not:    return "NOT";
        case Op::And:    return "AND";
        case Op::Or:     return "OR";
        case Op::LShift: return "LSHIFT";
        case Op::RShift: return "RSHIFT";
    }
    std::unreachable();
}

// std::format rather than std::to_string, which has no overload for std::string.
std::string toString(const Operand& operand) {
    return std::visit([](const auto& value) { return std::format("{}", value); }, operand);
}

}  // namespace

// Debugging aid: std::println("{} -> {}", gate, wire) reproduces the original input line,
// which makes it easy to check the parser. Must live outside the anonymous namespace because
// it specialises a std template.
template <>
struct std::formatter<Gate> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    auto format(const Gate& gate, std::format_context& ctx) const {
        switch (gate.op) {
            case Op::Assign:
                return std::format_to(ctx.out(), "{}", toString(gate.lhs));
            case Op::Not:
                return std::format_to(ctx.out(), "NOT {}", toString(gate.lhs));
            default:
                return std::format_to(ctx.out(), "{} {} {}",
                                      toString(gate.lhs), opName(gate.op), toString(*gate.rhs));
        }
    }
};

int main() {
    const auto lines = utils::readFileToLines(utils::getInputFilePath("inputs/day07.txt"));
    const auto circuit = lines | std::views::transform(parseLine) | std::ranges::to<Circuit>();

    // ranges::to keeps the first gate for a duplicate key, which would silently drop the other.
    if (circuit.size() != lines.size()) {
        throw std::runtime_error("some wire is driven by more than one gate");
    }

    Cache cache;
    const std::uint16_t part1 = evaluate(circuit, "a", cache);
    std::println("part 1: {}", part1);

    // Seeding the cache overrides wire b: evaluate returns the cached value and never looks at
    // b's own gate, so the circuit itself doesn't need to change.
    Cache overridden{{"b", part1}};
    std::println("part 2: {}", evaluate(circuit, "a", overridden));
}
