from pathlib import Path
p=Path("sdk/tools/build.py")
s=p.read_text()
old='flags = ["-Wl,--gc-sections"]'
new='flags = ["-Wl,--gc-sections", "-pthread"]'
assert old in s or new in s
p.write_text(s.replace(old,new))
