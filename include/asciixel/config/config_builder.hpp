#ifndef ASCIIXEL_CONFIG_BUILDER_HPP
#define ASCIIXEL_CONFIG_BUILDER_HPP

#include "asciixel/config/asciixel_config.hpp"
#include <string>
#include <optional>
#include <unordered_map>

namespace asciixel {

// Canonical keys: input, output, font, font-size, columns.
// Missing output key means terminal; nullopt means default output path.
// An empty string is an explicit invalid path.
// CLI and GUI can both supply these values; aliases and help are not accepted.
using ConfigValues = std::unordered_map<std::string, std::optional<std::string>>;

// Apply string values to public defaults. Throws invalid_argument on unknown
// keys or invalid typed values. Call resolveConfig afterwards for business rules.
Config buildConfig(const ConfigValues& values);

} // namespace asciixel
#endif
