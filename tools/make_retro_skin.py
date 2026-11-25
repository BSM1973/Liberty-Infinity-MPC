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
assert set(by)=={c["i"] for c in L["controls"]}, "Layout and DSP parameter mismatch"
out=P/"skins";out.mkdir(exist_ok=True)
for f in ("retro.png","gold_knob_36.png","gold_knob_40.png","power_36_on.png","power_36_off.png","power_40_on.png","power_40_off.png","selector_60.png"):shutil.copy2(P/"assets"/f,out/f)
o=g.obj
def rect(x,y,w,h):return f"{x} {y} {w} {h}"
def label(n,t,xy,colour="fff9f9f9",size=16):
    return g.entry(n,"Label",g.label_data("Text",colour,size,"horizontallyCentred verticallyCentred",t),xy,"No","Show","Always")
defs=[]
children=[]
for c in L["controls"]:
    i,x,y,w,h=[c[k] for k in ("i","x","y","w","h")]
    p=by[i]; name="retro"+str(i)
    if i==0:
        parts=[label("hint","FACTORY - DOUBLE TAP FOR SOUND LIST",rect(5,3,w-10,30),size=18),
               g.entry("current","Label",g.label_data("Value","fff9f9f9",35,"horizontallyCentred verticallyCentred"),rect(10,40,w-20,95),"No","Show","Always"),
               label("categories","100 SOUNDS / 12 CATEGORIES",rect(10,143,w-20,30),size=18),
               label("native","USE MPC PRESET SELECTOR TO LOAD / SAVE PROGRAMS",rect(10,181,w-20,25),size=14)]
        actions=g.control_actions("menu overlay")
    else:
        face=c["face"];fw,fh,fy=(face[k] for k in ("w","h","y"));fx=(w-fw)//2
        common=[label("name",c["name"],rect(0,face["labelY"],w,14),size=13),
                g.entry("value","Label",g.label_data("Value","fff9f9f9",11,"horizontallyCentred verticallyCentred"),rect(0,face["valueY"],w,11),"No","Show","Always")]
        if c["kind"]=="toggle":
            stem=face["asset"]
            parts=[g.entry("switch","Button",g.button_data(stem+"_on.png",stem+"_off.png",1,1),rect(fx,fy,fw,fh),"No","Show","Always")]+common
            actions=g.control_actions("menu overlay",double_click=False)
        elif c["kind"]=="selector":
            parts=[g.entry("face","Image",o(("version",2),("imageType","Regular"),("colour","0"),("image",face["asset"])),rect(fx,fy,fw,fh),"No","Show","Always"),
                   g.entry("selection","Label",g.label_data("Value","fff9f9f9",10,"horizontallyCentred verticallyCentred"),rect(fx+2,fy+6,fw-12,20),"No","Show","Always"),
                   label("name",c["name"],rect(0,face["labelY"],w,14),size=13)]
            actions=g.control_actions("menu overlay")
        else:
            parts=[g.entry("knob","Knob",g.knob_filmstrip_data(face["asset"],127,"Vertical"),rect(fx,fy,fw,fh),"No","Show","Always")]+common
            actions=g.control_actions("knob overlay")
    defs.append(o(("key",name),("value",g.component_definition(actions,parts,"0"))))
    children.append(g.entry(name,name,o(("version",1),("handleName","Data")),rect(x,y,w,h),"Yes","Hide","Always",remap=[("Data",p.handle)]))
for n,typ,data in [("knob overlay","KnobOverlay",g.knob_overlay_data()),("menu overlay","MenuOverlay",g.menu_overlay_data())]:
    defs.append(o(("key",n),("value",g.overlay_component(typ,data,"ff101014"))))
# Same facade on each tab; the tab selects the active hardware Q-Link bank.
groups=[("MAIN",[8,5,11,13,9,10,12,14,15,16,1,3,6,17,2,4]),
        ("FX",[18,21,19,20,23,26,27,29,24,25,28,30,22,0,7]),
        ("REVERB",[31,34,35,37,32,33,36,38,0,7])]
tabs=[];maps=[]
for ti,(name,indices) in enumerate(groups):
    tabs.append(o(("version",1),("tabName",name),("fnKeyIndex",ti),("fnKeySubIndex",0),
      ("componentDefinition",g.component_definition([],children,"ff101014",bg_image="retro.png")),("qlinkBoundsData",[])))
    maps.append(o(("Tab",ti+1),("SubTab",1),("Bank Direction","Column"),
      ("Q-Links",g.Obj(("Q-Link %d"%(j+1),indices[j] if j<len(indices) else -1) for j in range(16)))))
repl=[o(("key",p.handle),("value",list(p.choices))) for p in params if p.is_discrete]
tui=o(("pageData",o(("version",1),("componentDefinitions",o(("version",2),("importFiles",[]),("localComponentDefinitions",defs))),
  ("replacementStrings",repl),("info",o(("version",1),("type","CompleteDescription"))),("tabs",tabs))))
ql=o(("version",4),("info",o(("version",1),("type","CompleteDescription"))),
  ("Screen Mode Q-Links",o(("version",4),("map",maps))),
  ("Program Mode Q-Links",g.Obj(("Q-Link %d"%(i+1),v) for i,v in enumerate(groups[0][1]))))
for n,v in [("TUI.json",tui),("Q-Links.json",ql)]: (out/n).write_text(g.dump_json(v)+"\n",encoding="utf8")
print("Black Gold skin: 39 bound parameters, three Q-Link banks")
