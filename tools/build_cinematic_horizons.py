"""Original PCM-only expansion candidate. No acoustic recordings claimed."""
from pathlib import Path
import tempfile,json,hashlib
import numpy as np
from scipy.io import wavfile
from PIL import Image,ImageDraw,ImageFont
from expansion_pack import build
RATE=44100;N=RATE*2
GROUPS=[
(1,"BA","bass","Obsidian Pulse|Gravity Engine|Sub Eclipse|Copper Drive|Night Reactor|Silicon Funk|Dark Matter|Velvet Core|Neon Torque|Magnetic Low|Quantum Reese|Midnight Voltage|Chrome Acid|Black Current|Deep Vector|Solar Pressure"),
(2,"KY","keys","Glass Circuit|Golden Tines|Lunar Electric|Amber Keys|Soft Voltage|Crystal Organ|Silver Chime|Dusty Memory|Velvet Phase|Neon Electric|Silicon Bells|Orbit Keys"),
(3,"PD","pad","Cinematic Dawn|Astral Veil|Frozen Orbit|Silent Aurora|Golden Mist|Celestial Bloom|Infinite Skies|Deep Horizon|Amber Nebula|Polar Drift|Velvet Cosmos|Luminous Dusk|Stellar Rain|Prism Waves|Obsidian Clouds|Dream Fragments|Moonlit Ocean|Silver Atmos|Solar Wings|Afterglow"),
(4,"PL","pluck","Prism Drop|Trance Spark|Neon Harp|Crystal Motion|Copper Rain|Digital Dew|Velvet Pick|Solar Pulse|Glass Echo|Lunar Strings|Amber Pluck|Midnight Drops|Electric Pearl|Satellite Pluck|Frosted Chord|Golden Plectrum"),
(5,"LD","lead","Aurora Lead|Festival Horizon|Neon Comet|Solar Anthem|Prism Flight|Chrome Solo|Night Rider|Electric Sky|Supernova|Laser Silk|Amber Rocket|Silicon Storm|Golden Saw|Magnetic Lead|Crystal Racer|Obsidian Beam"),
(10,"BL","bell","Frozen Bell|Lunar Mallet|Golden Clock|Astral Chime|Glass Meteor|Silver Resonance|Amber Orbit|Celestial Vibes"),
(11,"AT","atmos","Dark Aurora|Nebula Breath|Midnight Fog|Distant Signal|Solar Wind|Beyond Earth|Crystal Void|Eternal Night"),
(12,"FX","fx","Gravity Sweep|Digital Comet|Reverse Echo|Obsidian Impact")]
def render(note,vel,index,v,kind):
    t=np.arange(N)/RATE;hz=440*2**((note-69)/12);ph=2*np.pi*hz*t;strength=vel/127
    rng=np.random.default_rng(983734+index*1000+note*4+vel)
    def part(h,offset=0):
        return np.sin(ph*h+offset) if h*hz<15000 else np.zeros(N)
    def harmonics(power,detune=1,offset=0):
        # Distinct spectral families, not only small variations in harmonic tilt.
        family=v%4
        def weight(h):
            if family==0:return 1.
            if family==1:return 1. if h%2 else .08
            if family==2:return np.sin(np.pi*h*(.18+.035*(v%5)))/(.65*h**.2)
            return (1.+1.4*np.exp(-((h-(3+v%5))/1.5)**2))*(-1 if h%3==0 else 1)
        return sum(weight(h)*part(h*detune,offset*h+.013*v*h)/h**power for h in range(1,min(24,int(14500/(hz*detune))+1)))
    def wide(power,spread):
        return sum(harmonics(power,1+j*spread,j*.21) for j in (-2,-1,0,1,2))/5
    if kind=="bass":
        x=.70*part(1)+(.18+.52*strength)*harmonics(1.2+.053*v)
        y=x+.15*part(1.003+.0004*v);env=np.minimum(1,t*(120-2*v))*(.92+.08*np.cos(2*np.pi*t*(1+v*.17)))
    elif kind=="keys":
        mod=(.25+strength*(.8+v*.08))*np.exp(-t*(2.2+.14*v))
        x=np.sin(ph+mod*np.sin(ph*(2+v%3)))+.22*part(2.008)*np.exp(-t*3)
        if v%4==1:x=.65*x+.35*part(3.01)*np.exp(-t*6)
        elif v%4==2:x=.68*x+.32*harmonics(2.1)*(.7+.3*np.cos(2*np.pi*t*4.7))
        elif v%4==3:x=.56*x+.35*harmonics(1.8)+.09*part(.5)
        y=.92*x+.18*part(1.002,.4);env=np.minimum(1,t*170)*np.exp(-t*(.65+.11*v))
    elif kind=="pad":
        spread=.002+v*.00035;power=1.35+v*.032
        x=wide(power,spread)+.22*part(.5 if v%4==0 else 2)
        y=wide(power,spread*1.23)+.2*part(1.003,.7);env=np.minimum(1,t*(2.7+v*.13))*(.9+.1*np.sin(t*(2+v*.31)))
    elif kind=="pluck":
        x=harmonics(1.15+v*.043)+.31*part(2.006+v*.006)*np.exp(-t*4)
        y=.9*x+.22*part(1.004,.5);env=np.minimum(1,t*250)*np.exp(-t*(3.1+v*.18))
    elif kind=="lead":
        x=wide(1+v*.028,.003+v*.0005)+.25*part(1)
        y=wide(1.05+v*.029,.004+v*.0005)+.2*part(2);env=np.minimum(1,t*(85-v*2))
    elif kind=="bell":
        x=sum(part(h,.05*v)*a*np.exp(-t*decay) for h,a,decay in [(1,.7,1.1),(2.12+v*.03,.24,3),(3.43+v*.04,.15,5),(5.17,.1,7)])
        y=x+.12*part(1.008,.5)*np.exp(-t*2);env=np.minimum(1,t*200)
    elif kind=="atmos":
        x=wide(1.85+v*.06,.007+v*.0008)+.17*part(2.006)
        y=wide(1.9+v*.04,.008+v*.0008)+.14*part(.5,.8);env=np.minimum(1,t*(2.2+v*.14))*(.75+.25*np.sin(t*(3+v*.4)))
    else:
        sweep=ph*(1+(.04+v*.035)*t);x=.65*np.sin(sweep)+.17*np.sin(sweep*1.51)+.025*rng.standard_normal(N)
        y=.7*x+.21*np.sin(sweep*1.007+.7);env=np.minimum(1,t*80)*np.exp(-t*(1.3+v*.6))
        if v==2:env=np.minimum(1,t*1.7)**.75; x=x[::-1].copy();y=y[::-1].copy()
    loop=kind in ("bass","pad","lead","atmos")
    if not loop:env*=np.minimum(1,(N-1-np.arange(N))/(RATE*.045))
    stereo=np.column_stack((x*env,y*env))
    freq=np.fft.rfftfreq(N,1/RATE);lp=.5+.5*np.cos(np.pi*np.clip((freq-13500)/3500,0,1))
    stereo=np.fft.irfft(np.fft.rfft(stereo,axis=0)*lp[:,None],n=N,axis=0)
    peak=float(np.max(np.abs(stereo)))
    if not np.isfinite(stereo).all() or peak<1e-5:raise ValueError("Invalid render")
    # Match active-window RMS, retain velocity contrast, prevent source clipping.
    block=RATE//10
    energy=np.array([np.mean(stereo[i:i+block]**2) for i in range(0,N,block)])
    active=energy[energy>=max(1e-12,float(energy.max())*.1)]
    rms=float(np.sqrt(active.mean()))
    goal=.22 if vel<64 else .38
    scale=min(goal/max(rms,1e-9),.92/peak)
    return (stereo*scale).astype(np.float32),loop

def loop_points(x):
    # Joint stereo matching of the two overlap windows. Search bounded candidate
    # positions close to the original start and end; crossfade remains in DSP.
    xf=RATE*80//1000; best=None
    for start in range(RATE*6//10,RATE*9//10+1,735):
        a=x[start:start+xf:8].astype(np.float64)
        for end in range(N-RATE//5,N+1,735):
            b=x[end-xf:end:8].astype(np.float64)
            e=float(np.mean((a-b)**2)/(np.mean(a*a+b*b)+1e-12))
            if best is None or e<best[0]:best=(e,start,end,xf)
    return best
def font(size):
    try:return ImageFont.truetype("DejaVuSans-Bold.ttf",size)
    except OSError:return ImageFont.load_default(size=size)
def cover(path):
    im=Image.new("RGB",(512,512),(11,14,23));d=ImageDraw.Draw(im)
    for j in range(18):
        k=j*9;d.ellipse((36+k,35+k,476-k,475-k),outline=(65+j*7,48+j*6,80+j*5),width=2)
    d.rectangle((18,18,494,494),outline=(204,169,92),width=3)
    d.rectangle((28,155,484,346),fill=(13,14,22))
    for y,s,n in [(51,"LIBERTY INFINITY",27),(183,"CINEMATIC",48),(243,"HORIZONS",48),(310,"100 ORIGINAL PRESETS",20),(454,"LIBERTY MUSIC SOFTWARE",16)]:
        d.text((256,y),s,font=font(n),anchor="mm",fill=(229,198,128))
    im.save(path)
def main():
    out=Path("cinematic-horizons");seen=set();index=0;loop_metrics=[]
    with tempfile.TemporaryDirectory() as temp:
        root=Path(temp);presets=[]
        for cat,prefix,kind,names in GROUPS:
            for v,name in enumerate(names.split("|")):
                zones=[]
                for note in (36,48,60,72,84):
                    for vel,lo,hi in [(40,1,63),(100,64,127)]:
                        x,loop=render(note,vel,index,v,kind)
                        digest=hashlib.sha256(x.tobytes()).digest()
                        if digest in seen:raise ValueError("Duplicate render")
                        seen.add(digest);wav=f"{index}-{note}-{vel}.wav";wavfile.write(root/wav,RATE,x)
                        zone=dict(wav=wav,root=note,vel_min=lo,vel_max=hi,source="Original Liberty offline render",rights="Original Liberty candidate sample")
                        if loop:
                            err,start,end,xf=loop_points(x)
                            zone.update(loop=[start,end],crossfade=xf)
                            loop_metrics.append(dict(preset=index,note=note,velocity=vel,normalized_overlap_error=err,start=start,end=end,crossfade=xf))
                        zones.append(zone)
                presets.append(dict(index=index,name=prefix+" "+name,category=cat,layers=[dict(zones=zones,gain=1.65 if kind in ('bass','keys','pluck','bell') else 2.0)]));index+=1
        assert index==100 and len(seen)==1000
        cover(root/"cover.png")
        manifest=dict(id="a0bdca61-77bd-4c21-9f5f-71b696950b11",name="Cinematic Horizons",entitlement="DEMO",cover="cover.png",presets=presets)
        (root/"manifest.json").write_text(json.dumps(manifest),encoding="utf8")
        build(root/"manifest.json",out)
    (out/"LOOP-QC.json").write_text(json.dumps(loop_metrics,indent=2),encoding="utf8")
    (out/"README.txt").write_text("Cinematic Horizons: 100 candidate presets, 1000 revised stereo offline renders. New immutable candidate UUID. DEVELOPMENT DEMO ONLY. No acoustic recordings or human listening certification.",encoding="utf8")
    print("PASS 100 presets, 1000 distinct offline stereo source renders")
if __name__=="__main__":main()
