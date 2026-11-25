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
im=Image.new("RGB",(1280,628)); d=ImageDraw.Draw(im)
for y in range(628):
    v=round(13+9*(1-y/628)+1.5*math.sin(y*2.3))
    d.line((0,y,1279,y),fill=(v,v,v+2))
d.rounded_rectangle((2,2,1277,625),radius=8,outline=(115,88,43),width=1)
for title,x,y,w in L["sections"]:
    bottom=359 if y<100 else 613
    d.rounded_rectangle((x-3,y-3,x+w+3,bottom),radius=5,fill=(12,12,14),outline=(89,69,38))
    for j in range(20):
        v=round(33-13*j/19)
        d.line((x,y+j,x+w,y+j),fill=(v,v,v+2))
    d.line((x,y+20,x+w,y+20),fill=G)
    d.text((x+9,y+3),title,font=font(13),fill=WHITE)
x,y,w,h=L["lcd"]
d.rounded_rectangle((x-6,y-6,x+w+6,y+h+6),radius=12,fill=(4,4,5),outline=G,width=2)
d.rounded_rectangle((x,y,x+w,y+h),radius=8,fill=(12,16,22),outline=(97,75,43))
for j in range(h-20):
    d.line((x+10,y+10+j,x+w-10,y+10+j),fill=lerp((24,29,37),(9,12,18),j/(h-20)))
d.text((x+23,y+12),"LIBERTY INFINITY",font=font(22),fill=WHITE)
d.text((x+w-177,y+18),"BLACK GOLD",font=font(15),fill=G)
d.line((x+20,y+47,x+w-20,y+47),fill=(137,107,54))
d.line((x+210,y+58,x+210,y+253),fill=(74,61,42))
d.line((x+20,y+260,x+w-20,y+260),fill=(100,78,44))
d.text((1046,596),"LIBERTY MUSIC SOFTWARE",font=font(11),fill=WHITE)
im.save(A/"retro.png")
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
for size in (36,40):
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
for w,h in ((36,34),(40,36)):
    for on in (True,False):
        hardware(on).resize((w,h),Image.Resampling.LANCZOS).save(A/f"power_{w}_{'on' if on else 'off'}.png")
sel=Image.new("RGBA",(72,42));q=ImageDraw.Draw(sel)
q.rounded_rectangle((1,1,70,39),radius=4,fill=(25,25,28),outline=G,width=1)
q.line((5,4,66,4),fill=(90,75,48));q.polygon([(60,14),(66,14),(63,19)],fill=LIGHT)
sel=sel.resize((60,32),Image.Resampling.LANCZOS);sel.save(A/"selector_60.png")
# Validate the exact shared slot geometry, including separation and screen bounds.
C=L["controls"]
assert len(C)==39 and len({c["i"] for c in C})==39
for c in C:
    x,y,w,h=(c[k] for k in ("x","y","w","h"))
    assert x>=0 and y>=0 and x+w<=1280 and y+h<=628
    if c["kind"]!="browser":
        assert w>=72 and h>=76
        face=c["face"];fw,fh,fy=face["w"],face["h"],face["y"]
        assert fw<=w and fy>=0 and fy+fh<=face["labelY"]
        assert face["labelY"]+14<=face["valueY"] and face["valueY"]+11<=h
        if c["kind"]=="knob":assert Image.open(A/face["asset"]).size==(fw,fh*127)
        elif c["kind"]=="toggle":
            for state in ("on","off"):assert Image.open(A/f"{face['asset']}_{state}.png").size==(fw,fh)
        else:assert Image.open(A/face["asset"]).size==(fw,fh)
for a,b in itertools.combinations(C,2):
    assert a["x"]+a["w"]<=b["x"] or b["x"]+b["w"]<=a["x"] or a["y"]+a["h"]<=b["y"] or b["y"]+b["h"]<=a["y"],(a,b)
# Static design preview, not a screenshot of the running host.
preview=im.convert("RGBA");p=ImageDraw.Draw(preview)
for c in C:
    if c["kind"]=="browser":continue
    x,y,w,h=(c[k] for k in ("x","y","w","h"))
    face=c["face"];fw,fh,fy=(face[k] for k in ("w","h","y"))
    if c["kind"]=="toggle":
        state="on" if c["i"] in (8,15) else "off"
        asset=Image.open(A/f"{face['asset']}_{state}.png").convert("RGBA")
    elif c["kind"]=="selector":asset=sel
    else:
        film=Image.open(A/face["asset"]);asset=film.crop((0,60*fh,fw,61*fh))
    preview.alpha_composite(asset,(x+(w-fw)//2,y+fy))
    p.text((x+w/2,y+face["labelY"]),c["name"],font=font(12),fill=WHITE,anchor="mt")
    if c["kind"]=="selector":
        p.text((x+w/2-3,y+fy+fh/2),{19:"LP",20:"12",24:"Stereo",32:"Hall"}[c["i"]],font=font(10),fill=WHITE,anchor="mm")
    else:
        p.text((x+w/2,y+face["valueY"]),"ON" if c["kind"]=="toggle" and c["i"] in (8,15) else "OFF" if c["kind"]=="toggle" else "50 %",font=font(10),fill=WHITE,anchor="mt")
p.rounded_rectangle((307,100,492,122),radius=3,outline=G);p.text((315,104),"All styles",font=font(12),fill=WHITE)
p.rounded_rectangle((500,99,961,123),radius=4,outline=G);p.text((510,104),"Rechercher un preset",font=font(12),fill=WHITE)
for i,n in enumerate(("All","Bass","Keys","Pads","Plucks","Leads","Pianos","Strings","Brass","Arpeggios","Bells & Mallets","Atmospheres","Synth & FX")):
    p.text((310,128+i*13),n,font=font(11),fill=WHITE)
p.rectangle((500,128,960,148),fill=(86,64,30))
for i,n in enumerate(("BA Deep Bass","KY Crystal Keys","PD Velvet Pad","PL Silver Pluck","BA Analog Titan","BA Berlin Bass")):
    p.text((508,129+i*22),n,font=font(15),fill=WHITE)
for x,y,w,label in ((307,302,185,"Tous / Favoris"),(500,264,68,"< Son"),(574,264,68,"Page -"),(817,264,68,"Page +"),(891,264,70,"Son >"),(500,288,108,"Ecouter"),(613,288,112,"Enregistrer"),(730,288,112,"Mes presets"),(847,288,114,"Favori : non")):
    p.rounded_rectangle((x,y,x+w,y+20),radius=3,outline=G);p.text((x+w/2,y+4),label,font=font(11),fill=WHITE,anchor="mt")
p.text((730,266),"1 / 17 | 100 sons",font=font(11),fill=WHITE,anchor="mt")
p.text((730,312),"BA Deep Bass",font=font(14),fill=WHITE,anchor="mt")
preview.convert("RGB").save(A/"black-gold-preview.png")
print("Black Gold assets generated; 39 unique slots, no overlapping slot bounds")
