#include "asciixel/config/asciixel_config.hpp"

#include <stdexcept>

namespace asciixel {

void CharsetConfig::validate() const
{
    if (font_path.empty() || font_path.find_first_of("\r\n") != std::string::npos) {
        throw std::invalid_argument("Invalid font_path");
    }
    if (pixel_size < 1 || pixel_size > 256) {
        throw std::invalid_argument("pixel_size must be 1..256");
    }
    bool seen[127] = {};
    for (unsigned char ch : candidates) {
        if (ch < 32 || ch > 126 || seen[ch]) {
            throw std::invalid_argument("candidates must be unique printable ASCII");
        }
        seen[ch] = true;
    }
    if (!seen[' ']) {
        throw std::invalid_argument("candidates must include a space");
    }
}

} // namespace asciixel
