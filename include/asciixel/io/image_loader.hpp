#ifndef ASCIIXEL_IMAGE_LOADER_HPP
#define ASCIIXEL_IMAGE_LOADER_HPP

#include "asciixel/model/image_frame.hpp"

#include <string>

namespace asciixel {

// Pixels and background are sRGB8. Transparency is composited in linear light.
ImageFrame loadImage(const std::string& path, Color background = {0, 0, 0});

} // namespace asciixel

#endif
