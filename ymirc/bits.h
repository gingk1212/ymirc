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

static inline uint16_t tobit_16(unsigned int nth) {
  if (nth >= 16) {
    return 0;
  }
  return (uint16_t)1 << nth;
}

static inline bool isset(uint64_t val, unsigned int nth) {
  if (nth >= 64) {
    return false;
  }
  return val >> nth & 1;
}

static inline bool isset_8(uint8_t val, unsigned int nth) {
  if (nth >= 8) {
    return false;
  }
  return val >> nth & 1;
}

static inline bool isset_16(uint16_t val, unsigned int nth) {
  if (nth >= 16) {
    return false;
  }
  return val >> nth & 1;
}

static inline uint64_t concat(uint32_t a, uint32_t b) {
  return ((uint64_t)a << 32) | b;
}

static inline uint64_t concat_64(uint64_t a, uint64_t b) {
  return ((a & 0xFFFFFFFF) << 32) | (b & 0xFFFFFFFF);
}

static inline void set_masked_bits(uint64_t *target, uint64_t val,
                                   uint64_t mask) {
  int shift = __builtin_ctzll(mask);
  *target = (*target & ~mask) | ((val << shift) & mask);
}

static inline uint64_t get_lower_bits(uint64_t val, uint8_t bits) {
  if (bits >= 64) {
    return val;
  }
  return val & ((1ULL << bits) - 1);
}

static inline void set_masked_bits_128(unsigned __int128 *target,
                                       unsigned __int128 val,
                                       unsigned __int128 mask) {
  int shift = __builtin_ctzll(mask);
  *target = (*target & ~mask) | ((val << shift) & mask);
}

static inline unsigned __int128 get_lower_bits_128(unsigned __int128 val,
                                                   uint8_t bits) {
  if (bits >= 128) {
    return val;
  }
  return val & ((1ULL << bits) - 1);
}

#endif  // BITS_H
