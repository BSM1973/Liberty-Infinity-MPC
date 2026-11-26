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
    assert (root/'Content/Expansions/01/SHA256SUMS.json').is_file()
    bad=root/'Content/Expansions/01/Cover.png'
    with bad.open('ab') as f:f.write(b'tampered')
    from expansion_pack import verify_checksums
    try:verify_checksums(bad.parent)
    except ValueError:pass
    else:raise AssertionError('Tampered cover accepted')
print("PASS real cover, installation, Factory unchanged, duplicate rejected")
