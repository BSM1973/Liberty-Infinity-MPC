"""Build custom TUI using the pinned SDK's ordered JSON helpers."""
from pathlib import Path
import sys,json,shutil
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"sdk/tools"))
import gen_ui as g
P=ROOT/"plugins/Infinity"
L=json.loads((P/"layout.json").read_text())
params=g.load_params(str(P/"build/params.json"))
by={p.index:p for p in params}
assert set(by)=={c["i"] for c in L["controls"]}|{39,40,41,42,43,44}, "Layout and DSP parameter mismatch"
out=P/"skins";out.mkdir(exist_ok=True)
for f in ("browser-category-on.png","browser-category-off.png","browser-row-on.png","browser-row-off.png","browser-scroll.png","main.png","filter.png","delay.png","reverb.png","amp-modifier.png","libraries.png","factory-1.png","gold_knob_64.png","gold_knob_104.png","power_40_on.png","power_40_off.png","power_76_on.png","power_76_off.png","selector_156.png"):shutil.copy2(P/"assets"/f,out/f)
o=g.obj
def rect(x,y,w,h):return f"{x} {y} {w} {h}"
def label(n,t,xy,colour="fff9f9f9",size=16):
    return g.entry(n,"Label",g.label_data("Text",colour,size,"horizontallyCentred verticallyCentred",t),xy,"No","Show","Always")
defs=[]
children={}
for c in L["controls"]:
    i,x,y,w,h=[c[k] for k in ("i","x","y","w","h")]
    p=by[i]; name="retro"+str(i)
    if i==0:continue
    else:
        face=c["face"];fw,fh,fy=(face[k] for k in ("w","h","y"));fx=(w-fw)//2
        common=[label("name",c["name"],rect(0,face["labelY"],w,face["labelH"]),size=face["labelSize"]),
                g.entry("value","Label",g.label_data("Value","fff9f9f9",face["valueSize"],"horizontallyCentred verticallyCentred"),rect(0,face["valueY"],w,face["valueH"]),"No","Show","Always")]
        if c["kind"]=="toggle":
            stem=face["asset"]
            parts=[g.entry("switch","Button",g.button_data(stem+"_on.png",stem+"_off.png",1,1),rect(fx,fy,fw,fh),"No","Show","Always")]+common
            actions=g.control_actions("menu overlay",double_click=False)
        elif c["kind"]=="selector":
            parts=[g.entry("face","Image",o(("version",2),("imageType","Regular"),("colour","0"),("image",face["asset"])),rect(fx,fy,fw,fh),"No","Show","Always"),
                   g.entry("selection","Label",g.label_data("Value","fff9f9f9",19,"horizontallyCentred verticallyCentred"),rect(fx+4,fy+8,fw-26,fh-16),"No","Show","Always"),
                   label("name",c["name"],rect(0,face["labelY"],w,face["labelH"]),size=face["labelSize"])]
            actions=g.control_actions("menu overlay")
        else:
            parts=[g.entry("knob","Knob",g.knob_filmstrip_data(face["asset"],127,"Vertical"),rect(fx,fy,fw,fh),"No","Show","Always")]+common
            actions=g.control_actions("knob overlay")
    defs.append(o(("key",name),("value",g.component_definition(actions,parts,"0"))))
    children[i]=g.entry(name,name,o(("version",1),("handleName","Data")),rect(x,y,w,h),"Yes","Hide","Always",remap=[("Data",p.handle)])
for n,typ,data in [("knob overlay","KnobOverlay",g.knob_overlay_data()),("menu overlay","MenuOverlay",g.menu_overlay_data())]:
    defs.append(o(("key",n),("value",g.overlay_component(typ,data,"ff101014"))))
# MAIN: native clickable buttons, static actual Factory names, vertical page scroller.
# SDK IndexedEnabling is used for page visibility, as in its visibleWhen generator.
V=json.loads((P/"main-browser.json").read_text(encoding="utf8"))
assert len(V)==len(by[43].choices)
assert [v["label"] for v in V]==list(by[43].choices)
assert len(by[44].choices)==9
mainchildren=[]
def bind(name,typ,parts,xy,param,invalidating=None,actions=None):
    defs.append(o(("key",name),("value",g.component_definition(actions or [],parts,"00000000"))))
    return g.entry(name,typ,o(("version",1),("handleName","Data")),xy,"Yes","Hide","Always",remap=[("Data",param.handle)],invalidating=invalidating)
# One button per category jumps to its first page; no SOUND overlay.
for cat in range(13):
    first=next(i for i,v in enumerate(V) if v["category"]==cat)
    title=V[first]["label"].rsplit("  ",1)[0]
    name="mainCategory"+str(cat);y=164+cat*31
    parts=[g.entry("button","Button",g.button_data("browser-category-on.png","browser-category-off.png",first,len(V)),rect(0,0,246,29),"No","Show","Always"),
           label("name",title,rect(6,1,234,27),size=17)]
    mainchildren.append(bind(name,name,parts,rect(32,y,246,29),by[43]))
# Header uses a real page choice and the selected Sound; both are readouts, no overlay.
mainchildren.append(g.entry("page","Label",g.label_data("Value","ffd5b06b",21,"left"),rect(304,132,410,30),"No","Show","Always",remap=[("Data",by[43].handle)]))
mainchildren.append(g.entry("selected","Label",g.label_data("Value","fff9f9f9",22,"right"),rect(714,132,256,30),"No","Show","Always",remap=[("Data",by[0].handle)]))
for page,v in enumerate(V):
    invalidate=["IndexedEnabling/%d/%d/%s"%(page,len(V),by[43].handle)]
    for row,preset in enumerate(v["rows"]):
        y=170+row*51;name="mainRow%d_%d"%(page,row)
        parts=[g.entry("button","Button",g.button_data("browser-row-on.png","browser-row-off.png",row+1,9),rect(0,0,666,46),"No","Show","Always"),
               label("index","%03d"%(preset["index"]+1),rect(8,4,70,38),colour="ffd5b06b",size=19),
               g.entry("preset","Label",g.label_data("Text","fff9f9f9",23,"left",preset["name"]),rect(90,4,560,38),"No","Show","Always")]
        mainchildren.append(bind(name,name,parts,rect(304,y,666,46),by[44],invalidate))
scroll=[g.entry("scroll","Knob",g.knob_filmstrip_data("browser-scroll.png",len(V),"Vertical"),rect(0,0,38,403),"No","Show","Always")]
mainchildren.append(bind("mainScroll","mainScroll",scroll,rect(980,170,38,403),by[43],actions=[g.action("Mouse Down","Q-Link","Data","")]))
mainchildren.append(children[7])

# Six actual pages, each with only its own parameter controls.
groups=[(p["name"],p["ids"] if p["name"]!="MAIN" else [43,44,7]) for p in L["pages"][:-1]]
tabs=[];maps=[]
for ti,page in enumerate(L["pages"][:-1]):
    indices=page["ids"] if ti else [43,44,7]
    tabs.append(o(("version",1),("tabName",page["name"]),("fnKeyIndex",ti),("fnKeySubIndex",0),
      ("componentDefinition",g.component_definition([], mainchildren if ti==0 else [children[c["i"]] for c in page["controls"]],"ff101014",bg_image=page["asset"]+".png")),("qlinkBoundsData",[])))
    maps.append(o(("Tab",ti+1),("SubTab",1),("Bank Direction","Column"),
      ("Q-Links",g.Obj(("Q-Link %d"%(j+1),indices[j] if j<len(indices) else -1) for j in range(16)))))
# Native Library Manager: controls backed by actual DSP parameter handles.
libchildren=[]
for index,title,x in ((39,"COLLECTION",32),(40,"CATÉGORIE",340),(41,"PRESET",648),(42,"PRÉÉCOUTE",956)):
    name=f"library{index}";param=by[index];w=292;h=110
    parts=[label("name",title,rect(5,8,w-10,26),size=20),
           g.entry("value","Label",g.label_data("Value","ffd5b06b",23,"horizontallyCentred verticallyCentred"),rect(8,44,w-16,34),"No","Show","Always")]
    if index==42:
        parts.append(g.entry("switch","Button",g.button_data("power_40_on.png","power_40_off.png",1,1),rect(126,78,40,36),"No","Show","Always"))
        # Button is contained in the 116-pixel control and acts on the preview bool.
        h=116
    overlay="menu overlay" if index in (39,40) else "knob overlay"
    defs.append(o(("key",name),("value",g.component_definition(g.control_actions(overlay,index!=42),parts,"00000000"))))
    libchildren.append(g.entry(name,name,o(("version",1),("handleName","Data")),rect(x,343,w,h),"Yes","Hide","Always",remap=[("Data",param.handle)]))
# Actual selected Sound handle; the native Sound list remains available.
name="librarySound"
parts=[label("name","SON ACTUEL",rect(0,0,1216,24),size=16),g.entry("value","Label",g.label_data("Value","fff9f9f9",28,"horizontallyCentred verticallyCentred"),rect(0,28,1216,46),"No","Show","Always")]
defs.append(o(("key",name),("value",g.component_definition(g.control_actions("menu overlay"),parts,"00000000"))))
libchildren.append(g.entry(name,name,o(("version",1),("handleName","Data")),rect(32,470,1216,76),"Yes","Hide","Always",remap=[("Data",by[0].handle)]))
tabs.append(o(("version",1),("tabName","LIBRARIES"),("fnKeyIndex",5),("fnKeySubIndex",0),("componentDefinition",g.component_definition([],libchildren,"ff101014",bg_image="libraries.png")),("qlinkBoundsData",[])))
maps.append(o(("Tab",6),("SubTab",1),("Bank Direction","Column"),("Q-Links",g.Obj(("Q-Link %d"%(j+1),[39,40,41,42,0][j] if j<5 else -1) for j in range(16)))))
repl=[o(("key",p.handle),("value",list(p.choices))) for p in params if p.is_discrete]
tui=o(("pageData",o(("version",1),("componentDefinitions",o(("version",2),("importFiles",[]),("localComponentDefinitions",defs))),
  ("replacementStrings",repl),("info",o(("version",1),("type","CompleteDescription"))),("tabs",tabs))))
ql=o(("version",4),("info",o(("version",1),("type","CompleteDescription"))),
  ("Screen Mode Q-Links",o(("version",4),("map",maps))),
  ("Program Mode Q-Links",g.Obj(("Q-Link %d"%(i+1),groups[0][1][i] if i<len(groups[0][1]) else -1) for i in range(16))))
for n,v in [("TUI.json",tui),("Q-Links.json",ql)]: (out/n).write_text(g.dump_json(v)+"\n",encoding="utf8")
print("Black Gold skin: 45 bound parameters, six Q-Link banks")
