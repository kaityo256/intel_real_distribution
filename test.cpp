#include <chrono>
#include <iostream>
#include <random>
#include <string>

using t_run = double (*)();

// Baseline
// Mersenne Twister (MT)
double run(void) {
  std::mt19937 mt(1);
  double r = 0.0;
  std::uniform_real_distribution<> ud(-1.0, 1.0);
  for (int j = 0; j < 10000; j++) {
    for (int i = 0; i < 10000; i++) {
      if (i % 2) r += ud(mt);
    }
  }
  return r;
}

// Linear congruential method
double run_linear(void) {
  std::minstd_rand0 mt(1);
  double r = 0.0;
  std::uniform_real_distribution<> ud(-1.0, 1.0);
  for (int j = 0; j < 10000; j++) {
    for (int i = 0; i < 10000; i++) {
      if (i % 2) r += ud(mt);
    }
  }
  return r;
}

// Subtract-with-carry (RANLUX) method
double run_subtract(void) {
  std::ranlux24_base mt(1);
  double r = 0.0;
  std::uniform_real_distribution<> ud(-1.0, 1.0);
  for (int j = 0; j < 10000; j++) {
    for (int i = 0; i < 10000; i++) {
      if (i % 2) r += ud(mt);
    }
  }
  return r;
}

// Xorshift method
class xorshift {
public:
  uint32_t operator()() {
    static uint32_t y = 2463534242;
    y = y ^ (y << 13);
    y = y ^ (y >> 17);
    return y = y ^ (y << 5);
  }
  static constexpr uint32_t max() {
    return std::mt19937::max();
  }
  static constexpr uint32_t min() {
    return 0;
  }
};

double run_xorshift(void) {
  xorshift mt;
  double r = 0.0;
  std::uniform_real_distribution<> ud(-1.0, 1.0);
  for (int j = 0; j < 10000; j++) {
    for (int i = 0; i < 10000; i++) {
      if (i % 2) r += ud(mt);
    }
  }
  return r;
}

// PRG returns always zero.
class always_zero {
public:
  uint32_t operator()() {
    return 0;
  }
  static constexpr uint32_t max() {
    return std::mt19937::max();
  }
  static constexpr uint32_t min() {
    return 0;
  }
};

double run_always_zero(void) {
  always_zero mt;
  double r = 0.0;
  std::uniform_real_distribution<> ud(-1.0, 1.0);
  for (int j = 0; j < 10000; j++) {
    for (int i = 0; i < 10000; i++) {
      if (i % 2) r += ud(mt);
    }
  }
  return r;
}

double run_without_if(void) {
  std::mt19937 mt(1);
  double r = 0.0;
  std::uniform_real_distribution<> ud(-1.0, 1.0);
  for (int j = 0; j < 10000; j++) {
    for (int i = 0; i < 10000; i++) {
      // if (i % 2) r += ud(mt);
      r += ud(mt);
    }
  }
  return r;
}

double run_int(void) {
  std::mt19937 mt(1);
  int r;
  std::uniform_int_distribution<> ud(-100, 100);
  for (int j = 0; j < 10000; j++) {
    for (int i = 0; i < 10000; i++) {
      if (i % 2) r += ud(mt);
    }
  }
  return static_cast<double>(r);
}

// https://github.com/boostorg/random/blob/develop/include/boost/random/uniform_real_distribution.hpp
double my_uniform_real(std::mt19937 &mt, double min_value, double max_value) {
  for (;;) {
    double numerator = static_cast<double>(mt() - mt.min());
    double divisor = static_cast<double>(mt.max() - mt.min());
    double result = numerator / divisor * (max_value - min_value) + min_value;
    if (result < max_value) return result;
  }
}

double run_my_distribution(void) {
  std::mt19937 mt(1);
  double r = 0.0;
  for (int j = 0; j < 10000; j++) {
    for (int i = 0; i < 10000; i++) {
      r += my_uniform_real(mt, -1.0, 1.0);
    }
  }
  return r;
}

void measure(t_run run, std::string title) {
  auto start = std::chrono::system_clock::now();
  double r = run();
  auto end = std::chrono::system_clock::now();
  double elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  std::cout << title << " ";
  std::cout << "Result = " << r << " ";
  std::cout << "Elapsed = " << elapsed << " [ms]";
  std::cout << std::endl;
}

int main() {
  // slow
  measure(run, "mt          + real   + if");
  measure(run_linear, "linear      + real   + if");
  measure(run_subtract, "subtract    + real   + if");
  measure(run_xorshift, "xorshift    + real   + if");
  measure(run_always_zero, "always_zero + real   + if");
  // fast
  measure(run_int, "mt          + int    + if");
  measure(run_without_if, "mt          + real   - if");
  measure(run_my_distribution, "mt          + myreal + if");
}
