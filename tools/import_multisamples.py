"""Create a personal fixed-grid Infinity bank from WAV files. Requires numpy and scipy.
Usage: python tools/import_multisamples.py --base Factory.pcm --manifest bank.json --out Personal/Factory.pcm
The original bank is never modified. Install output and .map together with the host stopped.
"""
import argparse, json, math, os, shutil, struct, tempfile
from pathlib import Path
import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly, correlate

RATE, FRAMES, ROOTS, VELOCITIES = 44100, 88200, (36,48,60,72,84), (16,48,80,112)
ZONE_BYTES = FRAMES*3
HEADER = b"LINFHD01"+struct.pack("<IIIIII",100,5,4,RATE,FRAMES,24)

def integer(x, lo, hi):
    if isinstance(x,bool) or not isinstance(x,int) or not lo<=x<=hi: raise ValueError(f"Invalid integer: {x}")
    return x

def number(x,lo,hi):
    x=float(x)
    if not math.isfinite(x) or not lo<=x<=hi: raise ValueError(f"Invalid number: {x}")
    return x

def read_wav(path):
    sr,a=wavfile.read(path)
    integer(int(sr),8000,192000)
    if a.ndim not in (1,2) or (a.ndim==2 and a.shape[1]!=2) or len(a)<64: raise ValueError(f"Unsupported WAV: {path}")
    if a.dtype.kind=="u": a=(a.astype(np.float64)-128)/128
    elif a.dtype.kind=="i": a=a.astype(np.float64)/float(2**(a.dtype.itemsize*8-1))
    elif a.dtype.kind=="f": a=a.astype(np.float64)
    else: raise ValueError("Unsupported PCM encoding")
    if a.ndim==2: a=a.mean(axis=1)
    if not np.isfinite(a).all() or np.max(np.abs(a))<1e-6: raise ValueError(f"Invalid or silent WAV: {path}")
    return int(sr),a

def detect_root(a,sr):
    # Monophonic, stable-pitch material only. Reject unreliable estimates.
    a=a[:min(len(a),sr*2)]
    factor=max(1,sr//12000)
    if factor>1: a=resample_poly(a,1,factor)
    sr=sr/factor
    a=a-a.mean()
    ac=correlate(a,a,mode="full",method="fft")[len(a)-1:]
    low=max(2,int(sr/2100)); high=min(len(a)//2,int(sr/30))
    if high<=low or ac[0]<=0: raise ValueError("Root detection failed; specify root")
    peaks=[i for i in range(low,high) if ac[i]>ac[i-1] and ac[i]>=ac[i+1]]
    if not peaks: raise ValueError("Root detection failed; specify root")
    best=max(ac[i] for i in peaks)
    if best<.55*ac[0]: raise ValueError("Uncertain root; specify root")
    lag=next(i for i in peaks if ac[i]>=.90*best)
    note=round(69+12*math.log2(sr/lag/440))
    return integer(note,0,127)

def zone_data(a,sr,source_root,target_root,loop,scale):
    # Polyphase conversion plus pitch conversion via rational approximation.
    from fractions import Fraction
    ratio=Fraction(RATE/sr*2**((source_root-target_root)/12)).limit_denominator(4096)
    b=resample_poly(a,ratio.numerator,ratio.denominator)
    end=min(len(b),FRAMES)
    if end<32: raise ValueError("Resampled zone too short")
    start=xf=0
    if loop is not None:
        ls=integer(loop[0],0,len(a)-1);le=integer(loop[1],ls+1,len(a))
        start=round(ls*float(ratio));end=round(le*float(ratio))
        if not 0<=start<end<=FRAMES or end-start<96: raise ValueError("Loop outside fixed 2-second zone")
        xf=min(RATE//25,(end-start)//3)
    b=np.clip(b[:end]*scale,-1,1-1/8388608)
    pcm=np.zeros(FRAMES,dtype=np.int32);pcm[:len(b)]=np.rint(b*8388608).astype(np.int32)
    u=pcm.astype(np.uint32)
    raw=np.column_stack((u&255,(u>>8)&255,(u>>16)&255)).astype(np.uint8).tobytes()
    return raw,start,end,xf

def build(base,manifest,out):
    base,manifest,out=map(Path,(base,manifest,out))
    side=Path(str(out)+".map")
    if base.resolve()==out.resolve() or out.exists() or side.exists(): raise ValueError("Choose a new output; never overwrite a bank")
    with base.open("rb") as f:
        if f.read(32)!=HEADER or base.stat().st_size!=32+100*20*ZONE_BYTES: raise ValueError("Invalid v0.6/v0.7 factory bank")
    config=json.loads(manifest.read_text(encoding="utf-8"))
    sources=config["sources"]; patches=config["patches"]
    records=["LINFMAP1"]; report=[]; used=set()
    for patch in patches:
        p=integer(patch["patch"],0,99)
        if p in used: raise ValueError("Duplicate patch")
        used.add(p);layers=patch["layers"]
        if not 1<=len(layers)<=4: raise ValueError("One to four layers required")
        for l,item in enumerate(layers):
            q=integer(item["source"],0,99)
            g=10**(number(item.get("gain_db",0),-60,6)*.05)
            pan=number(item.get("pan",0),-1,1); cents=number(item.get("cents",0),-1200,1200)
            records.append(f"L {p} {l} {q} {g:.9g} {pan} {cents}")
    out.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(dir=out.parent) as temp:
        temp=Path(temp); bank=temp/"bank"; mapping=temp/"map"
        shutil.copyfile(base,bank)
        seen=set()
        with bank.open("r+b") as f:
            for source in sources:
                q=integer(source["source"],0,99)
                if q in seen: raise ValueError("Duplicate source")
                seen.add(q); entries=[]
                for z in source["zones"]:
                    path=manifest.parent/z["wav"]; sr,a=read_wav(path)
                    root=integer(z["root"],0,127) if "root" in z else detect_root(a,sr)
                    lo=integer(z.get("vel_min",1),1,127); hi=integer(z.get("vel_max",127),lo,127)
                    kl=integer(z.get("key_min",0),0,127); kh=integer(z.get("key_max",127),kl,127)
                    entries.append((root,lo,hi,kl,kh,a,sr,z.get("loop"),path))
                if not entries: raise ValueError("Empty source")
                peak=max(np.max(np.abs(e[5])) for e in entries)
                scale=min(4.,.891250938/peak) if source.get("normalize",False) else 1.
                for i,root in enumerate(ROOTS):
                    for j,vel in enumerate(VELOCITIES):
                        eligible=[e for e in entries if e[1]<=vel<=e[2] and e[3]<=root<=e[4]]
                        if not eligible: raise ValueError(f"Missing zone {q}/{root}/{vel}")
                        e=min(eligible,key=lambda e:abs(e[0]-root))
                        raw,start,end,xf=zone_data(e[5],e[6],e[0],root,e[7],scale)
                        f.seek(32+(q*20+i*4+j)*ZONE_BYTES);f.write(raw)
                        records.append(f"Z {q} {i} {j} {start} {end} {xf}")
                        report.append(f"{q}/{root}/{vel}: {e[8].name}, root={e[0]}, frames={end}")
        mapping.write_text("\n".join(records)+"\n",encoding="ascii")
        # Publish only complete outputs; neither existing target is overwritten.
        os.link(bank,out)
        try: os.link(mapping,side)
        except Exception: out.unlink();raise
    return report

if __name__=="__main__":
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ("base","manifest","out"):p.add_argument("--"+arg,required=True)
    args=p.parse_args()
    for line in build(args.base,args.manifest,args.out):print(line)
