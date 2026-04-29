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