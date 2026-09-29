#include "asciixel/config/asciixel_config.hpp"

#include <stdexcept>
#include <string>

namespace {

void require(bool condition)
{
    if (!condition) {
        throw std::runtime_error("charset config assertion failed");
    }
}

void acceptsValidCandidates()
{
    const asciixel::CharsetConfig original{
        "C:/fonts/Maple Mono.ttf", 24, " =#\\'\""};
    original.validate();
}

void rejectsMissingSpace()
{
    const asciixel::CharsetConfig config{"font.ttf", 24, "#@"};
    bool rejected = false;
    try {
        config.validate();
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected);
}

} // namespace

int main()
{
    acceptsValidCandidates();
    rejectsMissingSpace();
}
