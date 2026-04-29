#pragma once

#include <cryptopp/shake.h>
#include <cstdint>

namespace mlkem
{

    // SHAKE-128 is an extendable-output function (XOF) from SHA-3.
    // Key properties:
    // Input: arbitrary-length bytes
    // Output: arbitrary-length bytes (not fixed like SHA-256)
    class SHAKE128
    {
    private:
        CryptoPP::SHAKE128 shake;

    public:
        void absorb(const std::uint8_t *data, std::size_t len)
        {
            // Update() → absorb input
            shake.Update(data, len);
        }

        void squeeze(std::uint8_t *out, std::size_t len)
        {
            // TruncatedFinal() → finalize + squeeze output
            shake.TruncatedFinal(out, len);
        }
    };

} // namespace mlkem