from pathlib import Path
import tempfile,json
from PIL import Image
from mpc_expansion_manager import add,refresh
with tempfile.TemporaryDirectory() as t:
    root=Path(t)/"plugin";root.mkdir();(root/"Content").mkdir();(root/"Plugin Skins").mkdir()
    (root/"plugin.json").write_text(json.dumps(dict(uid="LInf",name="Liberty Infinity")))
    (root/"Content/Factory.pcm").write_bytes(b"unchanged")
    skin=root/"Plugin Skins/libraries.png";Image.new("RGB",(1280,628)).save(skin)
    pack=Path("cinematic-horizons");add(pack,root,1)
    assert (root/"Content/Factory.pcm").read_bytes()==b"unchanged"
    with Image.open(skin) as im,Image.open(pack/"Cover.png") as cover:
        assert im.crop((319,106,469,256)).tobytes()==cover.convert("RGB").resize((150,150),Image.Resampling.LANCZOS).tobytes()
    try:add(pack,root,2)
    except ValueError:pass
    else:raise AssertionError("Duplicate accepted")
    refresh(root)
print("PASS real cover, installation, Factory unchanged, duplicate rejected")
