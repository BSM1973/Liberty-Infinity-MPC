import json,tempfile,unittest
from pathlib import Path
import numpy as np
from scipy.io import wavfile
from import_multisamples import *

class ImportTest(unittest.TestCase):
    def test_import(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d); base=p/"base.pcm";out=p/"personal"/"Factory.pcm"
            with base.open("wb") as f:
                f.write(HEADER);f.truncate(32+100*20*ZONE_BYTES)
            t=np.arange(RATE*2)/RATE
            wavfile.write(p/"a.wav",RATE,(.5*np.sin(2*np.pi*261.6256*t)*32767).astype(np.int16))
            sr,a=read_wav(p/"a.wav")
            self.assertEqual(detect_root(a,sr),60)
            conf={"sources":[{"source":0,"normalize":True,"zones":[{"wav":"a.wav","root":60}]}],
                  "patches":[{"patch":0,"layers":[{"source":0,"gain_db":-3,"pan":.25,"cents":-5}]}]}
            m=p/"bank.json";m.write_text(json.dumps(conf))
            self.assertEqual(len(build(base,m,out)),20)
            self.assertEqual(out.stat().st_size,base.stat().st_size)
            self.assertIn("L 0 0 0",Path(str(out)+".map").read_text())
            with out.open("rb") as f:
                f.seek(32+8*ZONE_BYTES);self.assertTrue(any(f.read(ZONE_BYTES)))
            with base.open("rb") as f:
                f.seek(32);self.assertFalse(any(f.read(ZONE_BYTES)))
            with self.assertRaises(ValueError):build(base,m,out)
            raw,start,end,xf=zone_data(a,RATE,60,60,[10000,80000],1)
            self.assertEqual((start,end),(10000,80000));self.assertGreater(xf,0)
if __name__=="__main__":unittest.main()
