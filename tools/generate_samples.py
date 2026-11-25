"""Deterministic original synthetic source library rendered OFFLINE as stereo PCM.
These are development sounds, not recordings of acoustic instruments."""
import json, hashlib
from pathlib import Path
import numpy as np
ROOT=Path(__file__).resolve().parents[1]
P=ROOT/"plugins/Infinity"
OUT=P/"Content/Samples"; OUT.mkdir(parents=True,exist_ok=True)
bank=json.loads((P/"factory.json").read_text())
assert len(bank)==100 and len({p["name"] for p in bank})==100
RATE=24000; N=RATE*2; t=np.arange(N,dtype=np.float64)/RATE
hashes=set()
for idx,p in enumerate(bank):
    k=p["kind"];v=p["variant"]
    for note in (36,48,60,72,84):
        hz=440*2**((note-69)/12);ph=2*np.pi*hz*t
        rng=np.random.default_rng(100000+idx*128+note)
        channels=[]
        for ch in (-1,1):
            det=1+ch*(.0008+.00018*v)
            def wave(h,phase=0): return np.sin(ph*h+phase) if h*hz<10500 else np.zeros_like(t)
            def stack(power,odd=False):
                return sum(wave(h,ch*.013*h*v)/(h**power) for h in range(1,min(16,int(10500/hz)+1)) if not odd or h%2)
            if k==0:
                y=.8*wave(1)+(.12+.025*(v%5))*stack(.7+(v%4)*.35,v%3==0)
                y*=.92+.08*np.exp(-t*(2+v*.3))
            elif k==1:
                y=.68*np.sin(ph+(.2+.14*v)*np.sin(ph*(2+v%3))*np.exp(-t*2))+ .2*wave(2.01)*np.exp(-t*(1+v*.2))
                y*=np.exp(-t*(.4+.13*v))
            elif k==2:
                y=.35*stack(1.3+v*.065)+.30*np.sin(ph*det)+.16*np.sin(ph*(1+ch*.004*(1+v%4)))+.10*wave(2)
                y*=np.minimum(1,t*(3+v*.25))*(.9+.1*np.sin(t*(1+v*.3)+ch*.3))
            elif k==3:
                y=(.65*stack(.9+(v%5)*.22)+.18*wave(2.005+v*.002))*np.exp(-t*(3+v*.32))
            elif k==4:
                y=.55*stack(.8+(v%5)*.2,v%3==0)+.25*np.sin(ph*det)+.12*np.sin(ph*(1+ch*.006))
                y*=.95+.05*np.sin(t*(3+v)*2*np.pi)
            elif k==5:
                y=sum((.65/h**.9)*wave(h*(1+.0008*h*h))*np.exp(-t*(.65+.15*h+v*.075)) for h in range(1,9))
                y+=.04*rng.standard_normal(N)*np.exp(-t*65)
            elif k==6:
                y=.45*stack(1.45+v*.12)+.24*np.sin(ph*det+.25*np.sin(t*2*np.pi*(4+v*.3)))+.13*wave(2)
                y*=np.minimum(1,t*(5+v))
            elif k==7:
                y=(.7*stack(.9+v*.17)+.18*wave(1))*np.minimum(1,t*(8+v*3))
            elif k==8:
                pulse=np.exp(-((t*(4+v))%1)*(5+v))
                y=(.55*stack(1+v*.2)+.22*wave(2))*pulse
            elif k==9:
                y=.48*wave(1)*np.exp(-t*(1+v*.3))+.30*wave(2.4+v*.13)*np.exp(-t*(3+v))+.19*wave(4.1+v*.21)*np.exp(-t*6)
            elif k==10:
                y=.38*np.sin(ph*det+.6*np.sin(t*(1+v*.3)))+.25*wave(2)+.14*np.sin(ph*(1+ch*.009))
                y*=np.minimum(1,t*2)*(.8+.2*np.sin(t*(2+v*.4)+ch))
            else:
                sweep=ph*(1+(.045+v*.03)*t)
                y=(.55*np.sin(sweep)+.22*np.sin(sweep*1.5)+.04*rng.standard_normal(N))*np.exp(-t*(1+v*.3))
                if v==1:y=y[::-1].copy()
            y*=np.minimum(1,t*200)
            if not p["loop"]:y*=np.minimum(1,(N-1-np.arange(N))/1200)
            channels.append(y)
        pcm=np.stack(channels,axis=1);pcm*=.8/max(.01,np.max(np.abs(pcm)))
        data=np.round(pcm*32767).astype("<i2").tobytes()
        assert len(data)==N*4
        digest=hashlib.sha256(data).digest();assert digest not in hashes;hashes.add(digest)
        (OUT/f"{idx}_{note}.pcm").write_bytes(data)
assert len(hashes)==500
print("100 original development patches, 500 unique stereo zones, 96 MB PCM16")
