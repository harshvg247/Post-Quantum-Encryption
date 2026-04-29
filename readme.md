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