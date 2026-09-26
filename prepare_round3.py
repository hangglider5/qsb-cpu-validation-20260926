import hashlib
from pathlib import Path
import subprocess
import urllib.request

commit='196861248169aed40f0b0ec051f74c29174d3d5f'
url=f'https://raw.githubusercontent.com/Layr-Labs/quantum-safe-bitcoin-challenge/{commit}/'
root=Path('generated')
def fetch(path):
    data=urllib.request.urlopen(url+path,timeout=30).read()
    dst=root/path;dst.parent.mkdir(parents=True,exist_ok=True);dst.write_bytes(data)
    print('SOURCE',path,hashlib.sha256(data).hexdigest())
    return data.decode()
cpu=fetch('candidates/subset/CpuGrindSubset.h')
assert hashlib.sha256(cpu.encode()).hexdigest()=='d4270ea6a7c25b566ff29d864498f412bb5b092adfe9d5f40c1819ffa747f890'
tree=fetch('candidates/subset/tests/gpu_epochs/tree.cu')
fetch('candidates/subset/tests/gpu_epochs/qsb_host_verify.h')
fetch('candidates/subset/COPYING')
fetch('candidates/subset/COPYING-secp256k1')
for f in ['crypto.py','problem.py','verify.py','gpu_wrap.py','config.py']:
    fetch('harness/'+f)
types=tree.split('/* Digest params loader */',1)[1].split('typedef struct {\n    uint64_t alpha[4]',1)[0]
helpers=tree.split('static uint64_t binom_u64(',1)[1].split('#if QSB_HOST_VERIFY',1)[0]
Path('generated/host_support.h').write_text('#include <stdint.h>\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#define SIG_PUSH_SIZE 10\n#define QSB_SE_PER_EPOCH 128\n#define QSB_SE_TWIN 3\n'+types+'\nstatic uint64_t binom_u64('+helpers+'\n#include "candidates/subset/tests/gpu_epochs/qsb_host_verify.h"\n')
Path('generated/baseline_cpu.h').write_text(cpu)
subprocess.run(['git','apply','--directory=generated','candidate.patch'],check=True)
variant=(root/'candidates/subset/CpuGrindSubset.h').read_text()
Path('generated/primitive_round3.h').write_text(variant.split('struct Ctx {',1)[0]+'\n} // namespace qcpu\n')
print('VARIANT',hashlib.sha256(variant.encode()).hexdigest())
