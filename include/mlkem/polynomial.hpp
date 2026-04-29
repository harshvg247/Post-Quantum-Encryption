#pragma once

#include <array>
#include <cstdint>
#include <algorithm>

#include "mlkem/constants.hpp"
#include "mlkem/zq.hpp"
#include "mlkem/ntt.hpp"

// std::array instead of std::vector Because:
//   Size is fixed at compile-time (256)
//   No heap allocation

namespace mlkem
{

    class Polynomial
    {
    public:
        using coefficient_type = std::uint16_t;
        using container_type = std::array<coefficient_type, MLKEM_N>;

    private:
        container_type coeffs_;

    public:
        // Default constructor: zero polynomial
        Polynomial() noexcept
        {
            coeffs_.fill(0);
        }

        // Access operator (read/write)
        coefficient_type &operator[](std::size_t i) noexcept
        {
            return coeffs_[i];
        }

        // Access operator (read-only)
        const coefficient_type &operator[](std::size_t i) const noexcept
        {
            return coeffs_[i];
        }

        // Addition: this += other
        void add(const Polynomial &other) noexcept
        {
            for (std::size_t i = 0; i < MLKEM_N; ++i)
            {
                coeffs_[i] = Zq::add(coeffs_[i], other.coeffs_[i]);
            }
        }

        // Subtraction: this -= other
        void sub(const Polynomial &other) noexcept
        {
            for (std::size_t i = 0; i < MLKEM_N; ++i)
            {
                coeffs_[i] = Zq::sub(coeffs_[i], other.coeffs_[i]);
            }
        }

        // Multiply polynomial by scalar in Zq
        void scalar_mul(coefficient_type scalar) noexcept
        {
            for (std::size_t i = 0; i < MLKEM_N; ++i)
            {
                coeffs_[i] = Zq::mul(coeffs_[i], scalar);
            }
        }

        // Secure zeroization (important later)
        void secure_zero() noexcept
        {
            coeffs_.fill(0);
        }

        const container_type &data() const noexcept
        {
            return coeffs_;
        }

        static Polynomial multiply(const Polynomial &a,
                                   const Polynomial &b)
        {
            Polynomial result;

            // Temporary array for full convolution
            std::array<std::int64_t, 2 * MLKEM_N> temp{};
            temp.fill(0);

            // Step 1: Full convolution
            for (std::size_t i = 0; i < MLKEM_N; ++i)
            {
                for (std::size_t j = 0; j < MLKEM_N; ++j)
                {
                    temp[i + j] += static_cast<std::int64_t>(a.coeffs_[i]) *
                                   static_cast<std::int64_t>(b.coeffs_[j]);
                }
            }

            // Step 2: Negacyclic folding in integer domain
            for (std::size_t k = MLKEM_N; k < 2 * MLKEM_N; ++k)
            {
                temp[k - MLKEM_N] -= temp[k];
            }

            // Step 3: Final reduction into result polynomial
            for (std::size_t i = 0; i < MLKEM_N; ++i)
            {
                temp[i] %= MLKEM_Q;
                if (temp[i] < 0)
                    temp[i] += MLKEM_Q;

                result.coeffs_[i] =
                    static_cast<std::uint16_t>(temp[i]);
            }
            return result;
        }

        static Polynomial multiply_ntt(const Polynomial &a,
                                       const Polynomial &b) noexcept
        {
            Polynomial result;

            std::array<std::uint16_t, MLKEM_N> fa{};
            std::array<std::uint16_t, MLKEM_N> fb{};
            std::array<std::uint16_t, MLKEM_N> fh{};

            for (std::size_t i = 0; i < MLKEM_N; ++i)
            {
                fa[i] = a.coeffs_[i];
                fb[i] = b.coeffs_[i];
            }

            NTT::forward(fa);
            NTT::forward(fb);

            NTT::multiply_ntts(fa, fb, fh);

            NTT::inverse(fh);

            for (std::size_t i = 0; i < MLKEM_N; ++i)
                result.coeffs_[i] = fh[i];

            return result;
        }
    };

} // namespace mlkem