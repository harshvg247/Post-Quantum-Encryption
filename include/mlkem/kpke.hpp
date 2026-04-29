#pragma once

#include <array>
#include <vector>

#include "mlkem/constants.hpp"
#include "mlkem/matrix.hpp"
#include "mlkem/vector.hpp"
#include "mlkem/sampling.hpp"
#include "mlkem/shake.hpp"
#include "mlkem/ntt.hpp"

namespace mlkem
{

    class KPKE
    {
    public:
        static constexpr std::size_t K = Matrix::K;
        static constexpr int ETA = 2;

        using Vec = Vector::Vec;

        struct PublicKey
        {
            Vec t;                        // t̂ (NTT domain)
            std::array<uint8_t, 32> seed; // seed for A
        };

        struct SecretKey
        {
            Vec s; // ŝ (NTT domain)
        };

        struct Ciphertext
        {
            Vec u;                           // NORMAL domain
            std::array<uint16_t, MLKEM_N> v; // NORMAL domain
        };


        // FIX THIS: Here randomness of A, s and e are correlated
        // ============================================================
        // KeyGen
        // ============================================================
        static void keygen(const std::array<uint8_t, 32> &seed,
                           PublicKey &pk,
                           SecretKey &sk)
        {
            auto A = Matrix::generate(seed);

            // --- generate randomness ---
            SHAKE128 shake;
            shake.absorb(seed.data(), seed.size());

            std::vector<uint8_t> buf(64 * ETA * 2 * K);
            shake.squeeze(buf.data(), buf.size());

            Vec s{}, e{};

            for (std::size_t i = 0; i < K; ++i)
            {
                std::vector<uint8_t> slice_s(
                    buf.begin() + i * 64 * ETA,
                    buf.begin() + (i + 1) * 64 * ETA);

                std::vector<uint8_t> slice_e(
                    buf.begin() + (i + K) * 64 * ETA,
                    buf.begin() + (i + K + 1) * 64 * ETA);

                auto si = Sampling::sample_poly_cbd<ETA>(slice_s);
                auto ei = Sampling::sample_poly_cbd<ETA>(slice_e);

                NTT::forward(si);
                NTT::forward(ei);

                s[i] = si;
                e[i] = ei;
            }

            auto t = Vector::mat_vec_mul(A, s);
            t = Vector::add(t, e);

            pk.t = t;
            pk.seed = seed;
            sk.s = s;
        }

        // ============================================================
        // Encrypt
        // ============================================================
        static Ciphertext encrypt(const PublicKey &pk,
                                  const std::array<uint8_t, 32> &coins,
                                  const std::array<uint8_t, 32> &message)
        {
            Ciphertext ct{};

            auto A = Matrix::generate(pk.seed);
            auto AT = Matrix::transpose(A);

            // --- randomness ---
            SHAKE128 shake;
            shake.absorb(coins.data(), coins.size());

            std::vector<uint8_t> buf(64 * ETA * (2 * K + 1));
            shake.squeeze(buf.data(), buf.size());

            Vec r{}, e1{};
            std::array<uint16_t, MLKEM_N> e2{};

            // ---- r and e1 ----
            for (std::size_t i = 0; i < K; ++i)
            {
                std::vector<uint8_t> slice_r(
                    buf.begin() + i * 64 * ETA,
                    buf.begin() + (i + 1) * 64 * ETA);

                std::vector<uint8_t> slice_e1(
                    buf.begin() + (i + K) * 64 * ETA,
                    buf.begin() + (i + K + 1) * 64 * ETA);

                auto ri = Sampling::sample_poly_cbd<ETA>(slice_r);
                auto e1i = Sampling::sample_poly_cbd<ETA>(slice_e1);

                // r in NTT
                NTT::forward(ri);

                // e1 stays NORMAL
                r[i] = ri;
                e1[i] = e1i;
            }

            // ---- e2 ----
            std::vector<uint8_t> slice_e2(
                buf.begin() + 2 * K * 64 * ETA,
                buf.end());

            e2 = Sampling::sample_poly_cbd<ETA>(slice_e2);

            // ========================================================
            // u = A^T * r  (NTT)
            // ========================================================
            auto u_ntt = Vector::mat_vec_mul(AT, r);

            // convert to NORMAL
            for (auto &poly : u_ntt)
                NTT::inverse(poly);

            // add e1 (NORMAL)
            ct.u = Vector::add(u_ntt, e1);

            // ========================================================
            // v = t·r
            // ========================================================
            auto v_ntt = Vector::dot(pk.t, r);

            NTT::inverse(v_ntt);

            // add e2
            for (std::size_t i = 0; i < MLKEM_N; ++i)
                v_ntt[i] = Zq::add(v_ntt[i], e2[i]);

            // encode message
            for (std::size_t i = 0; i < MLKEM_N; ++i)
            {
                uint8_t bit = (message[i / 8] >> (i % 8)) & 1;
                if (bit)
                    v_ntt[i] = Zq::add(v_ntt[i], MLKEM_Q / 2);
            }

            ct.v = v_ntt;

            return ct;
        }

        // ============================================================
        // Decrypt
        // ============================================================
        static std::array<uint8_t, 32>
        decrypt(const SecretKey &sk,
                const Ciphertext &ct)
        {
            std::array<uint8_t, 32> message{};
            message.fill(0);

            // convert u → NTT
            Vec u_ntt = ct.u;
            for (auto &poly : u_ntt)
                NTT::forward(poly);

            auto x_ntt = Vector::dot(u_ntt, sk.s);
            NTT::inverse(x_ntt);

            std::array<uint16_t, MLKEM_N> m_poly{};

            for (std::size_t i = 0; i < MLKEM_N; ++i)
                m_poly[i] = Zq::sub(ct.v[i], x_ntt[i]);

            // robust decode
            uint16_t half = MLKEM_Q / 2;

            for (std::size_t i = 0; i < MLKEM_N; ++i)
            {
                uint16_t val = m_poly[i];

                // map to closest multiple of q/2
                // equivalent to rounding

                uint32_t t = (static_cast<uint32_t>(val) * 2 + MLKEM_Q / 2) / MLKEM_Q;

                // t is now 0 or 1
                uint8_t bit = static_cast<uint8_t>(t & 1);

                message[i / 8] |= (bit << (i % 8));
            }

            return message;
        }
    };

} // namespace mlkem