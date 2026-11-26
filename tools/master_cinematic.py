"""Calibrate the master collection against actual Infinity-rendered WAVs.
Development-only receipt; preserve original UUID by creating a new version.
"""
from pathlib import Path
import argparse,csv,json,math
import numpy as np
from scipy.io import wavfile
from expansion_pack import CORE_FILES,sha256_file,verify_checksums
OLD="a0bdca61-77bd-4c21-9f5f-71b696950b11"
NEW="0b8e8b40-4376-44bc-b1e2-74bc6c94291e"
def measure(path):
    rate,data=wavfile.read(path)
    if rate!=48000 or data.dtype!=np.int16 or data.shape!=(384000,2):raise ValueError("Expected two 4s stereo passages")
    x=data.astype(np.float64)/32768
    # Use the hard-velocity passage, 100ms energy windows, relative -10 dB gate.
    hard=x[192000:];e=np.mean(hard.reshape(-1,4800,2)**2,axis=(1,2))
    active=e[e>=max(1e-12,e.max()*.1)]
    if not len(active) or active.mean()<1e-12:raise ValueError("Silent render")
    rms=10*math.log10(active.mean());peak=20*math.log10(max(1e-12,float(np.max(np.abs(x)))))
    return rms,peak
def calibrate(pack,listen,report):
    pack,listen=Path(pack),Path(listen);verify_checksums(pack)
    path=pack/"Library.map";lines=path.read_text(encoding="utf8").splitlines()
    if lines.count("I "+OLD)!=1 or any(x.startswith("G ") for x in lines):raise ValueError("Expected unmastered v0.9.11")
    values={}
    for i in range(100):
        rms,peak=measure(listen/f"preset-{i}.wav")
        # Target active RMS -25 dBFS; trim [-12,+12], single-note peak <= -12 dBFS.
        trim=round(max(-12,min(12,-25-rms,-12-peak)),3)
        values[i]=(rms,peak,trim)
    out=[];seen=set()
    for line in lines:
        if line=="I "+OLD:line="I "+NEW
        out.append(line)
        if line.startswith("P "):
            i=int(line.split(maxsplit=2)[1]);seen.add(i);out.append(f"G {i} {values[i][2]:.3f}")
    if seen!=set(range(100)):raise ValueError("Incomplete collection")
    path.write_text("\n".join(out)+"\n",encoding="utf8")
    (pack/"License.txt").write_text(f"LINFLIC1 {NEW} DEMO\n",encoding="ascii")
    (pack/"SHA256SUMS.json").write_text(json.dumps({n:sha256_file(pack/n) for n in CORE_FILES},indent=2),encoding="ascii")
    with Path(report).open("w",newline="") as f:
        writer=csv.writer(f);writer.writerow(["preset","before_active_rms_dbfs","before_peak_dbfs","trim_db"])
        for i,v in values.items():writer.writerow([i,*v])
    print("PASS 100 master trims, new immutable UUID, checksums refreshed")
def compare(before,listen,out):
    with Path(before).open() as f:rows=list(csv.DictReader(f))
    if len(rows)!=100:raise ValueError("Incomplete trim report")
    with Path(out).open("w",newline="") as f:
        w=csv.writer(f);w.writerow(["preset","before_active_rms_dbfs","after_active_rms_dbfs","before_peak_dbfs","after_peak_dbfs","trim_db","target_residual_db"])
        for row in rows:
            i=int(row["preset"]);rms,peak=measure(Path(listen)/f"preset-{i}.wav")
            if peak> -11.5:raise ValueError(f"Master peak too high: {i} {peak}")
            w.writerow([i,row["before_active_rms_dbfs"],rms,row["before_peak_dbfs"],peak,row["trim_db"],rms+25])
    print("PASS 100 independently rendered master presets, peak ceiling verified")
if __name__=="__main__":
    p=argparse.ArgumentParser();s=p.add_subparsers(dest="cmd",required=True)
    c=s.add_parser("calibrate");c.add_argument("pack");c.add_argument("listen");c.add_argument("report")
    q=s.add_parser("compare");q.add_argument("before");q.add_argument("listen");q.add_argument("out");a=p.parse_args()
    if a.cmd=="calibrate":calibrate(a.pack,a.listen,a.report)
    else:compare(a.before,a.listen,a.out)
