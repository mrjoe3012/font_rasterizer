#ifndef FONT_RASTERIZER_HPP_
#define FONT_RASTERIZER_HPP_

#include <optional>
#include <string>

#include <font_rasterizer/FontInfo.hpp>
namespace font_rasterizer {

// load a font from the provided path to a .ttf file
std::optional<FontInfo> load_font(const std::string& fontPath);

}  // namespace font_rasterizer

#endif  // FONT_RASTERIZER_HPP_
