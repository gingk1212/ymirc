#ifndef BITS_H
#define BITS_H

#include <stdbool.h>
#include <stdint.h>

static inline uint64_t tobit(unsigned int nth) {
  if (nth >= 64) {
    return 0;
  }
  return (uint64_t)1 << nth;
}

static inline bool isset(uint64_t val, unsigned int nth) {
  if (nth >= 64) {
    return false;
  }
  return val >> nth & 1;
}

static inline uint64_t concat(uint32_t a, uint32_t b) {
  return ((uint64_t)a << 32) | b;
}

#endif  // BITS_H
