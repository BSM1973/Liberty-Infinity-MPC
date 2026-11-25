"""Original offline test samples. No synthesis in the MPC audio callback."""
import math, struct
from pathlib import Path
RATE=24000
N=RATE*3
OUT=Path(__file__).resolve().parents[1]/"plugins/Infinity/Content/Samples"
OUT.mkdir(parents=True,exist_ok=True)
for patch in range(4):
    for note in (36,48,60,72,84):
        hz=440*2**((note-69)/12)
        data=bytearray()
        for i in range(N):
            t=i/RATE
            phase=2*math.pi*hz*t
            fade=min(1,i/120,(N-1-i)/1200)
            for channel in (-1,1):
                if patch==0:
                    y=.65*math.sin(phase)+.18*math.sin(2*phase)+.08*math.sin(3*phase)
                elif patch==1:
                    y=.52*math.sin(phase)*math.exp(-t*.9)+.22*math.sin(phase*2.01+channel*.1)*math.exp(-t*2)+.12*math.sin(phase*3.02)*math.exp(-t*3)
                elif patch==2:
                    y=.27*math.sin(phase)+.23*math.sin(phase*(1+channel*.003))+.12*math.sin(phase*2)+.08*math.sin(phase*3)
                    y*=min(1,t*5)
                else:
                    y=(.45*math.sin(phase)+.22*math.sin(phase*2)+.15*math.sin(phase*3))*math.exp(-t*4)
                value=round(max(-1,min(1,y*fade))*30000)
                data.extend(struct.pack("<h",value))
        assert len(data)==N*4
        (OUT/f"{patch}_{note}.pcm").write_bytes(data)
print("Generated 20 original stereo PCM test files (4 patches x 5 root notes).")
