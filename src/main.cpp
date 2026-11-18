#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <vector>

#if DEBUG
#include <opencv2/opencv.hpp>
#endif

#include <font_rasterizer/font_rasterizer.hpp>

static constexpr unsigned char PRINTABLE_START = 33;
static constexpr unsigned char PRINTABLE_END = 126;

#if DEBUG
static constexpr int DEBUG_HEIGHT = 128;
#endif

enum RetCode { OKAY = 0, BAD_ARGS, FAILED_TTF_LOAD, COULD_NOT_RASTERIZE };

struct ProgramArgs {
  // path to ttf file defining the font
  std::string ttfPath;
  // final height of bitmap will be scale * 8 bits
  unsigned int scale;
  // threshold used to decide how to binarise a monochrome 8bit image
  float binaryThreshold;
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

    auto gOp = rasterize(*fontOp, static_cast<int32_t>(c), progArgsOp->scale,
                         progArgsOp->binaryThreshold);
    if (!gOp) {
      std::cerr << "Unable to rasterize bitmap for character with code=" << c << std::endl;
      return RetCode::COULD_NOT_RASTERIZE;
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
      cv::Mat glyph(gOp->height(), gOp->width(), CV_8UC1, visVector.data());

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
  static constexpr int bitsPerByte = 8;
  static const int pixelHeight = bitsPerByte * progArgsOp->scale;
  std::ofstream of("output.h");
  of << "// this file was generated with "
        "https://github.com/mrjoe3012/font_rasterizer"
     << std::endl;
  of << "#define FONT_FONTNAME_NUM_PAGES " << progArgsOp->scale << std::endl;
  of << "#define FONT_FONTNAME_PIXEL_HEIGHT " << pixelHeight << std::endl;
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
  static constexpr int minNumArgs = 2, maxNumArgs = 4;
  enum { I_PROGNAME = 0, I_TTFPATH, I_SCALE, I_THRESHOLD };

  auto badArgs = [argv]() {
    std::cerr
        << "Usage: " << argv[I_PROGNAME] << " <TTF file>\n"
        << "    [scale (>= 1, default: 2)]\n"
        << "    [binaryThreshold (0 < binaryThreshold < 1, default: 0.6)]\n";
    return std::optional<ProgramArgs>();
  };

  if (argc < minNumArgs || argc > maxNumArgs) {
    return badArgs();
  }

  std::string ttfPath = argv[I_TTFPATH];

  unsigned int scale = 2;
  if (argc > minNumArgs) {
    unsigned long input;
    size_t pos;
    try {
      input = std::stoul(argv[I_SCALE], &pos);
    } catch (const std::exception &) {
      return badArgs();
    }
    static constexpr unsigned int maxVal =
        std::numeric_limits<unsigned int>::max();
    if (input > maxVal || input == 0 || pos != std::strlen(argv[I_SCALE])) {
      return badArgs();
    }
    scale = input;
  }

  float binaryThreshold = 0.6f;
  if (argc == maxNumArgs) {
    float input;
    size_t pos;
    try {
      input = std::stof(argv[I_THRESHOLD], &pos);
    } catch (const std::exception &) {
      return badArgs();
    }
    if (input <= 0 || input >= 1 || !std::isfinite(input) ||
        pos != std::strlen(argv[I_THRESHOLD])) {
      return badArgs();
    }
    binaryThreshold = input;
  }

  std::cout << "Configuration:\n\tttfPath: '" << ttfPath
            << "'\n\tscale: " << scale
            << "\n\tbinaryThreshold: " << binaryThreshold << std::endl
            << std::endl;
  ;

  return {{ttfPath, scale, binaryThreshold}};
}
