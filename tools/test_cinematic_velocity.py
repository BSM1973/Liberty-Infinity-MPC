"""Regression: soft and hard velocity PCM24 differ for every preset and root.
No loop search or bank construction required."""
import numpy as np,hashlib
from build_cinematic_horizons import GROUPS,render
seen={};i=0
for _,_,kind,names in GROUPS:
    for v,name in enumerate(names.split("|")):
        for note in (36,48,60,72,84):
            for vel in (40,100):
                x,_=render(note,vel,i,v,kind)
                assert np.isfinite(x).all()
                assert np.max(np.abs(x)) <= (.500001 if vel==40 else .920001)
                q=np.rint(x.astype(np.float64)*8388608).astype(np.int32)
                digest=hashlib.sha256(q.tobytes()).digest()
                identity=(i,name,note,vel)
                if digest in seen:raise AssertionError(f"Duplicate PCM24: {identity} vs {seen[digest]}")
                seen[digest]=identity
        i+=1
assert i==100 and len(seen)==1000
print("PASS 1000 distinct PCM24 velocity/root renders and peak ceilings")
