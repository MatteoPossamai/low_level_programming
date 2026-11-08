// Compiled with:
// Non vectorized:
// clang++ -std=c++20 -Wall -Wextra -O3 -fno-vectorize -fno-slp-vectorize
// images.cpp
// Auto vectorized: clang++ -std=c++20 -Wall -Wextra -O3
// -march=native -Rpass=loop-vectorize -Rpass-missed=loop-vectorize
// -Rpass-analysis=loop-vectorize images.cpp

// Non vectorized output:
// grayscale: 0,167ms
// threshold: 0,157ms
//
// Vectorized:
// grayscale: 0.112ms
// threshold: 0.090ms

#include <chrono>
#include <cmath>
#include <cstddef>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

constexpr int DESIRED_CHS = 3;

int grayscale(int r, int g, int b) {
  float_t gray = 0.299f * r + 0.587f * g + 0.114f * b;
  return static_cast<int>(gray + 0.5f);
}

int main() {
  typedef std::chrono::high_resolution_clock Clock;
  int width, height, chs_in;
  unsigned char *data =
      stbi_load("./image.jpg", &width, &height, &chs_in, DESIRED_CHS);

  if (data == NULL) {
    return 1;
  }

  // Grayscale generation
  std::cout << "Starting grayscale generation" << std::endl;
  unsigned char *grayscale_arr =
      new unsigned char[width * height * DESIRED_CHS];
  auto t1 = Clock::now();
  for (int i = 0; i < width * height; ++i) {
    int r = data[i * DESIRED_CHS + 0];
    int g = data[i * DESIRED_CHS + 1];
    int b = data[i * DESIRED_CHS + 2];

    int gray = grayscale(r, g, b);

    grayscale_arr[i * DESIRED_CHS + 0] = gray;
    grayscale_arr[i * DESIRED_CHS + 1] = gray;
    grayscale_arr[i * DESIRED_CHS + 2] = gray;
  }
  auto t0 = Clock::now();
  stbi_write_jpg("./grayscale.jpg", width, height, DESIRED_CHS, grayscale_arr,
                 width * 3);
  delete[] grayscale_arr;

  std::cout << "Done with grayscale" << std::endl;
  std::cout << "Took:" << t0 - t1 << std::endl;

  // Thresholding
  std::cout << "Starting threashold generation" << std::endl;
  unsigned char *thr_arr = new unsigned char[width * height * DESIRED_CHS];
  t1 = Clock::now();
  for (int i = 0; i < width * height; ++i) {
    int r = data[i * DESIRED_CHS + 0];
    int g = data[i * DESIRED_CHS + 1];
    int b = data[i * DESIRED_CHS + 2];

    int color = grayscale(r, g, b) >= 128 ? 255 : 0;

    thr_arr[i * DESIRED_CHS + 0] = color;
    thr_arr[i * DESIRED_CHS + 1] = color;
    thr_arr[i * DESIRED_CHS + 2] = color;
  }
  t0 = Clock::now();
  stbi_write_jpg("./threshold.jpg", width, height, DESIRED_CHS, thr_arr,
                 width * 3);
  delete[] thr_arr;
  std::cout << "Done with threashold" << std::endl;
  std::cout << "Took:" << t0 - t1 << std::endl;

  stbi_image_free(data);
  return 0;
}
