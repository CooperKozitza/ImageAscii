#include <algorithm>
#include <cctype>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

constexpr int DEFAULT_WIDTH = 80;
constexpr int DEFAULT_HEIGHT = 80;

constexpr size_t DEFAULT_ASCII_GRADIENT_SIZE = 58;
constexpr const char *DEFAULT_ASCII_GRADIENT = " `.-':_,^=;><+!rc*/z?sLTv)J731tl2EwqP6h9d4pOGUAKXg0MNWQ%&@";

struct arguments {
  const char *path;

  int output_width;
  int output_height;

  size_t ascii_gradient_size;
  const char *ascii_gradient;
};

arguments parse_args(const size_t argc, const char **argv);
bool is_number(const char *s);

int main(const int argc, const char **argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <filepath> [--width=N] [--height=N] [--ascii-gradient=STRING]" << std::endl;
        return 1;
    }

    const arguments args = parse_args(argc, argv);

    int input_width, input_height, channels;
    unsigned char *data = stbi_load(args.path, &input_width, &input_height, &channels, 0);

    if (!data) {
        throw std::runtime_error("Failed to load image");
    }

    const size_t output_pixel_width = input_width / args.output_width;
    const size_t output_pixel_height = input_height / args.output_height;

    if (output_pixel_width == 0 || output_pixel_height == 0) {
        stbi_image_free(data);
        throw std::runtime_error("Width or height is zero");
    }

    std::vector<unsigned char> pixel_averages(args.output_height * args.output_width);
    for (size_t y = 0; y < args.output_height; ++y) {
        for (size_t x = 0; x < args.output_width; ++x) {
            const size_t base_y = y * output_pixel_height;
            const size_t base_x = x * output_pixel_width;

            int sum = 0;
            for (size_t dy = 0; dy < output_pixel_height; ++dy) {
                for (size_t dx = 0; dx < output_pixel_width; ++dx) {
                    const int pixel_index = ((base_y + dy) * input_width + (base_x + dx)) * channels;
                    sum += std::accumulate(data + pixel_index, data + pixel_index + channels, 0);
                }
            }

            pixel_averages[y * args.output_width + x] = sum / (output_pixel_width * output_pixel_height * channels);
        }
    }

    size_t ignore_count = pixel_averages.size() / 10;

    std::vector<unsigned char> sorted_pixel_averages = pixel_averages;
    
    std::nth_element(sorted_pixel_averages.begin(), sorted_pixel_averages.begin() + ignore_count, sorted_pixel_averages.end());
    unsigned char min = sorted_pixel_averages[ignore_count];

    std::nth_element(sorted_pixel_averages.begin(), sorted_pixel_averages.end() - ignore_count - 1, sorted_pixel_averages.end());
    unsigned char max = sorted_pixel_averages[sorted_pixel_averages.size() - ignore_count - 1];

    if (min > max) std::swap(min, max);

    std::vector<char> output(args.output_height * args.output_width);
    const float gradient_scale = static_cast<float>(args.ascii_gradient_size - 1) / UCHAR_MAX;

    for (size_t i = 0; i < pixel_averages.size(); ++i) {
        const unsigned char clamped_value = std::clamp<int>(pixel_averages[i] - min, 0, max - min);
        const unsigned int normalized_value = static_cast<unsigned int>(clamped_value) * UCHAR_MAX / (max - min);

        output[i] = args.ascii_gradient[static_cast<size_t>(normalized_value * gradient_scale)];
    }

    for (size_t y = 0; y < args.output_height; ++y) {
        for (size_t x = 0; x < args.output_width; ++x) {
            std::cout << output[y * args.output_width + x];
        }
        std::cout << '\n';
    }

    stbi_image_free(data);
    return 0;
}

arguments parse_args(const size_t argc, const char **argv) {
  arguments args{};

  args.output_width = DEFAULT_WIDTH;
  args.output_height = DEFAULT_HEIGHT;

  args.ascii_gradient = DEFAULT_ASCII_GRADIENT;
  args.ascii_gradient_size = DEFAULT_ASCII_GRADIENT_SIZE;

  args.path = argv[1];

  for (size_t i = 2; i < argc; ++i) {
    const char *arg = argv[i];

    while (*arg != '\0' && !std::isalpha(*arg)) 
      ++arg;

    const char arg_name = *arg;

    while (*arg != '\0' && *arg != '=')
      ++arg;

    if (*arg == '=')
      ++arg;

    const bool num = is_number(arg);

    if (arg_name == 'w' && num) {
      args.output_width = std::atoi(arg);
    }
    else if (arg_name == 'h' && num) {
      args.output_height = std::atoi(arg);
    }
    else if (arg_name == 'a') {
      args.ascii_gradient = arg;

      size_t size = 0;
      while (*arg != '\0') 
        ++size, ++arg;

      args.ascii_gradient_size = size;
    }
  }

  return args;
}

bool is_number(const char *s) {
  if (*s == '\0') {
    return false;
  }

  if (!std::isdigit(*s)) {
    return false;
  }

  if (*(s + 1) == '\0') {
    return true;
  }

  return is_number(s + 1);
}
