"""Extract unchanged baseline primitives, rejecting drift before compilation."""
import hashlib
from pathlib import Path
import urllib.request

commit = "acdf549db2d32cfa7f58eac50e6dfa73140fee30"
url = f"https://raw.githubusercontent.com/Layr-Labs/quantum-safe-bitcoin-challenge/{commit}/candidates/subset/CpuGrindSubset.h"
data = urllib.request.urlopen(url, timeout=30).read()
expected = "6727ff78fc11426f2f674a7d16f416a8a84e2bd193956b186c3cbba9a3a031ab"
assert hashlib.sha256(data).hexdigest() == expected, "baseline source drift"
source = data.decode()
assert source.count("struct Ctx {") == 1
Path("baseline_primitives.h").write_text(source.split("struct Ctx {", 1)[0] + "\n} // namespace qcpu\n")
print(f"BASELINE {commit} sha256={expected}; unchanged prefix before Ctx")
