"""Static geometry/UTF-8 checks for the original panel and Studio controls."""
from pathlib import Path
import json,re,itertools,unicodedata
root=Path(__file__).resolve().parents[1]
def verify(rects,w,h):
    for name,x,y,a,b in rects:
        assert x>=0 and y>=0 and a>0 and b>0 and x+a<=w and y+b<=h,(name,x,y,a,b,w,h)
    for (n,x,y,w,h),(m,X,Y,W,H) in itertools.combinations(rects,2):
        assert min(x+w,X+W)<=max(x,X) or min(y+h,Y+H)<=max(y,Y),(n,m)
for path in [root/"macos/Main.mm",root/"macos/MultisampleStudio.h",root/"macos/NativeBankExport.h",root/"macos/PresetLibrary.h",root/"plugins/Infinity/layout.json"]:
    s=path.read_text(encoding="utf-8",errors="strict")
    assert not any(c in s for c in ("\ufffd","\u00c3\u00a9","\u00c3\u00a8","\u00e2\u20ac")),path
    assert not any(unicodedata.category(c)=="Cc" and c not in "\n\t\r" for c in s),path
layout=json.loads((root/"plugins/Infinity/layout.json").read_text())
# The Sound control is intentionally replaced by the standalone browser.
assert len(layout["pages"])==6
assert sorted(c["i"] for c in layout["controls"])==list(range(39))
for page in layout["pages"]:
    verify([(str(c["i"]),c["x"],c["y"],c["w"],c["h"]) for c in page["controls"]],1280,628)
s=(root/"macos/MultisampleStudio.h").read_text()
s=s[s.index("-(instancetype)initWithFrame:(NSRect)frame{"):s.index("-(NSMutableDictionary*)studioLayerRecord{return")]
rects=[]
for typ,title,x,y,w in re.findall(r'\[self (label|field|button|check):@"([^"]+)" x:(\d+) y:(\d+) w:(\d+)',s):
    x,y,w=map(int,(x,y,w))
    rects.append((title,x,y,w,{"label":20,"field":44,"button":30,"check":24}[typ] if title!="Déposez des fichiers WAV sur cette fenêtre." else 58))
for name,x,y,w,h in re.findall(r'(layerMenu|zoneMenu|roleMenu|wave|keys)=\[\[.*?initWithFrame:NSMakeRect\((\d+),(\d+),(\d+),(\d+)\)',s):
    rects.append((name,*map(int,(x,y,w,h))))
assert len(rects)>=30,len(rects)
verify(rects,908,600)
print(f"PASS UTF-8, {len(rects)} Studio rectangles, Factory controls; visual/glyph verification still manual.")
