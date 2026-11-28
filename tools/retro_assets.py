"""Shared Black Gold panel and original hardware-style assets."""
from pathlib import Path
import json, math, itertools
from PIL import Image, ImageDraw, ImageFont
ROOT=Path(__file__).resolve().parents[1]
P=ROOT/"plugins/Infinity"; A=P/"assets"; A.mkdir(exist_ok=True)
L=json.loads((P/"layout.json").read_text())
def font(n):
    for p in ("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf","/System/Library/Fonts/Supplemental/Arial Bold.ttf"):
        if Path(p).exists():return ImageFont.truetype(p,n)
    return ImageFont.load_default(size=n)
def lerp(a,b,t):return tuple(round(x+(y-x)*t) for x,y in zip(a,b))
G=(202,161,85); LIGHT=(246,226,179); WHITE=(249,249,249)
# Shared full-screen page backgrounds, independent hardware controls.
def panel_background(title):
    im=Image.new("RGB",(1280,628),(15,16,19));d=ImageDraw.Draw(im)
    for y in range(628):
        v=round(14+7*(1-y/628));d.line((0,y,1279,y),fill=(v,v,v+3))
    d.rounded_rectangle((3,3,1276,624),radius=10,outline=(115,88,43),width=2)
    d.text((26,65),"LIBERTY INFINITY",font=font(29),fill=LIGHT)
    d.text((1250,78),title,font=font(22),fill=G,anchor="rm")
    d.line((24,108,1256,108),fill=G,width=2)
    return im
# Original Factory icon: stylised gold PCM cartridge, not an external logo.
factory=Image.new("RGB",(512,512),(12,15,22));fd=ImageDraw.Draw(factory)
for k in range(12):
    z=16+k*5;fd.rounded_rectangle((z,z,512-z,512-z),radius=30,outline=(75+k*10,57+k*8,32+k*4),width=2)
fd.rounded_rectangle((91,95,421,364),radius=22,fill=(30,31,36),outline=LIGHT,width=4)
fd.rounded_rectangle((115,118,397,274),radius=9,fill=(9,13,19),outline=G,width=3)
for i in range(45):
    x=128+i*5.7;h=13+54*abs(math.sin(i*.71)*math.cos(i*.16))
    fd.line((x,196-h,x,196+h),fill=LIGHT,width=3)
for x in range(125,400,35):fd.rounded_rectangle((x,298,x+20,338),radius=3,fill=G)
fd.text((256,402),"FACTORY 1",font=font(42),fill=LIGHT,anchor="mm")
fd.text((256,452),"100 ORIGINAL PRESETS",font=font(19),fill=WHITE,anchor="mm")
factory.save(A/"factory-1.png")
# Anti-aliased original brass knobs. Gold machined rim, dark radial brushing,
# satin metallic cap, engraved pointer and a separate arc of travel.
S=64;N=127;Q=3;D=S*Q
strip=Image.new("RGBA",(S,S*N))
def angle(x,y):return (math.degrees(math.atan2(y,x))+360)%360
def knob(t):
    k=Image.new("RGBA",(D,D)); pix=k.load()
    for y in range(D):
        for x in range(D):
            dx=(x+.5)/Q-32;dy=(y+.5)/Q-31; r=math.hypot(dx,dy); a=math.atan2(dy,dx)
            if r>30.5:continue
            if r>27.4:
                col=(14,14,15,255) if r<29 else (0,0,0,110)
            elif r>23:
                sheen=.50+.24*math.sin(a*2+.8)+.16*math.sin(a*5-.4)
                col=(*lerp((104,72,29),(247,218,148),max(0,min(1,sheen))),255)
            elif r>21.5:col=(40,32,20,255)
            else:
                sheen=.50+.30*math.cos(a+1.15)+.07*math.sin(r*19)
                v=round(18+28*max(0,min(1,sheen)))
                col=(v,v,v+2,255)
            pix[x,y]=col
    q=ImageDraw.Draw(k)
    for j in range(11):
        a=math.radians(135+j*27)
        cx=(32+30*math.cos(a))*Q;cy=(31+30*math.sin(a))*Q
        rr=1.05*Q
        q.ellipse((cx-rr,cy-rr,cx+rr,cy+rr),fill=LIGHT if j<=t*10 else (99,86,62))
    a=math.radians(135+270*t)
    q.line(((32+8*math.cos(a))*Q,(31+8*math.sin(a))*Q,(32+20*math.cos(a))*Q,(31+20*math.sin(a))*Q),fill=LIGHT,width=3*Q)
    return k.resize((S,S),Image.Resampling.LANCZOS)
for i in range(N):strip.paste(knob(i/(N-1)),(0,S*i))
# Native-size filmstrips keep actual MPC and Mac knob dimensions identical.
for size in (36,40,64,104):
    compact=Image.new("RGBA",(size,size*N))
    for i in range(N):
        compact.paste(strip.crop((0,i*S,S,(i+1)*S)).resize((size,size),Image.Resampling.LANCZOS),(0,i*size))
    compact.save(A/f"gold_knob_{size}.png")
def hardware(on):
    k=Image.new("RGBA",(52,48));q=ImageDraw.Draw(k)
    q.rounded_rectangle((2,3,50,47),radius=5,fill=(0,0,0,170))
    q.rounded_rectangle((2,1,49,43),radius=4,fill=(46,35,20),outline=G if on else (112,89,50),width=2)
    for j in range(3,41):
        q.line((5,j,46,j),fill=lerp((45,45,48),(15,15,17),j/42))
    q.rounded_rectangle((9,7,42,35),radius=3,outline=(71,67,58),width=1)
    if on:q.rounded_rectangle((17,11,34,17),radius=3,fill=(117,75,20))
    q.rounded_rectangle((18,12,33,15),radius=1,fill=(255,218,137) if on else (57,49,36))
    q.line((20,28,31,28),fill=WHITE if on else (156,150,140),width=2)
    return k
for w,h in ((36,34),(40,36),(76,70)):
    for on in (True,False):
        hardware(on).resize((w,h),Image.Resampling.LANCZOS).save(A/f"power_{w}_{'on' if on else 'off'}.png")
sel=Image.new("RGBA",(72,42));q=ImageDraw.Draw(sel)
q.rounded_rectangle((1,1,70,39),radius=4,fill=(25,25,28),outline=G,width=1)
q.line((5,4,66,4),fill=(90,75,48));q.polygon([(60,14),(66,14),(63,19)],fill=LIGHT)
sel.resize((60,32),Image.Resampling.LANCZOS).save(A/"selector_60.png")
sel.resize((156,62),Image.Resampling.LANCZOS).save(A/"selector_156.png")
# Exact per-page geometry and independently rendered design previews.
for page in L["pages"][:-1]:
    controls=page["controls"];im=panel_background(page["name"]);d=ImageDraw.Draw(im)
    if page["asset"]=="main":
        for rect in ((16,121,1036,610),(1046,121,1260,610)):
            d.rounded_rectangle(rect,radius=9,fill=(11,13,17),outline=(104,81,45),width=2)
        d.text((1153,130),"OUTPUT",font=font(16),fill=G,anchor="mt")
        im.paste(factory.resize((166,166),Image.Resampling.LANCZOS),(1070,168))
    else:
        if page["asset"]=="filter":
            for rect,title,cx in (((16,120,644,596),"FILTER MODIFIER",330),((654,120,1260,596),"MASTER FILTER",957)):
                d.rounded_rectangle(rect,radius=9,fill=(15,16,20),outline=G,width=2)
                d.text((cx,132),title,font=font(20),fill=LIGHT,anchor="mt")
        for c in controls:
            x,y,w,h=(c[k] for k in ("x","y","w","h"))
            d.rounded_rectangle((x+3,y+3,x+w-3,y+h-3),radius=10,fill=(12,14,18),outline=(87,68,38),width=2)
    im.save(A/(page["asset"]+".png"))
    preview=im.convert("RGBA");draw=ImageDraw.Draw(preview)
    for c in controls:
        x,y,w,h=(c[k] for k in ("x","y","w","h"))
        assert x>=0 and y>=108 and x+w<=1280 and y+h<=628
        if c["kind"]=="browser":continue
        f=c["face"];fw,fh,fy=f["w"],f["h"],f["y"]
        assert fw<=w and fy+fh<=f["labelY"] and f["labelY"]+f["labelH"]<=f["valueY"] and f["valueY"]+f["valueH"]<=h
        if c["kind"]=="toggle":asset=Image.open(A/(f["asset"]+("_on.png" if c["i"] in (8,15) else "_off.png")))
        elif c["kind"]=="selector":asset=Image.open(A/f["asset"])
        else:
            film=Image.open(A/f["asset"]);assert film.size==(fw,fh*127);asset=film.crop((0,60*fh,fw,61*fh))
        preview.alpha_composite(asset.convert("RGBA"),(x+(w-fw)//2,y+fy))
        draw.text((x+w/2,y+f["labelY"]),c["name"],font=font(f["labelSize"]),fill=WHITE,anchor="mt")
        if c["kind"]=="selector":
            draw.text((x+w/2-8,y+fy+fh/2),{19:"LP",20:"12",24:"Stereo",32:"Hall"}[c["i"]],font=font(17),fill=WHITE,anchor="mm")
        else:
            draw.text((x+w/2,y+f["valueY"]),("ON" if c["i"] in (8,15) else "OFF") if c["kind"]=="toggle" else "50 %",font=font(f["valueSize"]),fill=LIGHT,anchor="mt")
    if page["asset"]=="main":
        draw.text((526,159),"FACTORY 1 / LIBRARIES",font=font(25),fill=G,anchor="mt")
        draw.text((526,270),"BA Deep Bass",font=font(46),fill=WHITE,anchor="mt")
        draw.text((526,425),"100 FACTORY PRESETS",font=font(22),fill=LIGHT,anchor="mt")
    preview.convert("RGB").save(A/(page["asset"]+"-preview.png"))
    for a,b in itertools.combinations(controls,2):
        assert a["x"]+a["w"]<=b["x"] or b["x"]+b["w"]<=a["x"] or a["y"]+a["h"]<=b["y"] or b["y"]+b["h"]<=a["y"],(page["name"],a["i"],b["i"])
assert sorted(c["i"] for c in L["controls"])==list(range(39))
Image.open(A/"main-preview.png").save(A/"black-gold-preview.png")
print("PASS five distinct panels, 39 unique parameter slots, no overlaps")
# Dedicated MPC Library Manager: static slot artwork, not installed pack covers.
lib=Image.new("RGB",(1280,628),(15,15,18));ld=ImageDraw.Draw(lib)
ld.rounded_rectangle((4,4,1275,623),radius=8,outline=G,width=2)
ld.text((32,56),"LIBERTY INFINITY  |  LIBRARY MANAGER",font=font(24),fill=LIGHT)
ld.text((1250,72),"FACTORY 1 + 4 EMPLACEMENTS PCM INDÉPENDANTS",font=font(15),fill=WHITE,anchor="rm")
for i in range(5):
    x=32+246*i;y=98
    ld.rounded_rectangle((x,y,x+232,y+224),radius=8,outline=G,width=2,fill=(25,26,31))
    if i==0:lib.paste(factory.resize((150,150),Image.Resampling.LANCZOS),(x+41,y+8))
    for j in range(6) if i else ():
        inset=22+j*10
        ld.rounded_rectangle((x+inset,y+17+j*7,x+232-inset,y+147-j*7),radius=10,outline=(95+j*13,76+j*10,42+j*6),width=2)
    title="FACTORY 1" if i==0 else f"LIBRARY {i}"
    ld.text((x+116,y+158),title,font=font(23),anchor="mt",fill=LIGHT)
    ld.text((x+116,y+192),"100 PRESETS" if i==0 else "EMPLACEMENT",font=font(15),anchor="mt",fill=WHITE)
for x in (32,340,648,956):ld.rounded_rectangle((x,343,x+292,459),radius=5,outline=G,fill=(22,23,27))
ld.rounded_rectangle((32,470,1248,546),radius=5,outline=G,fill=(12,16,22))
ld.text((32,566),"INSTALLATION : COPIER LA LIBRARY DANS CONTENT/EXPANSIONS, PUIS REOUVRIR LE PLUGIN",font=font(15),fill=WHITE)
ld.text((32,590),"COUVERTURES MISES A JOUR HORS LIGNE. EMPLACEMENTS FIXES POUR LES PROJETS.",font=font(13),fill=G)
lib.save(A/"libraries.png")
