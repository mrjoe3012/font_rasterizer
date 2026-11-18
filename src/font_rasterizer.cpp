#include <font_rasterizer/font_rasterizer.hpp>

#include <fstream>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace font_rasterizer {

std::optional<FontInfo> load_font(const std::string &fontPath) {
  std::ifstream stream(fontPath, std::ios::binary | std::ios::ate);
  if (!stream) {
    return {};
  }
  const std::streamsize fileSize = stream.tellg();
  if (fileSize <= 0) {
    return {};
  }
  std::vector<unsigned char> fontData(fileSize);
  stream.seekg(0, std::ios::beg);
  stream.read(reinterpret_cast<char *>(fontData.data()), fileSize);
  if (!stream) {
    return {};
  }
  stbtt_fontinfo font;
  const int fontOffset = stbtt_GetFontOffsetForIndex(fontData.data(), 0);
  if (fontOffset < 0 || !stbtt_InitFont(&font, fontData.data(), fontOffset)) {
    return {};
  }
  return {{font, std::move(fontData)}};
}

void binarise_bitmap(unsigned char *const bitmap, const size_t n, const float threshold) {
  if (n == 0)
    return;

  unsigned long histogram[256] = {};

  for (size_t i = 0; i < n; ++i) {
    ++histogram[bitmap[i]];
  }

  const size_t percentile = static_cast<size_t>(threshold * n);
  size_t cumulative = 0;
  unsigned char cutoff = 255;

  for (int i = 0; i < 256; ++i) {
    cumulative += histogram[i];
    if (cumulative >= percentile) {
      cutoff = static_cast<unsigned char>(i);
      break;
    }
  }

  for (size_t i = 0; i < n; ++i) {
    bitmap[i] = bitmap[i] > cutoff ? 255 : 0;
  }
}

std::vector<unsigned char> pagify_bitmap(const unsigned char *const data,
                                         const size_t width, const unsigned int scale) {
  const int nPages = scale;
  std::vector<unsigned char> pagified;
  pagified.resize(nPages * width, 0);
  for (size_t pageIdx = 0; pageIdx < nPages; pageIdx++) {
    for (size_t row = 0; row < 8; row++) {
      for (size_t col = 0; col < width; col++) {
        unsigned char bit =
            data[width * pageIdx * 8 + row * width + col] > 0 ? 1 : 0;
        if (bit) {
          pagified[pageIdx * width + col] |= (bit << row);
        }
      }
    }
  }
  return pagified;
}

} // namespace font_rasterizer
