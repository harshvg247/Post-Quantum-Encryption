#pragma once

#include <cstdint>
#include "mlkem/constants.hpp"

namespace mlkem
{
    class Zq
    {
    public:
        static inline std::uint16_t add(std::uint16_t a, std::uint16_t b) noexcept
        {
            std::uint16_t r = a + b;
            r -= (r >= MLKEM_Q) * MLKEM_Q;
            return r;
        }

        static inline std::uint16_t sub(std::uint16_t a, std::uint16_t b) noexcept
        {
            std::uint16_t r = a - b;
            r += (a < b) * MLKEM_Q;
            return r;
        }

        static inline std::uint16_t mul(std::uint16_t a, std::uint16_t b) noexcept
        {
            std::uint32_t r = static_cast<std::uint32_t>(a) * b;
            return static_cast<std::uint16_t>(r % MLKEM_Q);
        }

        static inline std::uint16_t mod_pow(std::uint16_t base, std::uint32_t pow) noexcept
        {
            std::uint32_t result = 1;
            while (pow > 0)
            {
                if (pow & 1)
                {
                    result = (result * base) % MLKEM_Q;
                }
                base = (base * base) % MLKEM_Q;
                pow >>= 1;
            }
            return static_cast<std::uint16_t>(result);
        }

        static std::uint16_t mod_inverse(std::uint16_t a) noexcept
        {
            // Fermat inverse since q is prime
            return mod_pow(a, MLKEM_Q - 2);
        }
    };

} // namespace mlkem