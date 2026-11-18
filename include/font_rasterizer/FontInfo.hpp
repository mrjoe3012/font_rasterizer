#ifndef FONT_INFO_HPP_
#define FONT_INFO_HPP_

#include <stb_truetype.h>
#include <vector>

namespace font_rasterizer {

struct FontInfo {
  stbtt_fontinfo fontInfo;
  std::vector<unsigned char> data;
};

}  // namespace font_rasterizer

#endif  // FONT_INFO_HPP_