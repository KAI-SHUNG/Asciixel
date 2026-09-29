#ifndef CHARSET_BUILDER_HPP
#define CHARSET_BUILDER_HPP

#include "asciixel/config/asciixel_config.hpp"
#include "asciixel/model/rasterized_charset.hpp"

namespace asciixel {

class CharsetBuilder {
public:
    static RasterizedCharset buildCharset(const CharsetConfig& config);
};

} // namespace asciixel

#endif // CHARSET_BUILDER_HPP
