#ifndef GLYPH_HPP_
#define GLYPH_HPP_

#include <vector>
#include <cstddef>
#include <cstdint>

namespace font_rasterizer {

class Glyph {
public:
  typedef std::vector<unsigned char> Buffer;
  Glyph(Buffer raw, Buffer binarised, Buffer pagified, size_t width, size_t nPages);

  // the character this glyph represents
  inline int32_t codepoint() const {
    return _codepoint;
  }
  // pixel width of stored bitmaps
  inline size_t width() const {
    return _width;
  }
  // pixel height of stored bitmaps
  inline size_t height() const {
    static constexpr int bitsPerByte = 8;
    return _nPages * bitsPerByte;
  }
  // number of pages (8-byte blocks) in pagified bitmap
  inline size_t nPages() const {
    return _nPages;
  }

  // raw bitmap as defined by the .ttf and rendered by STB TRUETYPE
  // monochrome row-major bitmap with 1 byte per pixel
  inline const Buffer& rawBuffer() const {
    return _raw;
  }
  // like rawBuffer, but all elements are 0 or 255
  inline const Buffer& binarisedBuffer() const {
    return _binarised;
  }
  // same data as binarisedBuffer, but in a different data representation:
  // each page (column of 8 vertical pixels) is packed into a single byte
  // the bitmap is stored as a matrix of pages in row-major order (nPagesxwidth)
  // individual pixel values require bitmasking the corresponding page
  // this data format is compatible with display drivers like the ssd1306
  inline const Buffer& pagifiedBuffer() const {
    return _pagified;
  }

private:
  Buffer _raw, _pagified, _binarised;
  size_t _width, _nPages;
  int32_t _codepoint;
};

}  // namespace font_rasterizer

#endif  // GLYPH_HPP_
