#include <cstdint>
#include <fstream>
#include <optional>
#include <vector>
#include <iostream>

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

enum RetCode { OKAY = 0, BAD_ARGS, FAILED_TTF_LOAD };

struct ProgramArgs {
  std::string ttfPath;
};

// returns true if program arguments are okay
std::optional<ProgramArgs> get_args(int argc, const char *const *argv);

using namespace font_rasterizer;
using namespace std;

int main(int argc, char **argv) {

  auto progArgsOp = get_args(argc, argv);
  if (!progArgsOp) {
    return RetCode::BAD_ARGS;
  }
  std::string fontPath = argv[1];
  auto fontOp = load_font(fontPath);
  if (!fontOp) {
    return RetCode::FAILED_TTF_LOAD;
  }

  std::vector<unsigned char> widths;
  widths.reserve(PRINTABLE_END - PRINTABLE_START + 1);
  std::vector<std::vector<unsigned char>> pagifiedBitmaps;
  pagifiedBitmaps.reserve(PRINTABLE_END - PRINTABLE_START + 1);

  // Generate bitmap for each printable character.
  for (unsigned int c = PRINTABLE_START; c <= PRINTABLE_END; ++c) {

    auto gOp = rasterize(*fontOp, static_cast<int32_t>(c), SCALE);
    if (!gOp) {
      // TODO
      std::cout << "CRITICAL ERROR\n";
      return 1;
    }

    std::cout << "Character: " << static_cast<char>(c)
              << " width: " << gOp->width() << " height: " << gOp->height()
              << std::endl;

    widths.push_back(gOp->width());
    pagifiedBitmaps.push_back(gOp->pagifiedBuffer());

#if DEBUG
    // Visualise expanded glyph bitmap.
    auto visVector = gOp->binarisedBuffer();
    if (gOp->width() > 0 && gOp->height() > 0) {
      cv::Mat glyph(PIX_HEIGHT, gOp->width(), CV_8UC1, visVector.data());

      const int debugWidth = std::max(
          1, static_cast<int>(std::round(static_cast<double>(gOp->width()) *
                                         DEBUG_HEIGHT / gOp->height())));

      cv::Mat resized;

      cv::resize(glyph, resized, cv::Size(debugWidth, DEBUG_HEIGHT), 0, 0,
                 cv::INTER_NEAREST);

      const std::string filename = "vis/" + std::to_string(c) + ".png";

      if (!cv::imwrite(filename, resized)) {
        std::cerr << "Failed to save " << filename << '\n';
      }
    }
#endif
  }

  // final step: write the output header
  std::ofstream of("output.h");
  of << "// this file was generated with <github>" << std::endl;
  of << "#define FONT_FONTNAME_NUM_PAGES " << SCALE << std::endl;
  of << "#define FONT_FONTNAME_PIXEL_HEIGHT " << PIX_HEIGHT << std::endl;
  of << "#define FONT_FONTNAME_CIDX(i) (i - " << (int)PRINTABLE_START << ")"
     << std::endl;
  of << "static const uint8_t FONT_FONTNAME_WIDTHS[] PROGMEM = {" << std::endl;
  for (const auto &width : widths) {
    of << (int)width << ",";
  }
  of << std::endl << "};" << std::endl;

  of << "static const uint32_t FONT_FONTNAME_OFFSETS[] PROGMEM = {"
     << std::endl;
  uint32_t nextOffset = 0;
  char fmtBuf[128];
  for (const auto &bmp : pagifiedBitmaps) {
    snprintf(fmtBuf, sizeof(fmtBuf), "0x%04x,", nextOffset);
    of << fmtBuf;
    nextOffset += bmp.size();
  }
  of << std::endl << "};" << std::endl;

  of << "static const uint8_t FONT_FONTNAME_GBUF[] PROGMEM = {" << std::endl;
  for (const auto &buf : pagifiedBitmaps) {
    for (const unsigned char val : buf) {
      snprintf(fmtBuf, sizeof(fmtBuf), "0x%02x,", val);
      of << fmtBuf;
    }
  }
  of << std::endl << "};" << std::endl;

  std::cout << "Wrote header file to output.h\nTotal bitmap bytes: "
            << nextOffset << std::endl;

  return 0;
}

std::optional<ProgramArgs> get_args(int argc, const char *const *const argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <TTF FILE>\n";
    return {};
  }
  return {{
    argv[1],
  }};
}
