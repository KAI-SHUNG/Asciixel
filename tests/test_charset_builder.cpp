#include "asciixel/core/charset_builder.hpp"
#include <cmath>
#include <stdexcept>

void require(bool condition) {
    if (!condition) throw std::runtime_error("charset builder assertion failed");
}
int main(int argc, char** argv) {
    require(argc == 2);
    asciixel::CharsetConfig config{argv[1], 24, " .Agj_"};
    const auto charset = asciixel::CharsetBuilder::buildCharset(config);
    require(charset.glyphs.size() == config.charset.size());
    const auto& layout = charset.layout;
    require(layout.cell_width > 0 && layout.cell_height > 0);
    require(charset.glyphs.front().character == ' ');
    require(charset.glyphs.front().density == 0);
    for (const auto& glyph : charset.glyphs) {
        require(glyph.alpha.size() == glyph.bitmap_width * glyph.bitmap_height);
        const auto x = layout.baseline_x + glyph.bitmap_left;
        const auto y = layout.baseline_y - glyph.bitmap_top;
        require(x >= 0 && y >= 0);
        require(static_cast<std::size_t>(x) + glyph.bitmap_width <= layout.cell_width);
        require(static_cast<std::size_t>(y) + glyph.bitmap_height <= layout.cell_height);
        double mass = 0;
        for (auto alpha : glyph.alpha) mass += alpha / 255.0;
        require(std::abs(glyph.density - mass / (layout.cell_width * layout.cell_height)) < 1e-6);
        if (glyph.character != ' ') require(glyph.density > 0 && glyph.density < 1);
    }
    const auto again = asciixel::CharsetBuilder::buildCharset(config);
    for (std::size_t i = 0; i < charset.glyphs.size(); ++i)
        require(charset.glyphs[i].alpha == again.glyphs[i].alpha);
    config.font_size = 48;
    const auto larger = asciixel::CharsetBuilder::buildCharset(config);
    require(larger.layout.cell_width > layout.cell_width);
    require(larger.layout.cell_height > layout.cell_height);
    config.font_size = 0;
    bool rejected = false;
    try { asciixel::CharsetBuilder::buildCharset(config); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected);
}
