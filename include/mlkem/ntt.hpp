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