"""Build and install demo PCM expansions for Liberty Infinity MPC.
Authoring computer only. Never overwrite Factory or an occupied slot.
"""
import argparse,hashlib,json,math,shutil,tempfile,uuid
from pathlib import Path
import numpy as np
from scipy.io import wavfile
from PIL import Image,ImageDraw,ImageFont
ROOTS=(36,48,60,72,84)
VELS=(16,48,80,112)
LIMIT=48*1024*1024
def quoted(s):
    if not isinstance(s,str) or not s or len(s.encode("utf8"))>80 or any(ord(c)<32 or ord(c)==127 for c in s):raise ValueError("Invalid name")
    return '"'+s.replace("\\","\\\\").replace('"','\\"')+'"'
def integer(x,lo,hi):
    if type(x)!=int or not lo<=x<=hi:raise ValueError("Invalid integer")
    return x
def real(x,lo,hi):
    if type(x) not in (float,int) or not math.isfinite(x) or not lo<=x<=hi:raise ValueError("Invalid number")
    return x
def wave_data(path):
    sr,a=wavfile.read(path)
    integer(sr,8000,192000)
    if a.ndim==1:a=a[:,None]
    if a.ndim!=2 or a.shape[1] not in (1,2) or not 32<=len(a)<=sr*120:raise ValueError("Invalid WAV")
    if a.dtype.kind=="i":a=a.astype(np.float64)/(2**(8*a.dtype.itemsize-1))
    elif a.dtype.kind=="u" and a.dtype.itemsize==1:a=(a.astype(np.float64)-128)/128
    elif a.dtype.kind=="f":a=a.astype(np.float64)
    else:raise ValueError("Invalid WAV PCM")
    if not np.isfinite(a).all():raise ValueError("Nonfinite audio")
    x=np.rint(np.clip(a,-1,1-1/8388608)*8388608).astype(np.int32).astype(np.uint32).ravel()
    raw=np.column_stack((x&255,(x>>8)&255,(x>>16)&255)).astype(np.uint8).tobytes()
    return sr,len(a),a.shape[1],raw
def build(manifest,out):
    manifest=Path(manifest);out=Path(out);d=json.loads(manifest.read_text(encoding="utf8"))
    id=str(uuid.UUID(d["id"]));name=d["name"];quoted(name)
    if d.get("entitlement")!="DEMO":raise ValueError("Commercial signed licenses are not enabled")
    patches=d["presets"]
    if not 1<=len(patches)<=100:raise ValueError("1–100 presets")
    if out.exists():raise FileExistsError(out)
    out.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(dir=out.parent) as t:
        temp=Path(t)/"pack";temp.mkdir()
        lines=["LINFEXP1",f"I {id}",f"N {quoted(name)}"];provenance=[];seen=set();cache={}
        with (temp/"Library.pcm").open("wb") as f:
            f.write(b"LINFEX01")
            for p in patches:
                i=integer(p["index"],0,99)
                if i in seen:raise ValueError("Duplicate preset index")
                seen.add(i);lines.append(f"P {i} {quoted(p['name'])}");lines.append(f"C {i} {integer(p.get('category',0),0,12)}")
                layers=p["layers"]
                if not 1<=len(layers)<=4:raise ValueError("1–4 layers")
                roles=[];resident={}
                for l,layer in enumerate(layers):
                    gain=real(layer.get("gain",1),0,2);pan=real(layer.get("pan",0),-1,1);cents=real(layer.get("cents",0),-1200,1200);role=integer(layer.get("role",0),0,4);roles.append(role)
                    lines.append(f"L {i} {l} {gain} {pan} {cents} {role}")
                    zones=layer["zones"]
                    if not 1<=len(zones)<=128:raise ValueError("1–128 source zones")
                    sources=[]
                    for z in zones:
                        path=(manifest.parent/z["wav"]).resolve()
                        if not z.get("rights") or not z.get("source"):raise ValueError("Source and redistribution rights required")
                        root=integer(z["root"],0,127)
                        kl=integer(z.get("key_min",0),0,127);kh=integer(z.get("key_max",127),kl,127)
                        vl=integer(z.get("vel_min",1),1,127);vh=integer(z.get("vel_max",127),vl,127)
                        if path not in cache:
                            sr,n,ch,raw=wave_data(path);off=f.tell()
                            if off+len(raw)>2_000_000_000:raise ValueError("2 GB bank limit")
                            f.write(raw);cache[path]=(off,n,ch,sr,hashlib.sha256(path.read_bytes()).hexdigest())
                        off,n,ch,sr,digest=cache[path]
                        start,end=z.get("loop",[0,n]);start=integer(start,0,n-1);end=integer(end,max(32,start+1),n)
                        xf=integer(z.get("crossfade",0),0,(end-start)//3)
                        sources.append((root,kl,kh,vl,vh,off,n,ch,sr,start,end,xf))
                        provenance.append(dict(preset=i,layer=l,file=path.name,sha256=digest,source=z["source"],rights=z["rights"],root=root))
                    for zi,note in enumerate(ROOTS):
                        for vi,vel in enumerate(VELS):
                            eligible=[x for x in sources if x[1]<=note<=x[2] and x[3]<=vel<=x[4]]
                            if not eligible:raise ValueError(f"Uncovered zone {i}/{l}/{note}/{vel}")
                            root,kl,kh,vl,vh,off,n,ch,sr,start,end,xf=min(eligible,key=lambda x:abs(x[0]-note))
                            resident[off]=n*ch*4
                            lines.append(f"X {i} {l} {zi} {vi} {off} {n} {ch} {sr} {root} {start} {end} {xf}")
                if sum(resident.values())>LIMIT:raise ValueError("48 MiB per preset")
                if roles.count(1)==1 or roles.count(2)!=roles.count(3) or roles.count(2)>1 or roles.count(4)>1 or roles.count(4)==len(roles):raise ValueError("Invalid layer roles")
        (temp/"Library.map").write_text("\n".join(lines)+"\n",encoding="utf8")
        (temp/"License.txt").write_text(f"LINFLIC1 {id} DEMO\n",encoding="ascii")
        (temp/"PROVENANCE.json").write_text(json.dumps(provenance,ensure_ascii=False,indent=2),encoding="utf8")
        # Generic typographic cover, not a dynamic MPC skin.
        if d.get("cover"):
            with Image.open(manifest.parent/d["cover"]) as im:
                im.convert("RGB").resize((512,512),Image.Resampling.LANCZOS).save(temp/"Cover.png")
        else:
            im=Image.new("RGB",(512,512),(17,17,20));draw=ImageDraw.Draw(im)
            draw.rectangle((14,14,498,498),outline=(201,165,87),width=3)
            def font(size):
                try:return ImageFont.truetype("DejaVuSans.ttf",size)
                except OSError:return ImageFont.load_default(size=size)
            draw.text((35,40),"LIBERTY INFINITY",font=font(28),fill=(220,186,112))
            words=name.split();rows=[];row=""
            for word in words:
                test=(row+" "+word).strip()
                if draw.textlength(test,font=font(24))>440 and row:rows.append(row);row=word
                else:row=test
            rows.append(row)
            for j,line in enumerate(rows[:8]):draw.text((35,130+j*32),line,font=font(24),fill="white")
            draw.text((35,445),"DEMO COLLECTION",font=font(18),fill=(220,186,112));im.save(temp/"Cover.png")
        temp.rename(out)
def install(pack,content,slot):
    pack=Path(pack);content=Path(content);slot=integer(slot,1,4)
    if not (content/"Factory.pcm").is_file() and not content.name=="Liberty Infinity":raise ValueError("Select MPC Content or Mac test support root")
    dest=content/"Expansions"/f"{slot:02d}"
    if dest.exists():raise FileExistsError(dest)
    names=("Library.pcm","Library.map","License.txt","PROVENANCE.json","Cover.png")
    if not all((pack/n).is_file() for n in names):raise ValueError("Incomplete pack")
    dest.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(dir=dest.parent) as t:
        temp=Path(t)/"pack";temp.mkdir()
        for n in names:shutil.copyfile(pack/n,temp/n)
        if dest.exists():raise FileExistsError(dest)
        temp.rename(dest)
    print(dest)
def list_slots(content):
    for s in range(1,5):
        path=Path(content)/"Expansions"/f"{s:02d}";name="vide"
        if path.exists():
            try:
                lines=(path/"Library.map").read_text(encoding="utf8").splitlines()
                name=next(x[2:] for x in lines if x.startswith("N "))
            except (OSError,StopIteration,UnicodeError):name="invalide"
        print(f"{s:02d}: {name}")
if __name__=="__main__":
    p=argparse.ArgumentParser();sp=p.add_subparsers(dest="cmd",required=True)
    b=sp.add_parser("build");b.add_argument("manifest");b.add_argument("out")
    i=sp.add_parser("install");i.add_argument("pack");i.add_argument("content");i.add_argument("slot",type=int)
    l=sp.add_parser("list");l.add_argument("content")
    a=p.parse_args()
    if a.cmd=="build":build(a.manifest,a.out)
    elif a.cmd=="install":install(a.pack,a.content,a.slot)
    else:list_slots(a.content)
