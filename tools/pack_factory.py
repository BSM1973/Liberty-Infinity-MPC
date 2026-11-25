"""Pack the exact original PCM bytes in engine order, with a versioned header.
One sequential file read instead of 500 separate file opens. No audio-thread I/O."""
from pathlib import Path
import struct, hashlib
P=Path(__file__).resolve().parents[1]/"plugins/Infinity/Content"
files=[P/"Samples"/f"{p}_{r}.pcm" for p in range(100) for r in (36,48,60,72,84)]
temp=P/"Factory.tmp";target=P/"Factory.pcm"
header=b"LINFBK01"+struct.pack("<IIII",100,5,24000,48000)
hashes=[]
with temp.open("wb") as out:
    out.write(header)
    for path in files:
        data=path.read_bytes()
        assert len(data)==48000*4, path
        hashes.append(hashlib.sha256(data).digest())
        out.write(data)
with temp.open("rb") as src:
    assert src.read(len(header))==header
    for digest in hashes:
        assert hashlib.sha256(src.read(48000*4)).digest()==digest
    assert not src.read(1)
temp.replace(target)
for path in files:path.unlink()
(P/"Samples").rmdir()
print("Packed and verified 500 identical PCM zones in one Factory.pcm file:",target.stat().st_size,"bytes")
