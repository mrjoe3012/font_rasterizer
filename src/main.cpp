
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <array>

#if DEBUG
#include <opencv2/opencv.hpp>
#endif

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

static constexpr int SCALE = 2;
static constexpr int PIX_HEIGHT = SCALE * 8;

static constexpr unsigned char PRINTABLE_START = 33;
static constexpr unsigned char PRINTABLE_END = 126;

#if DEBUG
static constexpr int DEBUG_HEIGHT = 128;
#endif

struct Glyph {
    unsigned char character;
    int width;
    std::vector<unsigned char> bitmap;
};

void binarise_bitmap(unsigned char* bitmap, size_t n, float threshold = 0.6f) {
    if (n == 0) return;

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

std::vector<unsigned char> pagify_bitmap(unsigned char* data, size_t width) {
  static constexpr int nPages = SCALE;
  std::vector<unsigned char> pagified;
  pagified.resize(nPages * width, 0);
  for (size_t pageIdx = 0; pageIdx < nPages; pageIdx++) {
    for (size_t row = 0; row < 8; row++) {
      for (size_t col = 0; col < width; col++) {
        unsigned char bit = data[width * pageIdx * 8 + row * width + col] > 0 ? 1 : 0;
        if (bit) {
          pagified[pageIdx * width + col] |= (bit << row);
        }
      }
    }
  }
  return pagified;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <TTF FILE>\n";
        return 1;
    }

    // Load TTF file.
    std::ifstream stream(argv[1], std::ios::binary | std::ios::ate);

    if (!stream) {
        std::cerr << "Failed to open '" << argv[1] << "'\n";
        return 2;
    }

    const std::streamsize fileSize = stream.tellg();

    if (fileSize <= 0) {
        std::cerr << "Invalid or empty TTF file\n";
        return 3;
    }

    std::vector<unsigned char> fontData(fileSize);

    stream.seekg(0, std::ios::beg);

    if (!stream.read(
            reinterpret_cast<char*>(fontData.data()),
            fileSize)) {
        std::cerr << "Failed to read TTF file\n";
        return 4;
    }

    // Initialise font.
    stbtt_fontinfo font{};

    const int fontOffset =
        stbtt_GetFontOffsetForIndex(fontData.data(), 0);

    if (fontOffset < 0 ||
        !stbtt_InitFont(&font, fontData.data(), fontOffset)) {
        std::cerr << "Failed to initialise TTF font\n";
        return 5;
    }

    // Scale font to requested pixel height.
    const float scale =
        stbtt_ScaleForPixelHeight(&font, PIX_HEIGHT);

    std::vector<unsigned char> widths;
    widths.reserve(PRINTABLE_END - PRINTABLE_START + 1);
    std::vector<std::vector<unsigned char>> pagifiedBitmaps;
    pagifiedBitmaps.reserve(PRINTABLE_END - PRINTABLE_START + 1);

    // Generate bitmap for each printable character.
    for (unsigned int c = PRINTABLE_START;
         c <= PRINTABLE_END;
         ++c) {

        int width, height;
        int totalWidth;
        int ix0, iy0, ix1, iy1;
        int advWidth, lSideBearing;
        int ascent, descent, linegap;

        unsigned char* bitmap =
            stbtt_GetCodepointBitmap(
                &font,
                scale,
                scale,
                static_cast<int>(c),
                &width,
                &height,
                nullptr,
                nullptr
            );

        // Get glyph bounding box offsets.
        stbtt_GetCodepointBitmapBox(
            &font,
            static_cast<int>(c),
            scale,
            scale,
            &ix0,
            &iy0,
            &ix1,
            &iy1
        );

        // get metrics
        stbtt_GetGlyphHMetrics(
          &font,
          static_cast<int>(c),
          &advWidth,
          &lSideBearing
        );

        // font metrics
        stbtt_GetFontVMetrics(
          &font,
          &ascent,
          &descent,
          &linegap
        );

        totalWidth = width + lSideBearing * scale;

        // Expanded bitmap retains glyph width.
        std::vector<unsigned char> expanded(
            static_cast<size_t>(totalWidth) * PIX_HEIGHT,
            0
        );

        // Position glyph vertically using its bounding-box offset.
        const int yOffset = PIX_HEIGHT - 1 + iy0 + descent * scale;
        const int xOffset = lSideBearing * scale;

        if (bitmap) {
            for (int y = 0; y < height; ++y) {
                const int destY = y + yOffset;

                if (destY < 0 || destY >= PIX_HEIGHT) {
                    continue;
                }

                std::memcpy(
                    expanded.data() +
                        static_cast<size_t>(destY) * totalWidth + xOffset,
                    bitmap +
                        static_cast<size_t>(y) * width,
                    width
                );
            }

            stbtt_FreeBitmap(bitmap, nullptr);
        }

      binarise_bitmap(expanded.data(), expanded.size());

#if DEBUG
        // Visualise expanded glyph bitmap.
        if (totalWidth> 0 && PIX_HEIGHT > 0) {
            cv::Mat glyph(
                PIX_HEIGHT,
                totalWidth,
                CV_8UC1,
                expanded.data()
            );

            const int debugWidth =
                std::max(1, static_cast<int>(
                    std::round(
                        static_cast<double>(totalWidth) *
                        DEBUG_HEIGHT / PIX_HEIGHT
                    )
                ));

            cv::Mat resized;

            cv::resize(
                glyph,
                resized,
                cv::Size(debugWidth, DEBUG_HEIGHT),
                0,
                0,
                cv::INTER_NEAREST
            );

            const std::string filename =
                "vis/" + std::to_string(c) + ".png";

            if (!cv::imwrite(filename, resized)) {
                std::cerr << "Failed to save " << filename << '\n';
            }
        }
#endif

        // Glyph is now available in expanded.
        std::cout
            << "Character: " << static_cast<char>(c)
            << " width: " << totalWidth
            << " height: " << height
            << " iy0: " << iy0
            << " iy1: " << iy1
            << " expanded height: " << PIX_HEIGHT
            << '\n';

      auto pagified = pagify_bitmap(expanded.data(), totalWidth);
      widths.push_back(totalWidth);
      pagifiedBitmaps.push_back(pagified);
    }

    // final step: write the output header
    std::ofstream of("output.h");
    of << "// this file was generated with <github>" << std::endl;
    of << "#define FONT_FONTNAME_NUM_PAGES " << SCALE << std::endl;
    of << "#define FONT_FONTNAME_PIXEL_HEIGHT " << PIX_HEIGHT << std::endl;
    of << "#define FONT_FONTNAME_CIDX(i) (i - " << (int)PRINTABLE_START << ")" << std::endl;
    of << "static const uint8_t FONT_FONTNAME_WIDTHS[] PROGMEM = {" << std::endl;
    for (const auto& width : widths) {
      of << (int)width << ",";
    }
    of << std::endl << "};" << std::endl;

    of << "static const uint32_t FONT_FONTNAME_OFFSETS[] PROGMEM = {" << std::endl;
    uint32_t nextOffset = 0;
    char fmtBuf[128];
    for (const auto& bmp : pagifiedBitmaps) {
      snprintf(fmtBuf, sizeof(fmtBuf), "0x%04x,", nextOffset);
      of << fmtBuf;
      nextOffset += bmp.size();
    }
    of << std::endl << "};" << std::endl;

    of << "static const uint8_t FONT_FONTNAME_GBUF[] PROGMEM = {" << std::endl;
    for (const auto& buf : pagifiedBitmaps) {
      for (const unsigned char val : buf) {
        snprintf(fmtBuf, sizeof(fmtBuf), "0x%02x,", val);
        of << fmtBuf;
      }
    }
    of << std::endl << "};" << std::endl;

    std::cout << "Wrote header file to output.h\nTotal bitmap bytes: " << nextOffset << std::endl;

    return 0;
}
