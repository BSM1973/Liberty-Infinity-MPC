"""Prepare a separate QA distribution; never label unsigned DEMO as commercial."""
from pathlib import Path
import zipfile
from expansion_pack import verify_checksums
pack=Path("cinematic-horizons");verify_checksums(pack)
out=Path("cinematic-horizons-master-QA.zip")
with zipfile.ZipFile(out,"x",zipfile.ZIP_DEFLATED,compresslevel=3) as z:
    for f in sorted(pack.iterdir()):
        if f.is_file():z.write(f,"Cinematic Horizons/"+f.name)
    for name in ("expansion_pack.py","mpc_expansion_manager.py"):z.write(Path("tools")/name,"Installer/"+name)
    z.write("docs/cinematic-master-edition.md","START-HERE-FR.md")
    z.write("docs/cinematic-product-draft.md","PRODUCT-DRAFT-FR.md")
with zipfile.ZipFile(out) as z:
    if z.testzip() is not None:raise ValueError("Corrupt ZIP")
    if "Cinematic Horizons/Library.pcm" not in z.namelist():raise ValueError("Missing PCM")
print("PASS standalone QA package, installer, documentation and ZIP CRC")
