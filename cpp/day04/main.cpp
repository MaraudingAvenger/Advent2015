#include <print>
#include <iomanip>
#include <sstream>
#include <openssl/evp.h>

#include "../common/utils.h"

std::string hash(const std::string& text, int number) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLen = 0;
    std::string input = text + std::to_string(number); 
    if (EVP_Digest(input.data(), input.size(), digest, &digestLen, EVP_md5(), nullptr) != 1) {
        throw std::runtime_error("EVP_Digest failed");
    }
    std::ostringstream oss;
    for (unsigned int i = 0; i < digestLen; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    return oss.str();
}

int main() {
    const auto text = utils::trim(utils::readFileToString(utils::getInputFilePath("inputs/day04.txt")));

    // generate hashes of the input text and a number until the resulting hash starts with at least five zeroes
    bool p1 = false;
    for (int i = 0;; ++i) {
        auto h = hash(text, i);
        if (h.substr(0, 5) == "00000") {
            if (!p1) {
                std::println("part 1: {}{}", text, i);
                p1 = true;
            }
        }
        if (h.substr(0, 6) == "000000") {
            std::println("part 2: {}{}", text, i);
            break;
        }
    }
    return 0;
}
