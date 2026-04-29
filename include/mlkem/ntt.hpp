#pragma once

#include <array>
#include <cstdint>

#include "mlkem/constants.hpp"
#include "mlkem/zq.hpp"

namespace mlkem
{

    class NTT
    {
    public:
        static constexpr std::array<uint16_t, 128> ZETAS = {
            1, 1729, 2580, 3289, 2642, 630, 1897, 848,
            1062, 1919, 193, 797, 2786, 3260, 569, 1746,
            296, 2447, 1339, 1476, 3046, 56, 2240, 1333,
            1426, 2094, 535, 2882, 2393, 2879, 1974, 821,
            289, 331, 3253, 1756, 1197, 2304, 2277, 2055,
            650, 1977, 2513, 632, 2865, 33, 1320, 1915,
            2319, 1435, 807, 452, 1438, 2868, 1534, 2402,
            2647, 2617, 1481, 648, 2474, 3110, 1227, 910,
            17, 2761, 583, 2649, 1637, 723, 2288, 1100,
            1409, 2662, 3281, 233, 756, 2156, 3015, 3050,
            1703, 1651, 2789, 1789, 1847, 952, 1461, 2687,
            939, 2308, 2437, 2388, 733, 2337, 268, 641,
            1584, 2298, 2037, 3220, 375, 2549, 2090, 1645,
            1063, 319, 2773, 757, 2099, 561, 2466, 2594,
            2804, 1092, 403, 1026, 1143, 2150, 2775, 886,
            1722, 1212, 1874, 1029, 2110, 2935, 885, 2154};

        static constexpr std::array<uint16_t, 128> GAMMAS = {
            17, 3312,
            2761, 568,
            583, 2746,
            2649, 680,
            1637, 1692,
            723, 2606,
            2288, 1041,
            1100, 2229,
            1409, 1920,
            2662, 667,
            3281, 48,
            233, 3096,
            756, 2573,
            2156, 1173,
            3015, 314,
            3050, 279,
            1703, 1626,
            1651, 1678,
            2789, 540,
            1789, 1540,
            1847, 1482,
            952, 2377,
            1461, 1868,
            2687, 642,
            939, 2390,
            2308, 1021,
            2437, 892,
            2388, 941,
            733, 2596,
            2337, 992,
            268, 3061,
            641, 2688,
            1584, 1745,
            2298, 1031,
            2037, 1292,
            3220, 109,
            375, 2954,
            2549, 780,
            2090, 1239,
            1645, 1684,
            1063, 2266,
            319, 3010,
            2773, 556,
            757, 2572,
            2099, 1230,
            561, 2768,
            2466, 863,
            2594, 735,
            2804, 525,
            1092, 2237,
            403, 2926,
            1026, 2303,
            1143, 2186,
            2150, 1179,
            2775, 554,
            886, 2443,
            1722, 1607,
            1212, 2117,
            1874, 1455,
            1029, 2300,
            2110, 1219,
            2935, 394,
            885, 2444,
            2154, 1175};

        // Forward NTT
        static void forward(std::array<std::uint16_t, MLKEM_N> &f) noexcept
        {
            std::size_t i = 1;

            for (std::size_t len = 128; len >= 2; len >>= 1)
            {
                for (std::size_t start = 0; start < MLKEM_N; start += 2 * len)
                {
                    std::uint16_t zeta = ZETAS[i];
                    i++;
                    for (std::size_t j = start; j < start + len; ++j)
                    {
                        std::uint16_t t = Zq::mul(zeta, f[j + len]);
                        f[j + len] = Zq::sub(f[j], t);
                        f[j] = Zq::add(f[j], t);
                    }
                }
            }
        }

        // Inverse NTT
        static void inverse(std::array<std::uint16_t, MLKEM_N> &f) noexcept
        {
            std::size_t i = 127;
            for (std::size_t len = 2; len <= 128; len <<= 1)
            {
                for (std::size_t start = 0; start < MLKEM_N; start += 2 * len)
                {
                    std::uint16_t zeta = ZETAS[i];
                    i--;
                    for (std::size_t j = start; j < start + len; ++j)
                    {
                        std::uint16_t t = f[j];
                        f[j] = Zq::add(t, f[j + len]);
                        std::uint16_t u = Zq::sub(f[j + len], t);
                        f[j + len] = Zq::mul(zeta, u);
                    }
                }
            }

            // Final scaling by 3303 (256^{-1} mod 3329)
            constexpr std::uint16_t INV_128 = 3303;
            for (std::size_t j = 0; j < MLKEM_N; ++j)
            {
                f[j] = Zq::mul(f[j], INV_128);
            }
        }

        static std::pair<std::uint16_t, std::uint16_t>
        base_case_multiply(std::uint16_t a0,
                           std::uint16_t a1,
                           std::uint16_t b0,
                           std::uint16_t b1,
                           std::uint16_t gamma) noexcept
        {
            // c0 = a0*b0 + a1*b1*gamma
            std::uint16_t c0 = Zq::add(Zq::mul(a0, b0), Zq::mul(Zq::mul(a1, b1), gamma));

            // c1 = a0*b1 + a1*b0
            std::uint16_t c1 = Zq::add(Zq::mul(a0, b1), Zq::mul(a1, b0));

            return {c0, c1};
        }
        
        static void multiply_ntts(
            const std::array<std::uint16_t, MLKEM_N> &f,
            const std::array<std::uint16_t, MLKEM_N> &g,
            std::array<std::uint16_t, MLKEM_N> &h) noexcept
        {
            for (std::size_t i = 0; i < 128; ++i)
            {
                std::uint16_t gamma = GAMMAS[i];

                auto [c0, c1] =
                    base_case_multiply(
                        f[2 * i],
                        f[2 * i + 1],
                        g[2 * i],
                        g[2 * i + 1],
                        gamma);

                h[2 * i] = c0;
                h[2 * i + 1] = c1;
            }
        }
    };

    class oldNTT
    {
    public:
        // Bit reverse helper (7-bit indices for 0..127)
        static std::size_t bit_reverse7(std::size_t x) noexcept
        {
            std::size_t r = 0;
            for (int i = 0; i < 7; ++i)
            {
                r = (r << 1) | (x & 1);
                x >>= 1;
            }
            return r;
        }

        // Compute zeta^k mod q  (zeta = 17)
        static std::uint16_t zeta_pow(std::size_t k) noexcept
        {
            return Zq::mod_pow(17, static_cast<std::uint32_t>(k));
        }

        // compute zeta^{bit_reverse7(i)}
        static std::uint16_t zeta_bitrev7(std::size_t i) noexcept
        {
            return zeta_pow(bit_reverse7(i));
        }

        // Forward NTT
        static void forward(std::array<std::uint16_t, MLKEM_N> &f) noexcept
        {
            std::size_t i = 1;

            for (std::size_t len = 128; len >= 2; len >>= 1)
            {
                for (std::size_t start = 0; start < MLKEM_N; start += 2 * len)
                {
                    std::uint16_t zeta = zeta_bitrev7(i);
                    i++;
                    for (std::size_t j = start; j < start + len; ++j)
                    {
                        std::uint16_t t = Zq::mul(zeta, f[j + len]);
                        f[j + len] = Zq::sub(f[j], t);
                        f[j] = Zq::add(f[j], t);
                    }
                }
            }
        }

        // Inverse NTT
        static void inverse(std::array<std::uint16_t, MLKEM_N> &f) noexcept
        {
            std::size_t i = 127;
            for (std::size_t len = 2; len <= 128; len <<= 1)
            {
                for (std::size_t start = 0; start < MLKEM_N; start += 2 * len)
                {
                    std::uint16_t zeta = zeta_bitrev7(i);
                    i--;
                    for (std::size_t j = start; j < start + len; ++j)
                    {
                        std::uint16_t t = f[j];
                        f[j] = Zq::add(t, f[j + len]);
                        std::uint16_t u = Zq::sub(f[j + len], t);
                        f[j + len] = Zq::mul(zeta, u);
                    }
                }
            }

            // Final scaling by 3303 (256^{-1} mod 3329)
            constexpr std::uint16_t INV_128 = 3303;
            for (std::size_t j = 0; j < MLKEM_N; ++j)
            {
                f[j] = Zq::mul(f[j], INV_128);
            }
        }

        static std::pair<std::uint16_t, std::uint16_t>
        base_case_multiply(std::uint16_t a0,
                           std::uint16_t a1,
                           std::uint16_t b0,
                           std::uint16_t b1,
                           std::uint16_t gamma) noexcept
        {
            // c0 = a0*b0 + a1*b1*gamma
            std::uint16_t c0 = Zq::add(Zq::mul(a0, b0), Zq::mul(Zq::mul(a1, b1), gamma));

            // c1 = a0*b1 + a1*b0
            std::uint16_t c1 = Zq::add(Zq::mul(a0, b1), Zq::mul(a1, b0));

            return {c0, c1};
        }
        static std::uint16_t
        gamma_value(std::size_t i) noexcept
        {
            std::size_t exp =
                2 * bit_reverse7(i) + 1;

            return Zq::mod_pow(17,
                               static_cast<std::uint32_t>(exp));
        }
        static void multiply_ntts(
            const std::array<std::uint16_t, MLKEM_N> &f,
            const std::array<std::uint16_t, MLKEM_N> &g,
            std::array<std::uint16_t, MLKEM_N> &h) noexcept
        {
            for (std::size_t i = 0; i < 128; ++i)
            {
                std::uint16_t gamma = gamma_value(i);

                auto [c0, c1] =
                    base_case_multiply(
                        f[2 * i],
                        f[2 * i + 1],
                        g[2 * i],
                        g[2 * i + 1],
                        gamma);

                h[2 * i] = c0;
                h[2 * i + 1] = c1;
            }
        }
    };

} // namespace mlkem