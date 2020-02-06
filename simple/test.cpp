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
