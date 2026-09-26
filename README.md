# QSB baseline CPU primitive validation

Bounded, free standard GitHub Actions validation of unchanged public baseline `acdf549db2d32cfa7f58eac50e6dfa73140fee30`. This is not a competition candidate or a target RTX 4090 benchmark. No GPU, cache or artifact upload is used. Only a manually dispatched, ten-minute-limited standard Ubuntu job is defined.

`prepare.py` verifies the complete upstream `CpuGrindSubset.h` SHA256 before extracting the unchanged prefix containing scalar/IFMA field and SHA-NI primitives. All upstream authorship and GPL-3 notices are preserved; upstream license: https://github.com/Layr-Labs/quantum-safe-bitcoin-challenge/blob/acdf549db2d32cfa7f58eac50e6dfa73140fee30/candidates/subset/COPYING . The independent test driver compares scalar operations with OpenSSL, IFMA with those scalar operations when available, and SHA-NI with OpenSSL. Missing ISA is explicitly skipped. It does not validate the full CUDA build, candidate enumeration, hit gate, or competition score.

Reproduce: `python3 prepare.py` then `g++ -std=c++17 -O3 -Wno-deprecated-declarations primitive_test.cpp -o primitive_test -lcrypto -pthread && ./primitive_test` on Linux x86_64.

## Round 3 product-tree experiment

A standard balanced product tree replaces the four serial prefix chains in ordinary CPU IFMA windows, leaving arithmetic, the final folded window, candidate enumeration, exact OpenSSL gate and GPU code unchanged. The pinned upstream baseline is `196861248169aed40f0b0ec051f74c29174d3d5f`. This is an experimental GPL-3 source patch, independently implemented scheduling of a standard batch-inversion identity; upstream authorship and license notices apply. See COPYING and the upstream header.

Frozen test commit `2080170c458aeab69ae52897f6acfb67e39e7696`, successful run [36228738477 attempt 4](https://github.com/hangglider5/qsb-cpu-validation-20260926/actions/runs/36228738477/attempts/4): Intel Xeon Platinum 8370C, one worker. Canonical window tests passed, including zero denominators and non-power-of-two fallback. Both complete workers processed the first 262144 candidates at N=12, producing identical sets of 124 hits; all were independently re-derived by the unchanged upstream Python verifier.

Fixed ABBA worker screening: baseline 1.69475 M/s, tree 1.59350 M/s, ratio 0.94026. Both blocks regressed by about 5.97%. Decision: stop this prototype; no competition submission or official GPU evaluation. Free-runner CPU-only results do not establish ranked-host or RTX 4090 performance. Failed preparation/compiler paths and three unsupported-ISA allocations preceded the first actual measurement; no measured result was redrawn.

Reproduction: `python3 prepare_round3.py`, compile/run `window_test.cpp` with the same flags as `primitive_test.cpp`, then `python3 run_round3.py`. These dev hooks are CPU-only tests and are not included in a ranked build. Temporary hit files were verified in the job but were not exported; the pinned harness/public fixture allow deterministic regeneration.
