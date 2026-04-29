#pragma once

#include <array>
#include <cstdint>

#include "mlkem/constants.hpp"
#include "mlkem/ntt.hpp"
#include "mlkem/matrix.hpp"

namespace mlkem
{

    class Vector
    {
    public:
        static constexpr std::size_t K = Matrix::K;

        using PolyNTT = std::array<std::uint16_t, MLKEM_N>;
        using Vec = std::array<PolyNTT, K>;

        // ============================================================
        // Zero vector
        // ============================================================
        static Vec zero()
        {
            Vec v{};
            for (auto &poly : v)
                poly.fill(0);
            return v;
        }

        // ============================================================
        // Vector addition: r = a + b
        // ============================================================
        static Vec add(const Vec &a, const Vec &b)
        {
            Vec r{};

            for (std::size_t i = 0; i < K; ++i)
            {
                for (std::size_t j = 0; j < MLKEM_N; ++j)
                {
                    r[i][j] = Zq::add(a[i][j], b[i][j]);
                }
            }

            return r;
        }

        // ============================================================
        // Matrix-vector multiplication in NTT domain
        // t = A * s
        // ============================================================
        static Vec mat_vec_mul(const Matrix::Mat &A,
                               const Vec &s)
        {
            Vec result = zero();

            for (std::size_t i = 0; i < K; ++i)
            {
                for (std::size_t j = 0; j < K; ++j)
                {
                    std::array<std::uint16_t, MLKEM_N> temp{};

                    // Multiply polynomials in NTT domain
                    NTT::multiply_ntts(A[i][j], s[j], temp);

                    // Accumulate
                    for (std::size_t k = 0; k < MLKEM_N; ++k)
                    {
                        result[i][k] =
                            Zq::add(result[i][k], temp[k]);
                    }
                }
            }

            return result;
        }

        // Dot product: result = sum(a[i] * b[i])
        static std::array<uint16_t, MLKEM_N>
        dot(const Vec &a, const Vec &b)
        {
            std::array<uint16_t, MLKEM_N> result{};
            result.fill(0);

            for (std::size_t i = 0; i < K; ++i)
            {
                std::array<uint16_t, MLKEM_N> temp{};
                NTT::multiply_ntts(a[i], b[i], temp);

                for (std::size_t j = 0; j < MLKEM_N; ++j)
                {
                    result[j] = Zq::add(result[j], temp[j]);
                }
            }

            return result;
        }
    };

} // namespace mlkem