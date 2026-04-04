#pragma once

#include <cstdint>
#include <cstring>
#include <string>

class Stream {
public:
  virtual int available() { return 0; }
  virtual int read() { return -1; }
  virtual int peek() { return -1; }
  virtual void flush() {}
  virtual size_t write(uint8_t) { return 1; }
  virtual size_t write(const uint8_t *buffer, size_t size) { return size; }
  virtual size_t readBytes(uint8_t *buffer, size_t length) {
    for (size_t i = 0; i < length; i++) {
      int c = read();
      if (c < 0) return i;
      buffer[i] = c;
    }
    return length;
  }

  // Print methods
  size_t print(const char *s) { return strlen(s); }
  size_t print(char c) { return 1; }
  size_t print(int n) { return 1; }
  size_t println(const char *s) { return print(s) + 1; }
  size_t println(char c) { return print(c) + 1; }
  size_t println(int n) { return print(n) + 1; }
  size_t println() { return 1; }
  
  size_t printf(const char *format, ...) {
    return 1;
  }
};
