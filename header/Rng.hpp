#ifndef RNG_HPP
#define RNG_HPP

#include <algorithm>
#include <cstdint>
#include <random>

// Unica fonte de aleatoriedade do projeto: injetada nos algoritmos para reproduzir experimentos.
class Rng {
    std::mt19937_64 g_;

public:
    explicit Rng(uint64_t semente = 12345) : g_(semente) {}
    void semente(uint64_t s) { g_.seed(s); }
    int inteiro(int n) { return n <= 1 ? 0 : std::uniform_int_distribution<int>(0, n - 1)(g_); }
    double real() { return std::uniform_real_distribution<double>(0.0, 1.0)(g_); }
    template <class It> void embaralhar(It a, It b) { std::shuffle(a, b, g_); }
};

#endif
