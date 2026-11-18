
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

#include <font_rasterizer/font_rasterizer.hpp>


static constexpr unsigned char PRINTABLE_START = 33;
static constexpr unsigned char PRINTABLE_END = 126;

static constexpr int SCALE = 2;
static constexpr int PIX_HEIGHT = SCALE * 8;


#if DEBUG
static constexpr int DEBUG_HEIGHT = 128;
#endif

struct Glyph {
    unsigned char character;
    int width;
    std::vector<unsigned char> bitmap;
};

enum RetCode {
    OKAY = 0,
    WRONG_ARG_COUNT,
    FAILED_TTF_LOAD
};

// returns true if program arguments are okay
bool check_args(int argc, const char * const *argv);

using namespace font_rasterizer;
using namespace std;

int main(int argc, char** argv) {

    if (!check_args(argc, argv)) {
        return RetCode::WRONG_ARG_COUNT;
    }

    std::string fontPath = argv[1];
    auto fontOp = load_font(fontPath);
    if (!fontOp) {
        return RetCode::FAILED_TTF_LOAD;
    }

    // Scale font to requested pixel height.
    const float scale =
        stbtt_ScaleForPixelHeight(&fontOp->fontInfo, PIX_HEIGHT);

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
                &fontOp->fontInfo,
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
            &fontOp->fontInfo,
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
          &fontOp->fontInfo,
          static_cast<int>(c),
          &advWidth,
          &lSideBearing
        );

        // font metrics
        stbtt_GetFontVMetrics(
          &fontOp->fontInfo,
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

      auto pagified = pagify_bitmap(expanded.data(), totalWidth, SCALE);
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

bool check_args(int argc, const char * const * const argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <TTF FILE>\n";
        return false;
    }
    return true;
}
