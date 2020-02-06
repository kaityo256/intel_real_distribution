# Optimization of the Intel Compiler

## Introduction

I've found that some codes compiled by the Intel compiler are significantly slower than those by GCC.

[https://github.com/kaityo256/compiler_test](https://github.com/kaityo256/compiler_test)

This is a Monte Carlo simulation code of the Potts model which is one of the fundamental models in statistical physics.

While investigating the cause of this problem, I arrived at the simple code.

```cpp
#include <iostream>
#include <random>

double run(void) {
  std::mt19937 mt;
  double r = 0.0;
  std::uniform_real_distribution<> ud(-1.0, 1.0);
  for (int j = 0; j < 10000; j++) {
    for (int i = 0; i < 10000; i++) {
      if (i % 2) r += ud(mt);
    }
  }
  return r;
}

int main() {
  std::cout << run() << std::endl;
}
```

The benchmark results are as follows.

* Envrionment
  * CPU: Intel(R) Xeon(R) Gold 6230 CPU @ 2.10GHz
  * OS: CentOS Linux release 7.6.1810 (Core)
  * GCC: g++ (GCC) 7.3.1 20180303 (Red Hat 7.3.1-5)
  * icpc: icpc (ICC) 19.0.4.243 20190416

```sh
$ g++ -O3 -march=native -Wall -Wextra -std=c++11 test.cpp -o gcc.out
$ icpc -O3 -xHOST -Wall -Wextra -std=c++11  test.cpp -o icpc.out
$ time ./gcc.out
-1884.26
./gcc.out  0.33s user 0.00s system 99% cpu 0.333 total

$ time ./icpc.out
-1884.26
./icpc.out  2.92s user 0.00s system 99% cpu 2.932 total
```

The two executables give the identical result, but the executable produced by Intel compiler is significantly slower than that by GCC.

From the detailed investigation described later, I found that the weird behavior will happen under the following conditions.

* A loop contains if-statement.
* The if-statements contains a function call of `std::uniform_real_distribution`.
* This behavior does not depend on the type of the pseudo-random generator.

## Detailed investigation

### Pseudo-random Generator

First, I suspect that this is caused by the pseudo-random generator (PRG). So I changed the Mersenne Twister to other PRG.

We can adopt several PNGs, such as the linear congruential method (`std::minstd_rand0`) and the RANLUX method (`std::ranlux24_base`). I also checked the Xorshift method. But the situation does not changed even if we changed the PNG.

Additionally, I defined the PNG which returns always zero.

```cpp
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
```

But the Intel compiler still generates a slower executable than that by GCC.

### If-statement

If the loop does not contains the if-statement, then this weird behavior does not occur.

```cpp
double run_without_if(void) {
  std::mt19937 mt(1);
  double r = 0.0;
  std::uniform_real_distribution<> ud(-1.0, 1.0);
  for (int j = 0; j < 10000; j++) {
    for (int i = 0; i < 10000; i++) {
      r += ud(mt);
    }
  }
  return r;
}
```

### Distribution

If we replace `std::uniform_real_distribution` by other functions, then the weird behavior dissapiars.

* Use `std::uniform_int_distribution`.

```cpp
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
```

* Use user-defined distribution

```cpp
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
```

In the bose cases, the Intel compiler generates faster executables than that by GCC.

