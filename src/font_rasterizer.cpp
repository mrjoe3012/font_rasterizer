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
  stream.read(
    reinterpret_cast<char*>(fontData.data()),
    fileSize
  );
  if (!stream) {
    return {};
  }
  stbtt_fontinfo font;
  const int fontOffset = stbtt_GetFontOffsetForIndex(fontData.data(), 0);
  if (fontOffset < 0 || !stbtt_InitFont(&font, fontData.data(), fontOffset)) {
    return {};
  }
  return {{
    font,
    std::move(fontData)
  }};
}

} // namespace font_rasteriser
