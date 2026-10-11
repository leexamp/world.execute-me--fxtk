import subprocess, sys, numpy as np
from PIL import Image
import io

def frames(mp4, t0, t1, step=0.1, band=(216, 246)):
    ys = list(np.arange(t0, t1, step))
    out = []
    for t in ys:
        p = subprocess.run(["ffmpeg","-hide_banner","-loglevel","error","-ss",f"{t}","-i",mp4,
                            "-frames:v","1","-f","image2pipe","-vcodec","png","-"],
                           capture_output=True).stdout
        a = np.array(Image.open(io.BytesIO(p)).convert("RGB")).astype(int)
        row = a[band[0]:band[1]]
        white = (row[:,:,0]>240)&(row[:,:,1]>240)&(row[:,:,2]>240)
        cols = white.sum(axis=0)
        # 统计连续"亮块"数量
        on = cols > 3
        blobs = 0; prev = False
        for v in on:
            if v and not prev: blobs += 1
            prev = v
        out.append((round(float(t),2), blobs, int(on.sum())))
    return out

for f in sys.argv[1:]:
    print("==", f)
    for t, b, w in frames(f, 0.4, 6.0, 0.2):
        print(f"   t={t:5.1f}  亮块={b}  亮列={w}")
