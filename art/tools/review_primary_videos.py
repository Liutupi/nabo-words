"""Read-only review of delivered videos; diagnostics are written only under art/."""
from pathlib import Path
from fractions import Fraction
import io,json,hashlib,subprocess,sys
import numpy as np
from PIL import Image,ImageDraw,ImageFont,ImageOps
ROOT=Path(__file__).resolve().parents[2]
ART=ROOT/'art';OUT=ART/'preview/video_review';OUT.mkdir(parents=True,exist_ok=True)
fpath=subprocess.check_output(['fc-match','-f','%{file}','PingFang SC'],text=True).strip()
font=ImageFont.truetype(fpath,18)
results=[]
paths=sorted(p for p in (ART/'video').glob('*/*.mp4') if not p.name.startswith('._'))
if len(sys.argv)>1:
    paths=[p for p in paths if p.name in sys.argv[1:]]
else:
    paths=[p for p in paths if p.parent.name=='primary']
for p in paths:
    probe=json.loads(subprocess.check_output(['ffprobe','-v','error','-show_format','-show_streams','-of','json',str(p)],text=True))
    video=next(s for s in probe['streams'] if s['codec_type']=='video' and s['codec_name']=='h264')
    raw=subprocess.check_output(['ffmpeg','-v','error','-i',str(p),'-map','0:v:0','-an','-vf','scale=240:320','-f','rawvideo','-pix_fmt','rgb24','pipe:1'])
    frames=np.frombuffer(raw,dtype=np.uint8).reshape((-1,320,240,3))
    count=len(frames); fps=float(Fraction(video['avg_frame_rate']))
    stage=p.parent.name
    # Broad chroma test identifies the desk edge and subject without modifying media.
    a=frames.astype(np.int16);r,g,b=a[:,:,:,0],a[:,:,:,1],a[:,:,:,2]
    green=(g>75)&(g>r*1.3)&(g>b*1.25)
    edges=[];boxes=[];desk_fallback_frames=[]
    side_columns=np.r_[4:16,224:236]
    for i in range(count):
        # Raised hands can reach a side column above the desk. Match the broad
        # wood-colored band on both sides rather than any non-green pixel.
        desk_color=np.median(a[i,-6:],axis=(0,1))
        side_pixels=a[i,160:][:,side_columns]
        wood=np.max(np.abs(side_pixels-desk_color),axis=2)<24
        rows=wood.mean(axis=1)>.9
        runs=np.flatnonzero(np.convolve(rows.astype(int),np.ones(8,dtype=int),'valid')==8)
        if len(runs):
            edge=int(runs[0]+160)
        else:
            # Retain diagnostics for rejected/deformed sources too. Flag every
            # fallback frame instead of treating the fallback as a clean result.
            desk_fallback_frames.append(i)
            left=np.where(~green[i,:,4])[0];left=left[left>160]
            right=np.where(~green[i,:,-5])[0];right=right[right>160]
            edge=int(np.median([left[0],right[0]])) if len(left) and len(right) else 265
        edges.append(edge)
        obj=~green[i,:edge-2]
        ys,xs=np.where(obj)
        boxes.append([int(xs.min()),int(ys.min()),int(xs.max()),int(ys.max())])
    diff=np.abs(a[-1]-a[0]);motion=np.mean(np.abs(a[1:]-a[:-1]),axis=(1,2,3))
    idxs=np.linspace(0,count-1,12).round().astype(int)
    sheet=Image.new('RGB',(1440,710),'#FFF8EC');d=ImageDraw.Draw(sheet)
    d.text((12,5),p.name+f'  |  sequential frames / {fps:g}fps',font=font,fill='#121829')
    for n,i in enumerate(idxs):
        x=(n%6)*240;y=38+(n//6)*336
        sheet.paste(Image.fromarray(frames[i]),(x,y))
        d.text((x+6,y+317),f'{i/fps:.2f}s  f{i}',font=font,fill='#121829')
    sheet.save(OUT/(p.stem+'_contact.png'))
    # Source/start and endpoint proof, larger than native game size to review details.
    compare=Image.new('RGB',(1080,520),'#FFF8EC')
    src=Image.open(ART/f'start/{stage}.png').convert('RGB')
    src=ImageOps.fit(src,(360,480),method=Image.Resampling.LANCZOS)
    compare.paste(src,(0,30))
    compare.paste(Image.fromarray(frames[0]).resize((360,480)),(360,30))
    compare.paste(Image.fromarray(frames[-1]).resize((360,480)),(720,30))
    dc=ImageDraw.Draw(compare)
    for x,t in [(10,f'Approved {stage} / center crop'),(370,'First frame'),(730,'Last frame')]:dc.text((x,2),t,font=font,fill='#121829')
    compare.save(OUT/(p.stem+'_endpoints.png'))
    results.append({'file':str(p.relative_to(ROOT)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),
      'width':video['width'],'height':video['height'],'frames':count,'fps':fps,'stage':stage,
      'duration_s':float(probe['format']['duration']),
      'audio_present':any(s['codec_type']=='audio' for s in probe['streams']),
      'desk_y_at_320h_range':[min(edges),max(edges)],
      'desk_detection':'eight wood-colored rows across both 12-pixel side bands; raised hands excluded',
      'desk_detection_fallback_frames':desk_fallback_frames,
      'character_bbox_all_frames':[min(b[0] for b in boxes),min(b[1] for b in boxes),max(b[2] for b in boxes),max(b[3] for b in boxes)],
      'character_bbox_first':boxes[0],'character_bbox_last':boxes[-1],
      'first_last_mean_rgb_abs_difference':round(float(diff.mean()),3),
      'successive_frame_rgb_abs_difference_range':[round(float(motion.min()),3),round(float(motion.max()),3)],
      'contact_sheet':str((OUT/(p.stem+'_contact.png')).relative_to(ROOT)),
      'endpoint_sheet':str((OUT/(p.stem+'_endpoints.png')).relative_to(ROOT))})
previous=json.loads((OUT/'metadata.json').read_text()) if (OUT/'metadata.json').exists() else []
updated={r['file']:r for r in previous}
updated.update({r['file']:r for r in results})
(OUT/'metadata.json').write_text(json.dumps(list(updated.values()),ensure_ascii=False,indent=2)+'\n')
for r in results:print(json.dumps(r,ensure_ascii=False))
