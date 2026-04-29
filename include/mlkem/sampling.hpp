#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "mlkem/constants.hpp"
#include "mlkem/zq.hpp"
#include "mlkem/shake.hpp"   // your Crypto++ wrapper

namespace mlkem {

class Sampling {
public:
    // ============================================================
    // Algorithm 7 — SampleNTT
    // Takes a 32-byte seed and two indices as input and outputs a pseudorandom element of 𝑇q
    // Input: 34 bytes (32-byte seed || 2 index bytes)
    // Output: a[256] in Z_q (NTT domain representation)
    // ============================================================
    static std::array<std::uint16_t, MLKEM_N>
    sample_ntt(const std::array<std::uint8_t, 34>& B)
    {
        std::array<std::uint16_t, MLKEM_N> a{};

        // 1) Absorb
        SHAKE128 shake;
        shake.absorb(B.data(), B.size());

        // 2) Squeeze a sufficiently large buffer once
        // Empirically, ~768 bytes is enough; we use 1024 for safety.
        constexpr std::size_t BUF_SIZE = 1024;
        std::array<std::uint8_t, BUF_SIZE> buf{};
        shake.squeeze(buf.data(), buf.size());

        // 3) Rejection sampling from 12-bit chunks (2 per 3 bytes)
        std::size_t j = 0;
        std::size_t pos = 0;

        while (j < MLKEM_N) {
            // If we run out of buffer, in spec we'd keep squeezing.
            // For this clean version, we assume BUF_SIZE is sufficient.
            // (Later we can extend to refill.)

            // extracting 3 bytes here to generate 2 numbers(d1 and d2) of 12 bits each
            std::uint8_t c0 = buf[pos++];
            std::uint8_t c1 = buf[pos++];
            std::uint8_t c2 = buf[pos++];


            // C[0]: 8 bits
            // C[1]: split into:
            // lower 4 bits → used for d1
            // upper 4 bits → used for d2
            // C[2]: 8 bits

            // d1 = C[0] + 256 * (C[1] mod 16)
            std::uint16_t d1 =
                static_cast<std::uint16_t>(c0)
              + static_cast<std::uint16_t>(256 * (c1 & 0x0F));

            // d2 = floor(C[1]/16) + 16 * C[2]
            std::uint16_t d2 =
                static_cast<std::uint16_t>(c1 >> 4)
              + static_cast<std::uint16_t>(16 * c2);

            if (d1 < MLKEM_Q) {
                a[j++] = d1;
            }
            if (d2 < MLKEM_Q && j < MLKEM_N) {
                a[j++] = d2;
            }
        }
        return a;
    }

    // This algorithm is a bit-level trick to simulate a 
    // symmetric, small-noise distribution efficiently, without costly math or bias

    // ============================================================
    // Algorithm 8 — SamplePolyCBD
    // Template parameter ETA ∈ {2,3}
    // Input: B of length 64*ETA bytes
    // Output: f[256] in Z_q with small coefficients
    // ============================================================
    template <int ETA>
    static std::array<std::uint16_t, MLKEM_N>
    sample_poly_cbd(const std::vector<std::uint8_t>& B)
    {
        static_assert(ETA == 2 || ETA == 3,
                      "ETA must be 2 or 3");

        std::array<std::uint16_t, MLKEM_N> f{};

        // 1) Bytes → bits (LSB-first per byte)
        std::vector<std::uint8_t> bits;
        bits.reserve(B.size() * 8);

        for (std::uint8_t byte : B) {
            for (int k = 0; k < 8; ++k) {
                bits.push_back((byte >> k) & 1u);
            }
        }

        // 2) For each coefficient
        for (std::size_t i = 0; i < MLKEM_N; ++i) {
            std::uint16_t x = 0;
            std::uint16_t y = 0;

            // x = sum of ETA bits
            for (int j = 0; j < ETA; ++j) {
                x += bits[2 * i * ETA + j];
            }

            // y = sum of next ETA bits
            for (int j = 0; j < ETA; ++j) {
                y += bits[2 * i * ETA + ETA + j];
            }

            // value = x - y (centered)
            std::int16_t val =
                static_cast<std::int16_t>(x)
              - static_cast<std::int16_t>(y);

            // map to Z_q
            if (val < 0)
                val += MLKEM_Q;

            f[i] = static_cast<std::uint16_t>(val);
        }

        return f;
    }
};

} // namespace mlkem