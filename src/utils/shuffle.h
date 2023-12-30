#include <random>
#include <vector>

template <class T> void shuffle(std::vector<T> &v, std::mt19937 &rng) {
  for (int i = v.size() - 1; i >= 0; i--) {
    int j = rng() % (i + 1);
    std::swap(v[i], v[j]);
  }
}