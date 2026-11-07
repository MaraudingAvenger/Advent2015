#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <print>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

#include "../common/utils.h"

namespace {

using Digest = std::array<unsigned char, 16>;

// One 64-byte block, minus the mandatory 0x80 pad byte and the 8-byte length field.
constexpr std::size_t MaxMessageLength = 55;

constexpr std::array<std::uint32_t, 64> RoundConstants{
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
};

constexpr std::array<int, 64> RotateAmounts{
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
    5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21,
};

// Hand-rolled rather than OpenSSL: EVP's per-digest setup touches shared state, which capped
// multithreaded scaling at ~4 threads. Every message here is "<key><number>", which always
// fits in one block, so the multi-block machinery is skipped entirely. constexpr so the
// static_asserts below can check it against known digests at compile time.
constexpr Digest md5(std::string_view message) {
    if (message.size() > MaxMessageLength) {
        throw std::length_error("md5: message must fit in a single block");
    }

    // MD5 reads the block as little-endian 32-bit words.
    std::array<std::uint32_t, 16> words{};
    for (std::size_t i = 0; i < message.size(); ++i) {
        words[i / 4] |= std::uint32_t{static_cast<unsigned char>(message[i])} << (8 * (i % 4));
    }
    words[message.size() / 4] |= 0x80u << (8 * (message.size() % 4));
    words[14] = static_cast<std::uint32_t>(message.size() * 8);  // length in bits

    constexpr std::array<std::uint32_t, 4> initial{0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};
    auto [a, b, c, d] = initial;

    for (std::size_t i = 0; i < 64; ++i) {
        std::uint32_t f = 0;
        std::size_t g = 0;
        switch (i / 16) {
            case 0:  f = (b & c) | (~b & d); g = i;                break;
            case 1:  f = (d & b) | (~d & c); g = (5 * i + 1) % 16; break;
            case 2:  f = b ^ c ^ d;          g = (3 * i + 5) % 16; break;
            default: f = c ^ (b | ~d);       g = (7 * i) % 16;     break;
        }
        f += a + RoundConstants[i] + words[g];
        a = d;
        d = c;
        c = b;
        b += std::rotl(f, RotateAmounts[i]);
    }

    const std::array<std::uint32_t, 4> state{initial[0] + a, initial[1] + b, initial[2] + c, initial[3] + d};
    Digest digest{};
    for (std::size_t i = 0; i < digest.size(); ++i) {
        digest[i] = static_cast<unsigned char>(state[i / 4] >> (8 * (i % 4)));
    }
    return digest;
}

// Counts leading zero hex digits straight from the bytes, so the digest never has to be
// formatted as a string. Each byte is two hex digits, hence countl_zero / 4.
constexpr int leadingZeroHexDigits(const Digest& digest) {
    int count = 0;
    for (const unsigned char byte : digest) {
        const int zeros = std::countl_zero(byte) / 4;
        count += zeros;
        if (zeros < 2) {
            break;
        }
    }
    return count;
}

consteval Digest fromHex(std::string_view hex) {
    auto nibble = [](char ch) { return ch <= '9' ? ch - '0' : ch - 'a' + 10; };
    Digest digest{};
    for (std::size_t i = 0; i < digest.size(); ++i) {
        digest[i] = static_cast<unsigned char>(nibble(hex[2 * i]) << 4 | nibble(hex[2 * i + 1]));
    }
    return digest;
}

// A hand-written hash is easy to get subtly wrong, so these make a mistake a build error
// instead of a wrong answer.
static_assert(md5("") == fromHex("d41d8cd98f00b204e9800998ecf8427e"));
static_assert(md5("abc") == fromHex("900150983cd24fb0d6963f7d28e17f72"));
static_assert(md5("The quick brown fox jumps over the lazy dog") == fromHex("9e107d9d372bb6826bd81d3542a419d6"));
// The puzzle's own examples.
static_assert(leadingZeroHexDigits(md5("abcdef609043")) >= 5);
static_assert(leadingZeroHexDigits(md5("pqrstuv1048970")) >= 5);

// std::atomic::fetch_min is C++26, so do it with a CAS loop.
void storeMin(std::atomic<int>& target, int value) {
    int current = target.load(std::memory_order_relaxed);
    while (value < current && !target.compare_exchange_weak(current, value, std::memory_order_relaxed)) {
    }
}

struct Answers {
    int fiveZeros;
    int sixZeros;
};

// Candidates are independent, so threads claim blocks of numbers from a shared counter.
// Finding a match doesn't let us stop immediately: a smaller match may still be sitting in a
// block another thread hasn't finished. So threads only stop claiming blocks that start past
// the best answer so far, and every number below it still gets checked.
Answers search(std::string_view key) {
    if (key.size() + std::numeric_limits<int>::digits10 + 1 > MaxMessageLength) {
        throw std::invalid_argument("key too long to hash as a single block");
    }

    constexpr int BlockSize = 4096;  // big enough that the shared counter isn't contended
    constexpr int NotFound = std::numeric_limits<int>::max();

    std::atomic<int> nextBlock{0};
    std::atomic<int> fiveZeros{NotFound};
    std::atomic<int> sixZeros{NotFound};

    {
        std::vector<std::jthread> threads;
        const unsigned threadCount = std::max(1u, std::thread::hardware_concurrency());
        for (unsigned t = 0; t < threadCount; ++t) {
            threads.emplace_back([&] {
                // The key is written once; only the number's digits change per hash.
                std::array<char, MaxMessageLength> buffer{};
                std::ranges::copy(key, buffer.begin());
                char* const digits = buffer.data() + key.size();

                while (true) {
                    const int start = nextBlock.fetch_add(BlockSize, std::memory_order_relaxed);
                    if (start > sixZeros.load(std::memory_order_relaxed)) {
                        return;
                    }
                    for (int n = start; n < start + BlockSize; ++n) {
                        // Can't fail: the length check above guarantees room for any int.
                        const char* const end = std::to_chars(digits, buffer.data() + buffer.size(), n).ptr;
                        const int zeros = leadingZeroHexDigits(md5({buffer.data(), end}));
                        if (zeros >= 5) {
                            storeMin(fiveZeros, n);
                        }
                        if (zeros >= 6) {
                            storeMin(sixZeros, n);
                        }
                    }
                }
            });
        }
    }  // jthreads join on destruction

    // Any six-zero hash also has five zeros, and everything below the six-zero answer was
    // checked, so the five-zero answer is final too.
    return {fiveZeros.load(), sixZeros.load()};
}

}  // namespace

int main() {
    const auto key = utils::trim(utils::readFileToString(utils::getInputFilePath("inputs/day04.txt")));
    const auto [part1, part2] = search(key);
    std::println("part 1: {}", part1);
    std::println("part 2: {}", part2);
}
