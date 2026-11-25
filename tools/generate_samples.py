"""Original 44.1 kHz / 24-bit mono source renders, four real velocity versions.
No source recordings are claimed. Layers reuse independent sources from the pool."""
import json, hashlib
from pathlib import Path
import numpy as np
ROOT=Path(__file__).resolve().parents[1]
P=ROOT/"plugins/Infinity"; OUT=P/"Content/Samples";OUT.mkdir(parents=True,exist_ok=True)
bank=json.loads((P/"factory.json").read_text());assert len(bank)==100
RATE=44100;N=RATE*2;t=np.arange(N)/RATE
hashes=set()
for idx,p in enumerate(bank):
    k=p["kind"];v=p["variant"]
    for note in (36,48,60,72,84):
        hz=440*2**((note-69)/12);ph=2*np.pi*hz*t
        for vel in range(4):
            strength=(vel+1)/4;rng=np.random.default_rng(100000+idx*10000+note*4+vel)
            def sine(h,offset=0):
                return np.sin(ph*h+offset) if h*hz<16000 else np.zeros_like(t)
            def stack(power,odd=False,detune=1):
                return sum(sine(h*detune,.071*h*v)/h**power for h in range(1,min(28,int(15000/hz)+1)) if not odd or h%2)
            def saws(power,voices=5):
                ds=np.linspace(-.011,.011,voices)*(1+v*.035)
                return sum(stack(power,False,1+d) for d in ds)/voices
            if k==0:
                y=.68*sine(1)+(.18+.45*strength)*stack(1.5-strength*.7+v*.018,v%3==0)
                y+=.12*sine(2)*np.exp(-t*(2+v*.15))
            elif k==1:
                y=.66*np.sin(ph+(.12+.8*strength+.09*v)*np.sin(ph*(2+v%3))*np.exp(-t*3))+.15*sine(2.01)*np.exp(-t*2)
                y*=np.exp(-t*(.65+v*.07))
            elif k==2:
                y=.60*saws(1.55+.3*(1-strength)+v*.025,5)+.18*sine(1.004)+.10*sine(2)
                y*=np.minimum(1,t*(3+v*.15))*(.92+.08*np.sin(t*(1+v*.2)))
            elif k==3:
                y=(.55*stack(1.7-strength*.8+v*.028)+.24*sine(2.005+v*.003))*np.exp(-t*(3.1+v*.18))
            elif k==4:
                y=.70*saws(1.35-strength*.43+v*.02,7)+.20*sine(1)+.10*sine(2.005)
            elif k==5:
                # Stiff-string model: detuned string courses, velocity-dependent hammer excitation,
                # individual partial decays, sympathetic resonance and transient.
                y=np.zeros(N)
                for h in range(1,15):
                    ratio=h*np.sqrt(1+(.00016+.000035*v)*h*h)
                    amp=(.7/h**(1.8-strength*.85))*np.exp(-t*(.75+.16*h+v*.06))
                    y+=amp*(sine(ratio)+.35*sine(ratio*1.0007)+.24*sine(ratio*.9991))
                y+=.014*strength*rng.standard_normal(N)*np.exp(-t*75)
            elif k==6:
                y=.62*saws(1.5-strength*.35+v*.08,5)+.20*sine(1.002)
                y*=np.minimum(1,t*(6+v))*(.97+.03*np.sin(2*np.pi*t*5))
            elif k==7:
                y=(.75*stack(1.8-strength*.75+v*.10)+.15*sine(1))*np.minimum(1,t*(10+v*3))
            elif k==8:
                y=(.55*stack(1.4-strength*.45+v*.1)+.2*sine(2))*np.exp(-((t*(4+v))%1)*(5+v))
            elif k==9:
                y=.5*sine(1)*np.exp(-t*(1+v*.3))+.31*sine(2.4+v*.13)*np.exp(-t*(3+v))+.18*strength*sine(4.1+v*.21)*np.exp(-t*5)
            elif k==10:
                y=.54*saws(2.1-strength*.25+v*.07,3)+.22*sine(2.003)+.11*sine(.998)
                y*=np.minimum(1,t*2)*(.85+.15*np.sin(t*(2+v*.4)))
            else:
                sweep=ph*(1+(.045+v*.03)*t)
                y=(.55*np.sin(sweep)+.19*np.sin(sweep*1.5)+.02*strength*rng.standard_normal(N))*np.exp(-t*(1+v*.3))
                if v==1:y=y[::-1].copy()
            y*=np.minimum(1,t*200)
            if not p["loop"]:y*=np.minimum(1,(N-1-np.arange(N))/2205)
            # Offline lowpass with a raised-cosine transition to limit source aliasing.
            spec=np.fft.rfft(y);freq=np.fft.rfftfreq(N,1/RATE)
            transition=np.clip((freq-13500)/3500,0,1);spec*=.5+.5*np.cos(np.pi*transition)
            y=np.fft.irfft(spec,N)
            y*=.72/max(.01,np.max(np.abs(y)))
            q=np.round(np.clip(y,-1,1)*8388607).astype(np.int32)
            data=np.stack((q&255,(q>>8)&255,(q>>16)&255),axis=1).astype(np.uint8).tobytes()
            assert len(data)==N*3 and np.any(q&255)
            digest=hashlib.sha256(data).digest();assert digest not in hashes;hashes.add(digest)
            (OUT/f"{idx}_{note}_{vel}.pcm").write_bytes(data)
assert len(hashes)==2000
print("100 patches / 2000 unique mono PCM24 zones, 44.1 kHz, four velocity layers")
