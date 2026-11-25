"""Verify and pack HD sources in patch, root, velocity order."""
from pathlib import Path
import struct,hashlib
P=Path(__file__).resolve().parents[1]/"plugins/Infinity/Content"
files=[P/"Samples"/f"{p}_{r}_{v}.pcm" for p in range(100) for r in (36,48,60,72,84) for v in range(4)]
temp=P/"Factory.tmp";target=P/"Factory.pcm"
header=b"LINFHD01"+struct.pack("<IIIIII",100,5,4,44100,88200,24)
hashes=[]
with temp.open("wb") as out:
    out.write(header)
    for path in files:
        data=path.read_bytes();assert len(data)==88200*3,path
        hashes.append(hashlib.sha256(data).digest());out.write(data)
with temp.open("rb") as src:
    assert src.read(len(header))==header
    for digest in hashes:assert hashlib.sha256(src.read(88200*3)).digest()==digest
    assert not src.read(1)
assert len(set(hashes))==2000
temp.replace(target)
for path in files:path.unlink()
(P/"Samples").rmdir()
print("HD pack verified:",target.stat().st_size,"bytes")
