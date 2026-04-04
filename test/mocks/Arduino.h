#pragma once

#include "Stream.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Basic types
typedef uint8_t byte;
typedef bool boolean;

// Constants
#define HIGH         1
#define LOW          0
#define INPUT        0
#define OUTPUT       1
#define INPUT_PULLUP 2
#define MSBFIRST     0
#define LSBFIRST     1

#define DEC          10
#define HEX          16
#define OCT          8
#define BIN          2

// Time functions
inline uint32_t millis() {
  auto now = std::chrono::steady_clock::now();
  auto duration = now.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

inline uint32_t micros() {
  auto now = std::chrono::steady_clock::now();
  auto duration = now.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

inline void delay(uint32_t ms) {
  // No-op for tests or sleep
}

inline void delayMicroseconds(uint32_t us) {
  // No-op
}

inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

inline long random(long min, long max) {
  return min + (rand() % (max - min));
}

inline long random(long max) {
  return rand() % max;
}

inline char* ltoa(long val, char* s, int radix) {
  if (radix == 10) {
    sprintf(s, "%ld", val);
  } else if (radix == 16) {
    sprintf(s, "%lx", val);
  } else {
    sprintf(s, "%ld", val); // fallback
  }
  return s;
}
