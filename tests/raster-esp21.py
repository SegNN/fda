from pathlib import Path
import json,numpy as np
from PIL import Image
import sys
root=Path(__file__).resolve().parent.parent
width=int(sys.argv[1]) if len(sys.argv)>1 else 1920
kind=f'esp21-{width}'+('-hidden' if len(sys.argv)>2 else '')
m=json.loads((root/'build'/f'{kind}-mesh.json').read_text());fw,fh=m['font'];W,H=m['screen']
textures={1:np.frombuffer((root/'build'/f'{kind}-font.rgba').read_bytes(),dtype=np.uint8).reshape(fh,fw,4)}
for i,p in m['textures'].items():textures[int(i)]=np.asarray(Image.open(p).convert('RGBA'))
scale=2;canvas=np.zeros((H*scale,W*scale,4),dtype=np.float32)
for l in m['lists']:
 vertices=np.array(l['v'],dtype=float);colors=np.array([[int(v[4])>>shift&255 for shift in (0,8,16,24)] for v in l['v']],dtype=float)/255
 for off,n,base,tex,cx,cy,cxx,cyy in l['cmd']:
  texture=textures.get(tex,textures[1]);th,tw=texture.shape[:2]
  for at in range(off,off+n,3):
   ids=np.array(l['idx'][at:at+3])+base;p=vertices[ids,:2]*scale;uv=vertices[ids,2:4];c=colors[ids]
   lo=np.maximum(np.floor(p.min(axis=0)).astype(int),[max(0,int(cx*scale)),max(0,int(cy*scale))]);hi=np.minimum(np.ceil(p.max(axis=0)).astype(int),[min(W*scale,int(cxx*scale)),min(H*scale,int(cyy*scale))]);x0,y0=lo;x1,y1=hi
   if x1<=x0 or y1<=y0:continue
   xx,yy=np.meshgrid(np.arange(x0,x1)+.5,np.arange(y0,y1)+.5)
   den=(p[1,1]-p[2,1])*(p[0,0]-p[2,0])+(p[2,0]-p[1,0])*(p[0,1]-p[2,1])
   if abs(den)<1e-7:continue
   a=((p[1,1]-p[2,1])*(xx-p[2,0])+(p[2,0]-p[1,0])*(yy-p[2,1]))/den
   b=((p[2,1]-p[0,1])*(xx-p[2,0])+(p[0,0]-p[2,0])*(yy-p[2,1]))/den;d=1-a-b
   mask=(a>=0)&(b>=0)&(d>=0)
   if not mask.any():continue
   weights=np.stack([a[mask],b[mask],d[mask]],axis=1);samples=weights@uv;col=weights@c
   tx=np.clip((samples[:,0]*tw).astype(int),0,tw-1);ty=np.clip((samples[:,1]*th).astype(int),0,th-1)
   src=texture[ty,tx].astype(float)/255*col;region=canvas[y0:y1,x0:x1];dest=region[mask];alpha=src[:,3:4];dest[:,:3]=src[:,:3]*alpha+dest[:,:3]*(1-alpha);dest[:,3:4]=alpha+dest[:,3:4]*(1-alpha);region[mask]=dest
im=Image.fromarray(np.clip(canvas*255,0,255).astype('uint8'),'RGBA').convert('RGB').resize((W,H),Image.Resampling.LANCZOS);im.save(root/'build'/f'{kind}-preview.png');print('Rendered actual ESP21 C++ draw mesh at',W,H)
