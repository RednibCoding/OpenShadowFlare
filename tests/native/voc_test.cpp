#include "libs/RKC_DSOUND/rkc_dsound.hpp"

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace {

void appendU16(
    std::vector<std::uint8_t>& bytes,
    std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value));
    bytes.push_back(static_cast<std::uint8_t>(value >> 8u));
}

void appendU32(
    std::vector<std::uint8_t>& bytes,
    std::uint32_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value));
    bytes.push_back(static_cast<std::uint8_t>(value >> 8u));
    bytes.push_back(static_cast<std::uint8_t>(value >> 16u));
    bytes.push_back(static_cast<std::uint8_t>(value >> 24u));
}

void appendFixedString(
    std::vector<std::uint8_t>& bytes,
    const char* value) {
    std::size_t length = 0;
    while (value[length] != '\0' && length < 255) {
        ++length;
    }
    bytes.insert(bytes.end(), value, value + length);
    bytes.resize(bytes.size() + 256 - length, 0);
}

std::vector<std::uint8_t> makeVocFixture() {
    std::vector<std::uint8_t> bytes;
    const char header[16] = "VoiceData  V003";
    bytes.insert(bytes.end(), header, header + sizeof(header));
    appendU32(bytes, 2);
    appendU32(bytes, 4);

    appendU32(bytes, 0);
    appendFixedString(bytes, "click");
    appendFixedString(bytes, "");
    appendU16(bytes, 1);
    appendU16(bytes, 1);
    appendU32(bytes, 12000);
    appendU32(bytes, 12000);
    appendU16(bytes, 1);
    appendU16(bytes, 8);
    appendU16(bytes, 0x6164);
    appendU32(bytes, 3);
    bytes.push_back(0x40);
    bytes.push_back(0x80);
    bytes.push_back(0xc0);

    appendU32(bytes, 1);
    appendFixedString(bytes, "alias");
    appendFixedString(bytes, "click");
    return bytes;
}

bool check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "%s\n", message);
    }
    return condition;
}

}  // namespace

int main() {
    osf::VocFile file;
    std::string error;
    if (!check(
            file.decode(makeVocFixture(), &error),
            error.c_str())) {
        return 1;
    }
    if (!check(
            file.version() == 3 &&
                file.variant_count() == 4 &&
                file.samples().size() == 2 &&
                file.samples()[0].name == "click" &&
                file.samples()[0].format.sample_rate == 12000 &&
                file.samples()[0].format.bits_per_sample == 8 &&
                file.samples()[0].pcm.size() == 3 &&
                file.samples()[1].name == "click" &&
                file.samples()[1].reference_index == 0,
            "The portable VOC decoder produced the wrong structure.")) {
        return 1;
    }

    std::vector<std::uint8_t> truncated = makeVocFixture();
    truncated.pop_back();
    return check(
               !file.decode(truncated, &error) && !error.empty(),
               "The VOC decoder accepted a truncated reference.")
        ? 0
        : 1;
}
