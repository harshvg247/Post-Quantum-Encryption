#include <iostream>
#include "mlkem/kem.hpp"

int main()
{
    mlkem::KEM::PublicKey pk;
    mlkem::KEM::SecretKey sk;

    mlkem::KEM::keygen(pk, sk);

    mlkem::KEM::Ciphertext ct;
    mlkem::KEM::SharedSecret ss1, ss2;

    mlkem::KEM::encaps(pk, ct, ss1);
    mlkem::KEM::decaps(sk, ct, ss2);

    bool ok = true;

    for (int i = 0; i < 32; ++i)
    {
        if (ss1.key[i] != ss2.key[i])
        {
            ok = false;
            std::cout << "Mismatch at byte " << i << "\n";
            break;
        }
    }

    std::cout << "[ML-KEM] "
              << (ok ? "PASSED\n" : "FAILED\n");

    return 0;
}