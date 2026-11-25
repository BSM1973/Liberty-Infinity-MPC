"""Generate optional per-preset trims from the 100 Factory audition WAVs.
No source or default Factory is modified; this is RMS matching, not LUFS.
"""
from pathlib import Path
import argparse,csv,math,statistics,wave,struct
def build(src,out):
    src,out=Path(src),Path(out)
    stats=[]
    for i in range(100):
        with wave.open(str(src/f"preset-{i}.wav"),"rb") as f:
            if (f.getnchannels(),f.getsampwidth(),f.getframerate())!=(2,2,48000):raise ValueError("Unexpected WAV format")
            v=[x[0]/32768 for x in struct.iter_unpack("<h",f.readframes(f.getnframes()))]
        if not v:raise ValueError("Empty audition")
        rms=10*math.log10(max(1e-20,sum(x*x for x in v)/len(v)))
        peak=20*math.log10(max(1e-10,max(map(abs,v))))
        if rms<-75:raise ValueError(f"Silent preset {i}")
        stats.append((i,rms,peak))
    target=statistics.median(r[1] for r in stats)
    out.mkdir(parents=True,exist_ok=False)
    with (out/"level-trims.csv").open("w",newline="",encoding="utf-8") as f:
        w=csv.writer(f);w.writerow(["preset","audition_rms_dbfs","audition_peak_dbfs","target_rms_dbfs","recommended_trim_db"])
        records=["LINFMAP1"]
        for i,rms,peak in stats:
            trim=max(-12,min(6,target-rms,-1-peak))
            w.writerow([i,round(rms,3),round(peak,3),round(target,3),round(trim,3)])
            records.append(f"G {i} {trim:.3f}")
    (out/"Factory.pcm.map").write_text("\n".join(records)+"\n",encoding="ascii")
    (out/"README.txt").write_text("Optional v0.9.5 Factory level-match map only. Use with the original unmodified Factory.pcm and no existing map. Stop the host, back up original content, copy map beside PCM. Remove map to restore original. Do not use with acoustic or personal banks. Trims are based on 100 audition WAVs, limited to -12/+6 dB; applied pre-effects and saturation, so final RMS is not guaranteed to equal the target. Not LUFS or listening certification. Older versions do not accept G records.")
if __name__=="__main__":
    p=argparse.ArgumentParser();p.add_argument("--src",required=True);p.add_argument("--out",required=True);a=p.parse_args();build(a.src,a.out)
