#pragma once

#include <array>
#include <random>

#include "mlkem/kpke.hpp"
#include "mlkem/hash.hpp"
#include "mlkem/serialize.hpp"

namespace mlkem
{

    // CPA security protects against attackers who can obtain encryptions
    // of chosen messages, while
    // CCA security additionally protects against attackers
    // who can request decryptions of chosen ciphertexts.
    // CCA is strictly stronger and necessary in practical systems,
    // as it defends against adaptive attacks where adversaries manipulate ciphertexts.
    // ML-KEM achieves CCA security using the Fujisaki–Okamoto transform,
    // which verifies ciphertext integrity via re-encryption
    // and uses fallback randomness to prevent information leakage.

    class KEM
    {
    public:
        struct PublicKey
        {
            KPKE::PublicKey pk;
        };

        struct SecretKey
        {
            KPKE::SecretKey sk_pke;
            KPKE::PublicKey pk;
            std::array<uint8_t, 32> hpk;
            std::array<uint8_t, 32> z;
        };

        struct Ciphertext
        {
            KPKE::Ciphertext ct;
        };

        struct SharedSecret
        {
            std::array<uint8_t, 32> key;
        };

        // ============================================================
        // Encapsulation
        // ============================================================
        static void encaps(const PublicKey &pk,
                           Ciphertext &ct_out,
                           SharedSecret &ss_out)
        {
            // --------------------------------------------------------
            // 1. Generate random message m
            // --------------------------------------------------------
            std::array<uint8_t, 32> m{};

            std::random_device rd;
            for (auto &b : m)
                b = rd() & 0xFF;

            // --------------------------------------------------------
            // 2. Compute H(pk)
            // --------------------------------------------------------
            auto pk_bytes = Serialize::pk_to_bytes(pk.pk);
            auto hpk = Hash::H(pk_bytes);

            // --------------------------------------------------------
            // 3. Compute K̄ = H(m || H(pk))
            // --------------------------------------------------------
            std::vector<uint8_t> input;

            for (auto b : m)
                input.push_back(b);

            for (auto b : hpk)
                input.push_back(b);

            auto kbar = Hash::sha3_256(input);

            // --------------------------------------------------------
            // 4. Derive (K, coins)
            // --------------------------------------------------------
            std::array<uint8_t, 32> K{};
            std::array<uint8_t, 32> coins{};

            Hash::G(std::vector<uint8_t>(kbar.begin(), kbar.end()),
                    K,
                    coins);

            // --------------------------------------------------------
            // 5. Encrypt
            // --------------------------------------------------------
            ct_out.ct = KPKE::encrypt(pk.pk, coins, m);

            // --------------------------------------------------------
            // 6. Output shared secret
            // --------------------------------------------------------
            ss_out.key = K;
        }

        // ============================================================
        // ML-KEM KeyGen
        // ============================================================
        static void keygen(PublicKey &pk,
                           SecretKey &sk)
        {
            // --------------------------------------------------------
            // 1. Generate base seed
            // --------------------------------------------------------
            std::array<uint8_t, 32> seed{};

            std::random_device rd;
            for (auto &b : seed)
                b = rd() & 0xFF;

            // --------------------------------------------------------
            // 2. Run PKE KeyGen
            // --------------------------------------------------------
            KPKE::PublicKey pk_pke;
            KPKE::SecretKey sk_pke;

            KPKE::keygen(seed, pk_pke, sk_pke);

            // --------------------------------------------------------
            // 3. Serialize pk
            // --------------------------------------------------------
            auto pk_bytes = Serialize::pk_to_bytes(pk_pke);

            // --------------------------------------------------------
            // 4. Compute H(pk)
            // --------------------------------------------------------
            auto hpk = Hash::H(pk_bytes);

            // --------------------------------------------------------
            // 5. Generate random z
            // --------------------------------------------------------
            std::array<uint8_t, 32> z{};
            for (auto &b : z)
                b = rd() & 0xFF;

            // --------------------------------------------------------
            // 6. Store
            // --------------------------------------------------------
            pk.pk = pk_pke;

            sk.sk_pke = sk_pke;
            sk.pk = pk_pke;
            sk.hpk = hpk;
            sk.z = z;
        }

        static bool ciphertext_equal(const KPKE::Ciphertext &a,
                                     const KPKE::Ciphertext &b)
        {
            uint16_t diff = 0;

            // Compare vector u
            for (std::size_t i = 0; i < KPKE::K; ++i)
            {
                for (std::size_t j = 0; j < MLKEM_N; ++j)
                {
                    // Bitwise OR accumulates any non-zero difference
                    diff |= (a.u[i][j] ^ b.u[i][j]);
                }
            }

            // Compare polynomial v
            for (std::size_t i = 0; i < MLKEM_N; ++i)
            {
                diff |= (a.v[i] ^ b.v[i]);
            }

            // If diff is 0, they are equal.
            // We return a boolean while avoiding an explicit 'if' if possible,
            // though the final conversion to bool is generally safe at the end.
            return (diff == 0);
        }

        static void decaps(const SecretKey &sk,
                           const Ciphertext &ct,
                           SharedSecret &ss_out)
        {
            // --------------------------------------------------------
            // 1. Decrypt
            // --------------------------------------------------------
            auto m_prime = KPKE::decrypt(sk.sk_pke, ct.ct);

            // --------------------------------------------------------
            // 2. Compute K̄' = H(m' || H(pk))
            // --------------------------------------------------------
            std::vector<uint8_t> input;

            for (auto b : m_prime)
                input.push_back(b);

            for (auto b : sk.hpk)
                input.push_back(b);

            auto kbar_prime = Hash::sha3_256(input);

            // --------------------------------------------------------
            // 3. Derive (K', coins')
            // --------------------------------------------------------
            std::array<uint8_t, 32> K_prime{};
            std::array<uint8_t, 32> coins_prime{};

            Hash::G(std::vector<uint8_t>(kbar_prime.begin(), kbar_prime.end()),
                    K_prime,
                    coins_prime);

            // --------------------------------------------------------
            // 4. Re-encrypt
            // --------------------------------------------------------
            auto ct_prime = KPKE::encrypt(sk.pk, coins_prime, m_prime);

            // --------------------------------------------------------
            // 5. Compare
            // --------------------------------------------------------
            if (ciphertext_equal(ct.ct, ct_prime))
            {
                ss_out.key = K_prime;
            }
            else
            {
                // fallback: derive from z
                std::vector<uint8_t> fallback_input;

                for (auto b : sk.z)
                    fallback_input.push_back(b);

                for (std::size_t i = 0; i < MLKEM_N; ++i)
                    fallback_input.push_back(static_cast<uint8_t>(ct.ct.v[i] & 0xFF));

                auto fallback = Hash::sha3_256(fallback_input);

                ss_out.key = fallback;
            }
        }
    };

} // namespace mlkem