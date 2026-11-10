// Both approaches are vectorized. Hence, the vanilla is more performant

#include <benchmark/benchmark.h>
#include <cstring>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

constexpr int DESIRED_CHS = 1;
constexpr int kernel[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};

unsigned char *get_image_data(int *width, int *height, int *chs_in) {
  return stbi_load("./image.jpg", width, height, chs_in, DESIRED_CHS);
}

static void BM_vanilla(benchmark::State &state) {
  int width, height, chs_in;
  auto data = get_image_data(&width, &height, &chs_in);
  if (data == nullptr) {
    state.SkipWithError("Could not load ./image.jpg");
    return;
  }
  const int output_width = width - 2;
  const int output_height = height - 2;
  auto buffer = new unsigned char[output_height * output_width];

  for (auto _ : state) {
    for (int i = 0; i < height - 2; ++i) {
      for (int j = 0; j < width - 2; ++j) {
        int response = ((kernel[0][0] * data[i * width + j]) +
                        (kernel[0][1] * data[i * width + j + 1]) +
                        (kernel[0][2] * data[i * width + j + 2]) +
                        (kernel[1][0] * data[(i + 1) * width + j]) +
                        (kernel[1][1] * data[(i + 1) * width + j + 1]) +
                        (kernel[1][2] * data[(i + 1) * width + j + 2]) +
                        (kernel[2][0] * data[(i + 2) * width + j]) +
                        (kernel[2][1] * data[(i + 2) * width + j + 1]) +
                        (kernel[2][2] * data[(i + 2) * width + j + 2]));
        response = response < 0 ? -response : response;
        buffer[i * output_width + j] =
            static_cast<unsigned char>(response > 255 ? 255 : response);
      }
    }
    int checksum = 0;
    for (int i = 0; i < output_height * output_width; ++i) {
      checksum += buffer[i];
    }
    benchmark::DoNotOptimize(checksum);
  }

  stbi_write_jpg("./horizontal.jpg", output_width, output_height, DESIRED_CHS,
                 buffer, output_width);
  stbi_image_free(data);
  delete[] buffer;
}

static void BM_partial(benchmark::State &state) {
  int width, height, chs_in;
  auto data = get_image_data(&width, &height, &chs_in);
  if (data == nullptr) {
    state.SkipWithError("Could not load ./image.jpg");
    return;
  }
  const int output_width = width - 2;
  const int output_height = height - 2;
  const int output_size = output_width * output_height;
  auto buffer = new unsigned char[output_size];
  auto buffer_t = new int[output_size];

  for (auto _ : state) {
    std::memset(buffer_t, 0, output_size * sizeof(*buffer_t));

    // Top row of kernel
    for (int i = 0; i < output_height; ++i) {
      for (int j = 0; j < output_width; ++j) {
        buffer_t[i * output_width + j] +=
            kernel[0][0] * data[i * width + j] +
            kernel[0][1] * data[i * width + j + 1] +
            kernel[0][2] * data[i * width + j + 2];
      }
    }
    // Central row of kernel
    for (int i = 0; i < output_height; ++i) {
      for (int j = 0; j < output_width; ++j) {
        buffer_t[i * output_width + j] +=
            kernel[1][0] * data[(i + 1) * width + j] +
            kernel[1][1] * data[(i + 1) * width + j + 1] +
            kernel[1][2] * data[(i + 1) * width + j + 2];
      }
    }
    // Bottom row of kernel
    for (int i = 0; i < output_height; ++i) {
      for (int j = 0; j < output_width; ++j) {
        buffer_t[i * output_width + j] +=
            kernel[2][0] * data[(i + 2) * width + j] +
            kernel[2][1] * data[(i + 2) * width + j + 1] +
            kernel[2][2] * data[(i + 2) * width + j + 2];
      }
    }

    // All together
    for (int i = 0; i < output_height; ++i) {
      for (int j = 0; j < output_width; ++j) {
        const int index = i * output_width + j;
        auto val = buffer_t[index];
        auto response = val < 0 ? -val : val;
        buffer[index] =
            static_cast<unsigned char>(response > 255 ? 255 : response);
      }
    }

    int checksum = 0;
    for (int i = 0; i < output_size; ++i) {
      checksum += buffer[i];
    }
    benchmark::DoNotOptimize(checksum);
  }

  stbi_image_free(data);
  delete[] buffer;
  delete[] buffer_t;
}

BENCHMARK(BM_vanilla);
BENCHMARK(BM_partial);
BENCHMARK_MAIN();
