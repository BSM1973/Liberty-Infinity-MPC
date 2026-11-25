from pathlib import Path
import tempfile,json
import numpy as np
from scipy.io import wavfile
from expansion_pack import build,install,list_slots
with tempfile.TemporaryDirectory() as t:
    root=Path(t);wavfile.write(root/"demo.wav",44100,(np.sin(np.arange(44100)*2*np.pi*261.6256/44100)*16000).astype(np.int16))
    manifest={"id":"12345678-1234-1234-1234-123456789abc","name":"Épreuve PCM","entitlement":"DEMO","presets":[{"index":0,"name":"Test","layers":[{"zones":[{"wav":"demo.wav","root":60,"source":"synthetic CI test","rights":"original fixture"}]}]}]}
    (root/"manifest.json").write_text(json.dumps(manifest),encoding="utf8")
    build(root/"manifest.json",root/"pack")
    assert (root/"pack/Library.pcm").stat().st_size==8+44100*3
    assert len([x for x in (root/"pack/Library.map").read_text().splitlines() if x.startswith("X ")])==20
    content=root/"Content";content.mkdir();(content/"Factory.pcm").write_bytes(b"unchanged")
    install(root/"pack",content,1)
    assert (content/"Factory.pcm").read_bytes()==b"unchanged"
    try:install(root/"pack",content,1)
    except FileExistsError:pass
    else:raise AssertionError("overwrite accepted")
    manifest["entitlement"]="COMMERCIAL";(root/"manifest.json").write_text(json.dumps(manifest))
    try:build(root/"manifest.json",root/"blocked")
    except ValueError:pass
    else:raise AssertionError("unsigned commercial pack accepted")
    list_slots(content)
print("PASS expansion authoring, compact PCM, cover, install without overwrite and commercial gate")
