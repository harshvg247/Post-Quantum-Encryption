#pragma once

#include <vector>
#include <cstdint>

#include "mlkem/kpke.hpp"

namespace mlkem {

class Serialize {
public:
    static std::vector<uint8_t>
    pk_to_bytes(const KPKE::PublicKey& pk)
    {
        std::vector<uint8_t> out;

        // serialize t̂
        for (std::size_t i = 0; i < KPKE::K; ++i)
        {
            for (std::size_t j = 0; j < MLKEM_N; ++j)
            {
                uint16_t val = pk.t[i][j];

                // little endian
                out.push_back(static_cast<uint8_t>(val & 0xFF));
                out.push_back(static_cast<uint8_t>(val >> 8));
            }
        }

        // append seed
        for (auto b : pk.seed)
            out.push_back(b);

        return out;
    }
};

} // namespace mlkem