from pathlib import Path
import json, math
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1]
P=ROOT/"plugins/Infinity"
A=P/"assets";A.mkdir(exist_ok=True)
L=json.loads((P/"layout.json").read_text())
def font(n):
    for p in ("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf","/System/Library/Fonts/Supplemental/Arial Bold.ttf"):
        if Path(p).exists():return ImageFont.truetype(p,n)
    return ImageFont.load_default()
im=Image.new("RGB",(1280,628));d=ImageDraw.Draw(im)
for y in range(628):
    v=int(239-28*y/628+4*math.sin(y*.06))
    d.line((0,y,1280,y),fill=(v,min(255,v+3),min(255,v+6)))
for title,x,y,w in L["sections"]:
    d.rounded_rectangle((x,y,x+w,y+24),radius=3,fill=(46,58,69))
    d.text((x+8,y+3),title,font=font(15),fill="white")
for x in (276,1004):
    d.line((x,40,x,354),fill=(161,173,183),width=2)
for x in (279,653,1029):
    d.line((x,410,x,611),fill=(164,176,187),width=2)
x,y,w,h=L["lcd"]
d.rounded_rectangle((x-5,y-5,x+w+5,y+h+5),radius=13,fill=(125,139,150))
d.rounded_rectangle((x,y,x+w,y+h),radius=10,fill=(17,30,42))
for j in range(h-22):
    t=j/(h-22);d.line((x+12,y+11+j,x+w-12,y+11+j),fill=(int(211-70*t),int(245-45*t),int(252-25*t)))
d.text((x+23,y+14),"LIBERTY INFINITY",font=font(22),fill=(32,67,91))
d.line((x+20,y+47,x+w-20,y+47),fill=(65,111,141),width=2)
d.text((x+23,y+h-35),"FACTORY LIBRARY  /  ROMPLER",font=font(16),fill=(32,67,91))
d.text((1046,605),"LIBERTY MUSIC SOFTWARE",font=font(12),fill=(48,65,80))
im.save(A/"retro.png")
# Original silver knob filmstrip with 127 vertical frames.
N=127;S=60; strip=Image.new("RGBA",(S,S*N))
for i in range(N):
    k=Image.new("RGBA",(S,S));q=ImageDraw.Draw(k)
    q.ellipse((4,5,56,57),fill=(77,90,101,85))
    for j in range(11):
        a=math.radians(135+j*27);cx=30+25*math.cos(a);cy=30+25*math.sin(a)
        q.ellipse((cx-1.4,cy-1.4,cx+1.4,cy+1.4),fill=(46,125,173,255) if j<=i*10/(N-1) else (91,103,114,255))
    q.ellipse((9,9,51,51),fill=(101,115,126,255))
    for y in range(11,49):
        dx=math.sqrt(max(0,19**2-(y-30)**2));v=int(244-76*(y-11)/38)
        q.line((30-dx,y,30+dx,y),fill=(v,min(255,v+3),min(255,v+6),255))
    a=math.radians(135+270*i/(N-1))
    q.line((30+10*math.cos(a),30+10*math.sin(a),30+19*math.cos(a),30+19*math.sin(a)),fill=(35,49,61,255),width=3)
    strip.paste(k,(0,S*i))
strip.save(A/"silver.png")
