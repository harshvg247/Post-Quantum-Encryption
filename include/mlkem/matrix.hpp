#pragma once

#include <array>
#include <cstdint>

#include "mlkem/constants.hpp"
#include "mlkem/sampling.hpp"

namespace mlkem
{

    class Matrix
    {
    public:
        static constexpr std::size_t K = 2; // ML-KEM-512

        using PolyNTT = std::array<std::uint16_t, MLKEM_N>;
        using Mat = std::array<std::array<PolyNTT, K>, K>;

        // ============================================================
        // Generate Â (k x k matrix in NTT domain)
        // ============================================================
        static Mat generate(const std::array<std::uint8_t, 32> &seed)
        {
            Mat A{};

            for (std::size_t i = 0; i < K; ++i)
            {
                for (std::size_t j = 0; j < K; ++j)
                {
                    std::array<std::uint8_t, 34> B{};

                    // copy seed
                    for (std::size_t t = 0; t < 32; ++t)
                        B[t] = seed[t];

                    // append indices
                    B[32] = static_cast<std::uint8_t>(i);
                    B[33] = static_cast<std::uint8_t>(j);

                    A[i][j] = Sampling::sample_ntt(B);
                }
            }

            return A;
        }

        static Mat transpose(const Mat &A)
        {
            Mat AT{};

            for (std::size_t i = 0; i < K; ++i)
                for (std::size_t j = 0; j < K; ++j)
                    AT[i][j] = A[j][i];

            return AT;
        }
    };

} // namespace mlkem