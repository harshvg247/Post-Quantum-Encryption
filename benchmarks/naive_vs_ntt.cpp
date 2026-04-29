#include <array>
#include <chrono>
#include <iostream>
#include <random>

#include "mlkem/ntt.hpp"
#include "mlkem/zq.hpp"
#include "mlkem/polynomial.hpp"

using namespace mlkem;

constexpr std::size_t N = MLKEM_N;

// ------------------------------------------------------------
// Naive multiplication (mod x^N + 1)
// ------------------------------------------------------------
std::array<uint16_t, N> naive_mul(
    const std::array<uint16_t, N>& a,
    const std::array<uint16_t, N>& b)
{
    std::array<uint16_t, N> res{};
    
    for (std::size_t i = 0; i < N; ++i)
    {
        for (std::size_t j = 0; j < N; ++j)
        {
            std::size_t idx = (i + j) % N;

            uint16_t prod = Zq::mul(a[i], b[j]);

            // negacyclic reduction (x^N = -1)
            if (i + j >= N)
                prod = Zq::sub(0, prod);

            res[idx] = Zq::add(res[idx], prod);
        }
    }

    return res;
}

// ------------------------------------------------------------
// NTT-based multiplication
// ------------------------------------------------------------
std::array<uint16_t, N> ntt_mul(
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
// Random polynomial generator
// ------------------------------------------------------------
std::array<uint16_t, N> random_poly()
{
    std::array<uint16_t, N> a{};
    static std::mt19937 rng(42);
    std::uniform_int_distribution<uint16_t> dist(0, MLKEM_Q - 1);

    for (auto& x : a)
        x = dist(rng);

    return a;
}

// ------------------------------------------------------------
// Benchmark helper
// ------------------------------------------------------------
template <typename Func>
double benchmark(Func f, int iterations)
{
    // Pre-generate to avoid benchmarking the RNG
    auto a = random_poly();
    auto b = random_poly();
    volatile uint16_t sink = 0;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i)
    {
        // If f accepts by reference, this is now near-zero stack overhead
        auto res = f(a, b); 
        sink ^= res[0];
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    return diff.count();
}

// ------------------------------------------------------------
// Main
// ------------------------------------------------------------
int main()
{
    int iterations = 100;

    std::cout << "Running benchmark (" << iterations << " iterations)...\n";

    double naive_time = benchmark(naive_mul, iterations);
    double ntt_time   = benchmark(ntt_mul, iterations);

    std::cout << "\nResults:\n";
    std::cout << "Naive: " << naive_time << " sec\n";
    std::cout << "NTT  : " << ntt_time   << " sec\n";

    std::cout << "\nSpeedup: " << naive_time / ntt_time << "x\n";

    return 0;
}

