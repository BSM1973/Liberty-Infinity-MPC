from pathlib import Path
import json,tempfile
import numpy as np
from scipy.io import wavfile
from expansion_pack import build
with tempfile.TemporaryDirectory() as d:
    root=Path(d);presets=[]
    for i,(name,cat,harmonic) in enumerate([("Demo Warm Keys",2,2),("Demo Soft Pad",3,3),("Demo Bright Pluck",4,4)]):
        zones=[]
        for note in (36,48,60,72,84):
            t=np.arange(44100)/44100;hz=440*2**((note-69)/12)
            env=np.minimum(1,t*50)*np.exp(-t*(3 if i==2 else 1.2))
            x=(np.sin(2*np.pi*hz*t)+.15*np.sin(2*np.pi*hz*harmonic*t))*env*.5
            wav=f"{i}-{note}.wav";wavfile.write(root/wav,44100,np.column_stack((x,x*.8)).astype(np.float32))
            zones.append(dict(wav=wav,root=note,source="Liberty original synthetic test render",rights="Original test fixture"))
        presets.append(dict(index=i,name=name,category=cat,layers=[dict(zones=zones)]))
    manifest=dict(id="9f95dbe4-15d7-4c4b-8b8d-0ea319c96e88",name="Liberty Demo Collection",entitlement="DEMO",presets=presets)
    (root/"manifest.json").write_text(json.dumps(manifest))
    build(root/"manifest.json",Path("expansion-demo"))
