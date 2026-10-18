import glob, os, sys, numpy as np
from PIL import Image
rows=[]
import os as _os
_root=_os.path.join(_os.path.dirname(_os.path.abspath(__file__)),'..','footage','frames')
for d in sorted(glob.glob(_os.path.join(_root,'*')+_os.sep)):
    fs=sorted(glob.glob(d+'*.jpg'))
    if len(fs)<10: continue
    prev=None; dup=0; diffs=[]
    for f in fs:
        a=np.asarray(Image.open(f).convert('L').resize((160,90)), dtype=np.float32)
        if prev is not None:
            dv=float(np.abs(a-prev).mean()); diffs.append(dv)
            if dv<0.35: dup+=1
        prev=a
    n=len(diffs)
    rows.append((os.path.basename(d.rstrip('/')), n, 100.0*dup/max(n,1), round(float(np.mean(diffs)),2), round(float(np.percentile(diffs,90)),2)))
rows.sort(key=lambda r:-r[2])
print(f'{"clip":22s} {"frames":>6s} {"重复帧%":>7s} {"平均差":>6s} {"P90差":>6s}')
for r in rows: print(f'{r[0]:22s} {r[1]:6d} {r[2]:7.1f} {r[3]:6.2f} {r[4]:6.2f}')
