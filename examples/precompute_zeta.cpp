#include <array>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

#include "mlkem/ntt.hpp"
#include "mlkem/zq.hpp"

using namespace mlkem;

constexpr std::size_t N = MLKEM_N;
constexpr std::size_t NUM_SAMPLES = 512;

// ------------------------------------------------------------
// Dataset (pre-generated)
// ------------------------------------------------------------
struct PolyPair
{
    std::array<uint16_t, N> a;
    std::array<uint16_t, N> b;
};

std::vector<PolyPair> dataset;

void generate_dataset()
{
    std::mt19937 rng(42);
    std::uniform_int_distribution<uint16_t> dist(0, MLKEM_Q - 1);

    dataset.resize(NUM_SAMPLES);

    for (auto &p : dataset)
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            p.a[i] = dist(rng);
            p.b[i] = dist(rng);
        }
    }
}

// ------------------------------------------------------------
// Benchmark function
// ------------------------------------------------------------
template <typename Func>
double benchmark(Func f, int iterations)
{
    volatile uint16_t sink = 0;

    // Warmup
    for (int i = 0; i < 10; ++i)
    {
        for (auto &p : dataset)
        {
            auto a = p.a;
            auto b = p.b;
            auto res = f(a, b);
            sink ^= res[0];
        }
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (int it = 0; it < iterations; ++it)
    {
        for (auto &p : dataset)
        {
            auto a = p.a;
            auto b = p.b;
            auto res = f(a, b);
            sink ^= res[0];
        }
    }

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> diff = end - start;

    // Prevent optimization
    if (sink == 12345)
        std::cout << "";

    return diff.count();
}

// ------------------------------------------------------------
// Optimized (ZETAS + GAMMAS)
// ------------------------------------------------------------
std::array<uint16_t, N> mul_new(
    std::array<uint16_t, N> a,
    std::array<uint16_t, N> b)
{
    NTT::forward(a);
    NTT::forward(b);

    std::array<uint16_t, N> h{};
    NTT::multiply_ntts(a, b, h);

    NTT::inverse(h);
    return h;
}

// ------------------------------------------------------------
// Baseline (no tables)
// ------------------------------------------------------------
std::array<uint16_t, N> mul_old(
    std::array<uint16_t, N> a,
    std::array<uint16_t, N> b)
{
    oldNTT::forward(a);
    oldNTT::forward(b);

    std::array<uint16_t, N> h{};
    oldNTT::multiply_ntts(a, b, h);

    oldNTT::inverse(h);
    return h;
}

// ------------------------------------------------------------
// Main
// ------------------------------------------------------------
int main()
{
    int iterations = 10;

    generate_dataset();

    std::cout << "Running full NTT multiplication benchmark...\n";

    double new_time = benchmark(mul_new, iterations);
    double old_time = benchmark(mul_old, iterations);

    std::cout << "\nResults:\n";
    std::cout << "With tables : " << new_time << " sec\n";
    std::cout << "Without     : " << old_time << " sec\n";

    std::cout << "\nSpeedup: " << old_time / new_time << "x\n";

    return 0;
}