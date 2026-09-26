# QSB baseline CPU primitive validation

Bounded, free standard GitHub Actions validation of unchanged public baseline `acdf549db2d32cfa7f58eac50e6dfa73140fee30`. This is not a competition candidate or a target RTX 4090 benchmark. No GPU, cache or artifact upload is used. Only a manually dispatched, ten-minute-limited standard Ubuntu job is defined.

`prepare.py` verifies the complete upstream `CpuGrindSubset.h` SHA256 before extracting the unchanged prefix containing scalar/IFMA field and SHA-NI primitives. All upstream authorship and GPL-3 notices are preserved; upstream license: https://github.com/Layr-Labs/quantum-safe-bitcoin-challenge/blob/acdf549db2d32cfa7f58eac50e6dfa73140fee30/candidates/subset/COPYING . The independent test driver compares scalar operations with OpenSSL, IFMA with those scalar operations when available, and SHA-NI with OpenSSL. Missing ISA is explicitly skipped. It does not validate the full CUDA build, candidate enumeration, hit gate, or competition score.

Reproduce: `python3 prepare.py` then `g++ -std=c++17 -O3 -Wno-deprecated-declarations primitive_test.cpp -o primitive_test -lcrypto -pthread && ./primitive_test` on Linux x86_64.
