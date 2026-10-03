#include "asciixel/io/image_loader.hpp"

#include <cmath>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool near(float actual, float expected)
{
    return std::fabs(actual - expected) < 0.01f;
}

void loadsKnownPngPixels(const std::string& path)
{
    const auto frame = asciixel::loadImage(path);
    require(frame.width == 2 && frame.height == 1, "PNG dimensions were not preserved");
    require(frame.pixels.size() == 2, "PNG pixel count is incorrect");
    require(near(frame.at(0, 0).color.r, 255), "first pixel should be red");
    require(near(frame.at(0, 0).color.g, 0.0f), "first pixel should have no green");
    require(near(frame.at(0, 0).color.b, 0.0f), "first pixel should have no blue");
    require(near(frame.at(1, 0).color.r, 0.0f), "second pixel should have no red");
    require(near(frame.at(1, 0).color.g, 255), "second pixel should be green");
    require(near(frame.at(1, 0).color.b, 0.0f), "second pixel should have no blue");
}

void rejectsMissingFile(const std::string& path)
{
    bool threw = false;
    try {
        asciixel::loadImage(path + ".missing");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    require(threw, "missing image should raise runtime_error");
}

void rejectsNonImageFile(const std::string& path)
{
    bool threw = false;
    try {
        asciixel::loadImage(path);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    require(threw, "non-image file should raise runtime_error");
}

} // namespace

int main(int argc, char** argv)
{
    require(argc == 3, "expected PNG and non-image fixture paths");
    loadsKnownPngPixels(argv[1]);
    rejectsMissingFile(argv[1]);
    rejectsNonImageFile(argv[2]);
}
