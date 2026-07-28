#include "gapi/bitmap.hpp"
#include "gapi/gapi.hpp"
#include "gapi/njp.hpp"
#include "gapi/software_backend.hpp"

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace {

void appendI32(
    std::vector<std::uint8_t>& bytes,
    std::int32_t value) {
    const std::uint32_t raw = static_cast<std::uint32_t>(value);
    bytes.push_back(static_cast<std::uint8_t>(raw));
    bytes.push_back(static_cast<std::uint8_t>(raw >> 8u));
    bytes.push_back(static_cast<std::uint8_t>(raw >> 16u));
    bytes.push_back(static_cast<std::uint8_t>(raw >> 24u));
}

void appendBytes(
    std::vector<std::uint8_t>& destination,
    const char* source,
    std::size_t size) {
    destination.insert(
        destination.end(), source, source + size);
}

std::vector<std::uint8_t> makeCompressedNjpFixture() {
    std::vector<std::uint8_t> bytes;
    const char header[16] = "NJudgeUniPat003";
    appendBytes(bytes, header, sizeof(header));

    appendI32(bytes, 1);
    appendI32(bytes, 8);
    appendI32(bytes, 8);
    appendI32(bytes, 2);
    appendI32(bytes, 2);
    appendI32(bytes, 1);

    const char compressionMagic[8] = {
        'R', 'C', 'L', 'I', 'B', '-', 'L', '\x1a',
    };
    appendBytes(
        bytes, compressionMagic, sizeof(compressionMagic));
    appendI32(bytes, 8);
    appendI32(bytes, 9);
    bytes.push_back(0);
    const std::uint8_t pixels[8] = {
        3, 4, 0, 0,
        1, 2, 0, 0,
    };
    bytes.insert(bytes.end(), pixels, pixels + sizeof(pixels));

    appendI32(bytes, 1);
    appendI32(bytes, 1);
    appendI32(bytes, 1);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 2);
    appendI32(bytes, 2);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 1000);
    appendI32(bytes, 1000);

    appendI32(bytes, 1);
    for (std::int32_t index = 0; index < 256; ++index) {
        std::uint8_t red = 0;
        std::uint8_t green = 0;
        std::uint8_t blue = 0;
        if (index == 1) {
            red = 100;
        } else if (index == 2) {
            green = 120;
        } else if (index == 3) {
            blue = 140;
        } else if (index == 4) {
            red = 200;
            green = 180;
            blue = 160;
        }
        bytes.push_back(red);
        bytes.push_back(green);
        bytes.push_back(blue);
        bytes.push_back(0);
    }
    return bytes;
}

std::vector<std::uint8_t> makeFontNjpFixture() {
    std::vector<std::uint8_t> bytes;
    const char header[16] = "NJudgeUniPat002";
    appendBytes(bytes, header, sizeof(header));

    appendI32(bytes, 1);
    appendI32(bytes, 8);
    appendI32(bytes, 16);
    appendI32(bytes, 16);
    appendI32(bytes, 0);
    std::vector<std::uint8_t> pixels(16 * 16, 0);
    pixels[11 * 16 + 1] = 1;
    bytes.insert(bytes.end(), pixels.begin(), pixels.end());

    appendI32(bytes, 1);
    appendI32(bytes, 1);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 16);
    appendI32(bytes, 16);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 1000);
    appendI32(bytes, 1000);

    appendI32(bytes, 1);
    for (std::int32_t index = 0; index < 256; ++index) {
        const std::uint8_t value = index == 1 ? 255 : 0;
        bytes.push_back(value);
        bytes.push_back(value);
        bytes.push_back(value);
        bytes.push_back(0);
    }
    return bytes;
}

std::vector<std::uint8_t> makeBitmapFixture() {
    std::vector<std::uint8_t> bytes(62, 0);
    bytes[0] = 'B';
    bytes[1] = 'M';
    bytes[2] = 62;
    bytes[10] = 54;
    bytes[14] = 40;
    bytes[18] = 2;
    bytes[22] = 1;
    bytes[26] = 1;
    bytes[28] = 24;
    bytes[34] = 8;
    bytes[54] = 30;
    bytes[55] = 20;
    bytes[56] = 10;
    bytes[57] = 60;
    bytes[58] = 50;
    bytes[59] = 40;
    return bytes;
}

bool check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "%s\n", message);
    }
    return condition;
}

bool testViewport() {
    const osf::gapi::Viewport wide =
        osf::gapi::fitViewport(640, 480, 1920, 1080);
    const osf::gapi::Viewport fourK =
        osf::gapi::fitViewport(640, 480, 3840, 2160);
    return check(
        wide.x == 240 && wide.y == 0 &&
            wide.width == 1440 && wide.height == 1080 &&
            fourK.x == 480 && fourK.y == 0 &&
            fourK.width == 2880 && fourK.height == 2160,
        "GAPI aspect-fit viewport calculation differs.");
}

bool testNjpAndSoftwareBackend() {
    osf::gapi::NjpImage image;
    std::string error;
    if (!check(
            image.decode(makeCompressedNjpFixture(), &error),
            error.c_str())) {
        return false;
    }
    if (!check(
            image.version() == 3 &&
                image.parts().size() == 1 &&
                image.parts()[0].stride == 4 &&
                image.patterns().size() == 1 &&
                image.patterns()[0].parts.size() == 1 &&
                image.palettes().size() == 1,
            "The portable NJP decoder produced the wrong structure.")) {
        return false;
    }

    std::int32_t presented = 0;
    osf::gapi::SoftwareBackend backend(
        2,
        2,
        [&presented](osf::gapi::SurfaceView surface) {
            if (surface.width == 2 && surface.height == 2) {
                ++presented;
            }
        });
    backend.beginFrame({1, 2, 3, 255});
    if (!check(
            backend.drawPattern(
                image, 0, {0, 0, 1000, 1000, 500}),
            "The software backend rejected a valid NJP pattern.")) {
        return false;
    }
    backend.endFrame();

    const osf::gapi::SurfaceView surface = backend.surface();
    return check(
        sizeof(osf::gapi::Color) == 4 &&
            presented == 1 &&
            surface.pixels[0].red == 50 &&
            surface.pixels[0].green == 0 &&
            surface.pixels[1].red == 0 &&
            surface.pixels[1].green == 60 &&
            surface.pixels[2].blue == 70 &&
            surface.pixels[3].red == 100 &&
            surface.pixels[3].green == 90 &&
            surface.pixels[3].blue == 80,
        "NJP orientation, palette conversion, or brightness differs.");
}

bool testTruncatedNjp() {
    std::vector<std::uint8_t> bytes = makeCompressedNjpFixture();
    bytes.resize(40);
    osf::gapi::NjpImage image;
    std::string error;
    return check(
        !image.decode(bytes, &error) && !error.empty(),
        "The NJP decoder accepted a truncated compressed bitmap.");
}

bool testBitmapAndTextDrawing() {
    osf::gapi::BitmapImage bitmap;
    std::string error;
    if (!check(
            bitmap.decode(makeBitmapFixture(), &error),
            error.c_str())) {
        return false;
    }

    osf::gapi::NjpImage font;
    if (!check(
            font.decode(makeFontNjpFixture(), &error),
            error.c_str())) {
        return false;
    }

    osf::gapi::SoftwareBackend backend(5, 5);
    backend.beginFrame({0, 0, 0, 255});
    if (!check(
            backend.drawRectangle(
                {1, 1, 3, 2, {80, 40, 20, 255}, 500}) &&
                backend.drawBitmap(bitmap, {0, 0}) &&
                backend.drawText(
                    font,
                    "A",
                    {2, 3, {200, 100, 50, 255}, 500}),
            "The software backend rejected bitmap or text drawing.")) {
        return false;
    }
    const osf::gapi::SurfaceView surface = backend.surface();
    return check(
        bitmap.width() == 2 && bitmap.height() == 1 &&
            surface.pixels[0].red == 10 &&
            surface.pixels[0].green == 20 &&
            surface.pixels[0].blue == 30 &&
            surface.pixels[1].red == 40 &&
            surface.pixels[1].green == 50 &&
            surface.pixels[1].blue == 60 &&
            surface.pixels[1 * 5 + 1].red == 40 &&
            surface.pixels[1 * 5 + 1].green == 20 &&
            surface.pixels[1 * 5 + 1].blue == 10 &&
            surface.pixels[3 * 5 + 2].red == 100 &&
            surface.pixels[3 * 5 + 2].green == 50 &&
            surface.pixels[3 * 5 + 2].blue == 25,
        "BMP orientation, BGR conversion, or font tinting differs.");
}

}  // namespace

int main() {
    if (!testViewport() ||
        !testNjpAndSoftwareBackend() ||
        !testTruncatedNjp() ||
        !testBitmapAndTextDrawing()) {
        return 1;
    }
    return 0;
}
