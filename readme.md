Compile: 
g++ examples/main.cpp -Iinclude -lcryptopp -o mlkem.exe

Medium:[text](https://medium.com/@kcl17/the-new-handshake-understanding-ml-kem-fips-203-6a403bed68ee)
SHAKE: [text](https://cryptopp.com/docs/ref890/class_s_h_a_k_e.html#ab06f5158bc297ed622787704f41f84b2)

Why use noexcept?
    Exceptions are avoided because they:
        break constant-time guarantees
        introduce timing side channels
        make control flow harder to audit
        reduce performance predictability

On running examples/naive_vs_ntt.cpp following results were obtained:
// Running benchmark (100 iterations)...

Results:
Naive: 0.130817 sec
NTT  : 0.0088718 sec

Speedup: 14.7452x

On running precompute_zeta.cpp file following results were obtained
Running full NTT multiplication benchmark...

Results:
With tables : 0.0402558 sec
Without     : 0.0531028 sec

Speedup: 1.31913x

Known Answer Tests (KAT): Download the official NIST ML-KEM KAT files and write a wrapper to verify that your keygen, encaps, and decaps functions produce the exact same outputs given the same seeds.

Unit Tests for Primitives: Add specific tests for the Number Theoretic Transform (NTT) and the Centered Binomial Distribution (CBD) sampling to prove each component functions as specified in FIPS 203.
Parameter Agnosticism: Currently, your code is hardcoded for ML-KEM-512 ($k=2$). Refactor the Matrix and KPKE classes to use template parameters for $k$, $\eta_1$, and $\eta_2$. This would allow your library to support ML-KEM-768 and ML-KEM-1024 easily.