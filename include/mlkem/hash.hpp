#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <cryptopp/sha3.h>
#include <cryptopp/filters.h>

namespace mlkem {

class Hash {
public:
    // ============================================================
    // SHA3-256
    // ============================================================
    static std::array<uint8_t, 32>
    sha3_256(const std::vector<uint8_t>& input)
    {
        std::array<uint8_t, 32> out{};

        CryptoPP::SHA3_256 hash;
        hash.Update(input.data(), input.size());
        hash.Final(out.data());

        return out;
    }

    // ============================================================
    // SHA3-512
    // ============================================================
    static std::array<uint8_t, 64>
    sha3_512(const std::vector<uint8_t>& input)
    {
        std::array<uint8_t, 64> out{};

        CryptoPP::SHA3_512 hash;
        hash.Update(input.data(), input.size());
        hash.Final(out.data());

        return out;
    }

    // ============================================================
    // H(pk) = SHA3-256(pk_bytes)
    // ============================================================
    static std::array<uint8_t, 32>
    H(const std::vector<uint8_t>& pk_bytes)
    {
        return sha3_256(pk_bytes);
    }

    // ============================================================
    // G(input) = SHA3-512(input)
    // split into (key, coins)
    // ============================================================
    static void G(const std::vector<uint8_t>& input,
                  std::array<uint8_t, 32>& key,
                  std::array<uint8_t, 32>& coins)
    {
        auto hash = sha3_512(input);

        for (int i = 0; i < 32; ++i)
        {
            key[i] = hash[i];
            coins[i] = hash[32 + i];
        }
    }
};

} // namespace mlkem