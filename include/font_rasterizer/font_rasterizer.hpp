#ifndef FONT_RASTERIZER_HPP_
#define FONT_RASTERIZER_HPP_

#include <optional>
#include <string>
#include <cstdint>

#include <font_rasterizer/FontInfo.hpp>
#include <font_rasterizer/Glyph.hpp>
namespace font_rasterizer {

// load a font from the provided path to a .ttf file
std::optional<FontInfo> load_font(const std::string& fontPath);

// binarise a bitmap by intensity threshold based on per-bitmap pixel
// intensity distribution
void binarise_bitmap(unsigned char *bitmap, size_t n, float threshold);

// convert the bitmap (one byte per pixel) into a row-major page-oriented representation
// (8 pixels per byte) compatible with display controllers such as ssd1306
std::vector<unsigned char> pagify_bitmap(const unsigned char *data, size_t width, unsigned int scale);

/**
 * @brief rasterize the glyph for a particular character
 * 
 * @param font 
 * @param codepoint Unicode character codepoint to rasterize
 * @param scale defines bitmap height (= 8 * scale) and npages (= scale)
 * @param threshold binarisation threshold 0 < t < 1
 * @return std::optional<Glyph> 
 */
std::optional<Glyph> rasterize(const FontInfo& font, int32_t codepoint, unsigned int scale, float threshold);

}  // namespace font_rasterizer

#endif  // FONT_RASTERIZER_HPP_
