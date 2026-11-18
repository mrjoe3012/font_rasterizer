#include <font_rasterizer/Glyph.hpp>

namespace font_rasterizer {

Glyph::Glyph(Buffer raw, Buffer binarised, Buffer pagified, size_t width,
             size_t nPages)
    : _raw(raw), _pagified(pagified), _binarised(binarised), _width(width),
      _nPages(nPages) {}

} // namespace font_rasterizer
