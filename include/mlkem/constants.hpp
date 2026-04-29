#pragma once

#include <cstdint>

namespace mlkem {

// C++17 rule:
// constexpr variables in headers must be inline
// Prevents multiple definition errors
// Ensures single ODR instance

// A constexpr value can be used
// as a template argument or a fixed-array size 
// because the compiler is 100% sure of the value 
// before the program starts. 
// A const variable is only a compile-time constant 
// if its initializer is also a constant.

inline constexpr std::uint16_t MLKEM_N = 256;
inline constexpr std::uint16_t MLKEM_Q = 3329;
inline constexpr std::uint16_t MLKEM_ZETA = 17;

}