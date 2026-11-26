"""Offline MPC install and actual cover refresh. Stop host before use."""
from pathlib import Path
import argparse,json,tempfile,os,uuid
from PIL import Image,ImageDraw,ImageFont
from expansion_pack import install
def root_info(plugin):
    plugin=Path(plugin);m=json.loads((plugin/"plugin.json").read_text())
    if m.get("uid")!="LInf" or m.get("name")!="Liberty Infinity":raise ValueError("Select Liberty Infinity plugin folder")
    skin=plugin/"Plugin Skins"/"libraries.png"
    if not skin.is_file() or not (plugin/"Content/Factory.pcm").is_file():raise ValueError("Incomplete plugin")
    return plugin,skin
def metadata(path):
    if not path.is_dir():return None
    if not all((path/n).is_file() for n in ("Library.pcm","Library.map","License.txt","Cover.png","PROVENANCE.json")):raise ValueError("Incomplete pack")
    with (path/"Library.pcm").open("rb") as f:
        if f.read(8)!=b"LINFEX01":raise ValueError("Bad PCM header")
    if not 8<(path/"Library.pcm").stat().st_size<=2_000_000_000 or (path/"Library.map").stat().st_size>8*1024*1024:raise ValueError("Bad size")
    lines=(path/"Library.map").read_text(encoding="utf8").splitlines()
    if not lines or lines[0]!="LINFEXP1":raise ValueError("Bad map")
    ids=[x[2:] for x in lines if x.startswith("I ")];names=[x[2:] for x in lines if x.startswith("N ")]
    if len(ids)!=1 or str(uuid.UUID(ids[0]))!=ids[0] or len(names)!=1:raise ValueError("Bad identity")
    count=len([x for x in lines if x.startswith("P ")])
    if not 1<=count<=100:raise ValueError("Bad count")
    if (path/"License.txt").stat().st_size>1024:raise ValueError("Bad receipt")
    rec=(path/"License.txt").read_text(encoding="ascii").split()
    if len(rec)<3 or rec[1]!=ids[0] or rec[0] not in ("LINFLIC1","LINFLIC2"):raise ValueError("Bad receipt")
    if (path/"Cover.png").stat().st_size>8*1024*1024:raise ValueError("Cover too large")
    with Image.open(path/"Cover.png") as im:
        if im.format!="PNG" or im.size!=(512,512):raise ValueError("Expected 512x512 PNG")
        im.verify()
    return ids[0],names[0].strip('"'),count
def font(size):
    try:return ImageFont.truetype("DejaVuSans.ttf",size)
    except OSError:return ImageFont.load_default(size=size)
def refresh(plugin):
    plugin,skin=root_info(plugin)
    with Image.open(skin) as img:
        if img.size!=(1280,628):raise ValueError("Unsupported skin")
        im=img.convert("RGB")
    draw=ImageDraw.Draw(im);gold=(204,169,92)
    for i in range(1,5):
        x=32+246*i;y=98;path=plugin/"Content/Expansions"/f"{i:02d}"
        draw.rounded_rectangle((x,y,x+232,y+224),radius=8,outline=gold,width=2,fill=(25,26,31))
        try:info=metadata(path)
        except (ValueError,OSError):info=None;status="PACK INVALIDE"
        else:status="EMPLACEMENT VIDE"
        if info:
            with Image.open(path/"Cover.png") as cover:im.paste(cover.convert("RGB").resize((150,150),Image.Resampling.LANCZOS),(x+41,y+8))
            name=info[1]
            while name and draw.textlength(name,font=font(16))>216:name=name[:-1]
            draw.text((x+116,y+166),name,font=font(16),anchor="mt",fill=gold)
            status=f"{info[2]} PRESETS | LIB {i}"
        else:draw.text((x+116,y+75),f"LIBRARY {i}",font=font(24),anchor="mm",fill=gold)
        draw.text((x+116,y+195),status,font=font(14),anchor="mt",fill=(240,240,240))
    with tempfile.NamedTemporaryFile(dir=skin.parent,suffix=".png",delete=False) as f:tmp=Path(f.name)
    try:im.save(tmp);os.replace(tmp,skin)
    finally:tmp.unlink(missing_ok=True)
def add(pack,plugin,slot):
    plugin,_=root_info(plugin);pack=Path(pack);info=metadata(pack)
    if not info:raise ValueError("Pack absent")
    for i in range(1,5):
        try:old=metadata(plugin/"Content/Expansions"/f"{i:02d}")
        except (ValueError,OSError):continue
        if old and old[0]==info[0]:raise ValueError("Collection already installed")
    install(pack,plugin/"Content",slot);refresh(plugin)
if __name__=="__main__":
    p=argparse.ArgumentParser();s=p.add_subparsers(dest="cmd",required=True)
    i=s.add_parser("install");i.add_argument("pack");i.add_argument("plugin");i.add_argument("slot",type=int)
    f=s.add_parser("refresh");f.add_argument("plugin");a=p.parse_args()
    if a.cmd=="install":add(a.pack,a.plugin,a.slot)
    else:refresh(a.plugin)
