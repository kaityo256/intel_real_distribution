# Optimization of the Intel Compiler

## Introduction

I've found that some codes compiled by the Intel compiler are significantly slower than those by GCC.

[https://github.com/kaityo256/compiler_test](https://github.com/kaityo256/compiler_test)

This is a Monte Carlo simulation code of the Potts model, which is one of the fundamental models in statistical physics.

While investigating the cause of this problem, I arrived at the simple code.

```cpp
#include <iostream>
#include <random>

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

int main() {
  std::cout << run() << std::endl;
}
```

The benchmark results are as follows.

* Environment
  * CPU: Intel(R) Xeon(R) Gold 6230 CPU @ 2.10GHz
  * OS: CentOS Linux release 7.6.1810 (Core)
  * GCC: g++ (GCC) 7.3.1 20180303 (Red Hat 7.3.1-5)
  * icpc: icpc (ICC) 19.0.4.243 20190416

```sh
$ g++ -O3 -march=native -Wall -Wextra -std=c++11 test.cpp -o gcc.out
$ icpc -O3 -xHOST -Wall -Wextra -std=c++11  test.cpp -o icpc.out
$ time ./gcc.out
-6698.66
./gcc.out  0.33s user 0.00s system 99% cpu 0.335 total

$ time ./icpc.out
-6698.66
./icpc.out  2.93s user 0.00s system 99% cpu 2.939 total
```

The two executables give identical results, but the executable produced by the Intel compiler is significantly slower than that by GCC.

From the detailed investigation described later, I found that the weird behavior will happen under the following conditions.

* A loop contains if-statement.
* The if-statements contains a function call of `std::uniform_real_distribution`.
* This behavior does not depend on the type of the pseudo-random generator.

## Detailed investigation

### Pseudo-random Generator

First, I suspect that this is caused by the pseudo-random generator (PRG). So I changed the Mersenne Twister to other PRG.

We can adopt several PNGs, such as the linear congruential method (`std::minstd_rand0`) and the RANLUX method (`std::ranlux24_base`). I also checked the Xorshift method. But the situation does not change even if we changed the PNG.

Additionally, I defined the PNG, which always returns zero.

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

But the Intel compiler still generates a significantly slower executable than that by GCC.

### If-statement

If the loop does not contain the if-statement, then this weird behavior does not occur.

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

If we replace `std::uniform_real_distribution` by other functions, then the weird behavior disappears.

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

[Taken from boost](https://github.com/boostorg/random/blob/develop/include/boost/random/uniform_real_distribution.hpp
)

```cpp
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

In the bose cases, the Intel compiler generates faster (or acceptable) executables than that by GCC.

## Summary

Here is the table of the summary.

* Environment
  * CPU: Intel(R) Xeon(R) Gold 6230 CPU @ 2.10GHz
  * OS: CentOS Linux release 7.6.1810 (Core)
  * GCC: g++ (GCC) 7.3.1 20180303 (Red Hat 7.3.1-5)
  * icpc: icpc (ICC) 19.0.4.243 20190416

| PNG | Distribution| if-statement| gcc [ms]| icpc [ms] |
|--- | --- | --- | --- | --- |
| `std::mt19937` | `std::uniform_real_distribution`| Included | 523 | 2933 |
| `std::minstd_rand0` | `std::uniform_real_distribution` | Included | 410 | 2770 |
| `std::ranlux24_base` | `std::uniform_real_distribution` | Included | 782 | 2997 |
| Xorshift | `std::uniform_real_distribution` | Included | 188 | 2757 |
| Always Zero | `std::uniform_real_distribution` | Included | 52 | 2707 |
| `std::mt19937` | `std::uniform_int_distribution` | Included | 819 | 948 |
| `std::mt19937` | `std::uniform_real_distribution` | Not included | 1043 | 612 |
| `std::mt19937` | user defined | Included | 529 | 273 |

From the above results, we found that the optimizer of the Intel compiler exhibits weird behavior when the loop contains an if-statement and the if-statements contains `std::uniform_real_distribution`.

## How to build

```sh
make
make run
```

Here is the raw output.

```sh
./gcc.out
mt          + real   + if Result = -6698.66 Elapsed = 523 [ms]
linear      + real   + if Result = -1084.15 Elapsed = 410 [ms]
subtract    + real   + if Result = -2890.53 Elapsed = 782 [ms]
xorshift    + real   + if Result = -5872.91 Elapsed = 188 [ms]
always_zero + real   + if Result = -5e+07 Elapsed = 52 [ms]
mt          + int    + if Result = 508885 Elapsed = 819 [ms]
mt          + real   - if Result = -8741.89 Elapsed = 1043 [ms]
mt          + myreal + if Result = -4523.55 Elapsed = 529 [ms]
./icpc.out
mt          + real   + if Result = -6698.66 Elapsed = 2933 [ms]
linear      + real   + if Result = -1084.15 Elapsed = 2770 [ms]
subtract    + real   + if Result = -2890.53 Elapsed = 2997 [ms]
xorshift    + real   + if Result = -5872.91 Elapsed = 2757 [ms]
always_zero + real   + if Result = -5e+07 Elapsed = 2707 [ms]
mt          + int    + if Result = 508885 Elapsed = 948 [ms]
mt          + real   - if Result = -8741.89 Elapsed = 612 [ms]
mt          + myreal + if Result = -4523.55 Elapsed = 273 [ms]
```
