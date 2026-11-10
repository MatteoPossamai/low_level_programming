#include <cstdint>
#include <cstdio>
#include <immintrin.h>

int main() {
  __m256i val = _mm256_set_epi32(0, 1, 2, 3, 4, 5, 6, 7);
  // Pay attention to endian-ness

  alignas(32) int32_t values[8];
  _mm256_store_si256(reinterpret_cast<__m256i *>(values), val);

  for (int i = 0; i < 8; i++)
    printf("%d\n", values[i]);
}
