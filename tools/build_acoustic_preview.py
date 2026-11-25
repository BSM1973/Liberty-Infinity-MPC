"""Optional CC0 recorded-sample evaluation bank, never replaces Factory.
Pinned upstream Git blobs are checked before conversion. Requires numpy/scipy in CI only.
"""
import argparse,hashlib,json,shutil,urllib.request,urllib.parse
from pathlib import Path
import numpy as np
from scipy.io import wavfile
from import_multisamples import HEADER,ZONE_BYTES
REV="440300901dfe9275fd84e0b7763af1f8443ae62e"
BASE="https://raw.githubusercontent.com/sgossner/VSCO-2-CE/"+REV+"/"
# (source path, git blob sha, root MIDI, velocity group)
SOURCES={
78:[
("Brass/Trumpet/sus/Sum_SHTrumpet_sus_A2_v1_rr1.wav","59f4b5eadb7907b9656c5251869efa435a485c15",45,0),
("Brass/Trumpet/sus/Sum_SHTrumpet_sus_A2_v3_rr1.wav","85310d01faf50dbd83ed4a65ac7bed50d97904e1",45,1),
("Brass/Trumpet/sus/Sum_SHTrumpet_sus_G3_v1_rr1.wav","129e67aa9df78209517b461e38d8d1176a0e2861",55,0),
("Brass/Trumpet/sus/Sum_SHTrumpet_sus_G3_v3_rr1.wav","a301819024fe8d1fb8cdf86f9b5d0d8121c8c073",55,1),
("Brass/Trumpet/sus/Sum_SHTrumpet_sus_C5_v1_rr1.wav","bff7f04cb34938be10dca3793e3bdae722f88aa2",72,0),
("Brass/Trumpet/sus/Sum_SHTrumpet_sus_C5_v3_rr1.wav","9e43868b7c7e47a8ee107c68c6072756ef0f6bc1",72,1)],
72:[
("Strings/Cello Section/pizzT/pizzT_C1_v1_RR1.wav","c075a85b166054bbb9d1f900393a76f05f10a8b1",24,0),
("Strings/Cello Section/pizzT/pizzT_C1_v2_RR1.wav","e863415fcc1daf43bb56c7ad2990774f66c1cb98",24,1),
("Strings/Cello Section/pizzT/pizzT_C3_v1_RR1.wav","0e8a8a7eadd8ea2b5f009f70fd4b1c2e325ec137",48,0),
("Strings/Cello Section/pizzT/pizzT_C3_v2_RR1.wav","a0a21bf7abdff541715f3c49df50ca0a3b5ef0f7",48,1),
("Strings/Cello Section/pizzT/pizzT_D4_v1_RR1.wav","d431933dfc477cab274a7ba5e17bbad5ab68f768",62,0),
("Strings/Cello Section/pizzT/pizzT_D4_v2_RR1.wav","d8b97e0669415235b2d28ed65f247430264cc847",62,1)],
68:[("Keys/Upright Nr1/UR1_C2_pp_RR1.wav","aad159499f633ad8f5b5f1679ffd00e8c9077085",36,0),
("Keys/Upright Nr1/UR1_C2_f_RR1.wav","84cd2353a68dbe43054e9edea7537c7abbceb96a",36,1),
("Keys/Upright Nr1/UR1_C3_pp_RR1.wav","b9b046afcfb96255f219242cf1eaaa96c6c18435",48,0),
("Keys/Upright Nr1/UR1_C3_f_RR1.wav","dacaecf9ac8913b176970684a5767557a4b7adb9",48,1),
("Keys/Upright Nr1/UR1_C4_pp_RR1.wav","a5a8d82859df78cfd1c4ffdedd1b9b8ac209d6d4",60,0),
("Keys/Upright Nr1/UR1_C4_f_RR1.wav","d76c049063c299b9e78587cab7600882b3de0b58",60,1),
("Keys/Upright Nr1/UR1_C5_pp_RR1.wav","b18f8dd3cfc2115befe960866a646d80c5c078ee",72,0),
("Keys/Upright Nr1/UR1_C5_f_RR1.wav","aca4969e3bd81b18eae2a45ad41aee1d4c4a192d",72,1),
("Keys/Upright Nr1/UR1_C6_pp_RR1.wav","baaa90c2bb98c0b070013aec41a953803e0adf08",84,0),
("Keys/Upright Nr1/UR1_C6_f_RR1.wav","96a2005cbe74a9ccc5ac6f9ee554c5caeff65041",84,1)],
70:[("Strings/Solo Violin/Arco Vib/LLVln_ArcoVib_G3_p.wav","bc5f7d33610abbc3000f432562f36063f1d3d725",55,0),
("Strings/Solo Violin/Arco Vib/LLVln_ArcoVib_G3_f.wav","a09b038528097878ad6b12d30da4ac1a227d5a98",55,1),
("Strings/Solo Violin/Arco Vib/LLVln_ArcoVib_C5_p.wav","71f0671d77cf8b79ce1a18ba20d9661e24f086fc",72,0),
("Strings/Solo Violin/Arco Vib/LLVln_ArcoVib_C5_f.wav","93e9a2cf8106acc5e49ba18dab112c947473917f",72,1),
("Strings/Solo Violin/Arco Vib/LLVln_ArcoVib_C6_p.wav","33bc7e87816637462d6e174da9949eb9b5e45ef4",84,0),
("Strings/Solo Violin/Arco Vib/LLVln_ArcoVib_C6_f.wav","89f48f37313800df4c5901dc76b49b1c5c69e396",84,1)]}
def build(base,dest):
    base=Path(base);dest=Path(dest)
    with base.open("rb") as f:
        if f.read(32)!=HEADER or base.stat().st_size!=32+100*20*ZONE_BYTES:raise ValueError("Invalid Factory")
    dest.mkdir(parents=True,exist_ok=False);out=dest/"Factory.pcm";shutil.copyfile(base,out)
    log=[];records=["LINFMAP3"]
    import io
    with out.open("ab") as f:
        for patch,items in SOURCES.items():
            records.append(f"L {patch} 0 {patch} 0.8 0 0")
            samples=[]
            for path,sha,note,vel in items:
                req=urllib.request.Request(BASE+urllib.parse.quote(path),headers={"User-Agent":"Liberty-Infinity-QC"})
                with urllib.request.urlopen(req,timeout=60) as response:data=response.read(40_000_001)
                if len(data)>40_000_000 or hashlib.sha1(f"blob {len(data)}\0".encode()+data).hexdigest()!=sha:raise ValueError("Upstream blob mismatch: "+path)
                sr,a=wavfile.read(io.BytesIO(data))
                if a.ndim==1:a=a[:,None]
                if a.ndim!=2 or a.shape[1] not in (1,2) or not 8000<=sr<=192000:raise ValueError(path)
                if a.dtype.kind=="i":a=a.astype(np.float64)/(2**(8*a.dtype.itemsize-1))
                elif a.dtype.kind=="u":a=(a.astype(np.float64)-128)/128
                elif a.dtype.kind=="f":a=a.astype(np.float64)
                else:raise ValueError(path)
                # Limit evaluation takes to 8 s with a short end fade.
                a=a[:8*sr].copy()
                if len(a)<64 or not np.isfinite(a).all() or np.max(np.abs(a))<1e-6:raise ValueError(path)
                fade=min(sr//20,len(a)//4);a[-fade:]*=np.linspace(1,0,fade)[:,None]
                samples.append((path,sha,note,vel,sr,a))
            scale=min(4.,.891250938/max(float(np.max(np.abs(x[5]))) for x in samples))
            entries=[]
            for path,sha,note,vel,sr,a in samples:
                offset=f.tell();u=np.rint(np.clip(a*scale,-1,1-1/8388608)*8388608).astype(np.int32).astype(np.uint32).ravel()
                f.write(np.column_stack((u&255,(u>>8)&255,(u>>16)&255)).astype(np.uint8).tobytes())
                entries.append((note,vel,offset,len(a),a.shape[1],sr))
                log.append(dict(path=path,git_blob=sha,root=note,velocity_group=vel,frames=len(a),rate=int(sr),channels=a.shape[1]))
            for z,root in enumerate((36,48,60,72,84)):
                for v in range(4):
                    note,vel,offset,n,ch,sr=min((x for x in entries if x[1]==v//2),key=lambda x:abs(x[0]-root))
                    records.append(f"X {patch} {z} {v} {offset} {n} {ch} {sr} {note} 0 {n} 0")
    (dest/"Factory.pcm.map").write_text("\n".join(records)+"\n")
    (dest/"PROVENANCE.json").write_text(json.dumps({"source":"VSCO 2 Community Edition","upstream":BASE,"license":"CC0-1.0","credits":"Versilian Studios; Sam Gossner, Simon Dalzell; sample editing Elan Hickler/Soundemote","samples":log},indent=2))
    (dest/"README.txt").write_text("Evaluation bank only. Install both Factory.pcm and Factory.pcm.map with host stopped; back up original. Patch 68 PN Warm Upright = recorded upright piano; patch 70 ST Hollywood Ensemble = recorded solo violin (evaluation name mismatch); patch 78 BR Bright Fanfare = recorded trumpet; patch 72 ST Chamber Sketch = recorded cello section pizzicato. Other factory presets reusing those sources can change. Three to five roots/two recorded dynamics per instrument, extrapolated to existing grid; extreme registers are not realistic. No loops authored. Up to 8 seconds per take. Sources: VSCO 2 CE, CC0-1.0, https://versilian-studios.com/vsco-community/ . Original factory remains in standard build. No listening certification.")
if __name__=="__main__":
    p=argparse.ArgumentParser();p.add_argument("--base",required=True);p.add_argument("--out",required=True);a=p.parse_args();build(a.base,a.out)
