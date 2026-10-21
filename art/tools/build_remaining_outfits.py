"""Normalize the four remaining outfits; graduation retains generated diploma grip."""
from pathlib import Path
import json,subprocess
import numpy as np
from PIL import Image,ImageDraw,ImageFont,ImageFilter
ART=Path(__file__).resolve().parents[1]
BASE=Image.open(ART/'start/primary.png').convert('RGB')
BC=Image.open(ART/'design/source/primary_a_cutout.png').convert('RGBA')
ba=np.asarray(BASE);bc=np.asarray(BC);yy=np.indices(ba.shape[:2])[0]
HEAD_END=831
headcolors=np.unique(bc[:,:,:3][(bc[:,:,3]>0)&(yy<HEAD_END)],axis=0)
headpalette=['#'+bytes(c).hex() for c in headcolors]
fontpath=subprocess.check_output(['fc-match','-f','%{file}','PingFang SC'],text=True).strip()
def font(n):return ImageFont.truetype(fontpath,n)
def text(im,xy,s,n=18,color='#121829'):ImageDraw.Draw(im).text(xy,s,font=font(n),fill=color)
def save(im,p):im.save(ART/p,optimize=True);return im
NEAR=Image.Resampling.NEAREST;LANC=Image.Resampling.LANCZOS

def flat(im,cols):
 p=Image.new('P',(1,1));rgb=[tuple(bytes.fromhex(c[1:])) for c in cols]
 p.putpalette([v for c in rgb for v in c]+list(rgb[-1])*(256-len(rgb)))
 out=im.convert('RGB').quantize(palette=p,dither=Image.Dither.NONE).convert('RGBA')
 out.putalpha(im.getchannel('A').point(lambda v:255 if v>=128 else 0));return out

# Restore original fingers and nearby outlines; preserve every pixel from the desk down.
skincolors=['#FFCEA9','#EFAC89','#CF876A','#F5AE9D','#F6B997']
skin=np.zeros(ba.shape[:2],bool)
for c in skincolors:skin|=np.all(ba==np.array(tuple(bytes.fromhex(c[1:]))),axis=2)
handmask=Image.fromarray(np.uint8(skin&(yy>=1034))*255).filter(ImageFilter.MaxFilter(5))
hm=np.asarray(handmask)>0
valid=np.zeros_like(hm)
for c in headcolors:valid|=np.all(ba==c,axis=2)
hm&=valid&(bc[:,:,3]>0)&(yy>=1034)


import hashlib
WORK=ART/'design/round5'
SPECS=[('cet4','四级 / 灰卫衣＋耳机','#D9D4C7','#715A48',['#4B505A','#777E88','#A1A6B1','#C4CAD1']),('cet6','六级 / 白衬衫＋背包','#E8DDF0','#7E5A47',['#142338','#274363','#4D657E']),('adv','进阶 / 驼色风衣＋相机','#24324F','#5B483B',['#252D3C','#4C5665','#9E7149','#CD9E68','#E3BC85']),('grad','毕业 / 黑金学士服＋证书','#24324F','#5B483B',['#24242B','#45414A','#B57E2A','#FFD56C','#B63D38'])]
checks={};smalls={};cuts={}
for stage,title,wall,desk,extras in SPECS:
 raw=Image.open(WORK/f'source/outfit_{stage}.png').convert('RGB');ar=np.asarray(raw).astype(np.int16);r,g,b=ar[:,:,0],ar[:,:,1],ar[:,:,2]
 green=(g>80)&(g>r*1.3)&(g>b*1.25)
 edge=int(np.flatnonzero((~green[:,0])&(np.arange(raw.height)>raw.height*.55))[0])
 y,x=np.where(~green[:edge-3]);top=int(y.min());left=int(x.min());right=int(x.max())+1
 alpha=~green
 if stage=='grad':
  # Below the alignment line keep the generated hands/ribbon, never a wood rectangle.
  details=((r>225)&(g>145)&(b>110))|((r>130)&(r>g*1.5)&(b<105))|(np.maximum(np.maximum(r,g),b)<105)
  alpha[edge:]=details[edge:]
 rgba=raw.convert('RGBA');rgba.putalpha(Image.fromarray(np.uint8(alpha)*255))
 obj=rgba.crop((left,top,right,edge+40));k=704/(edge-top);obj=obj.resize((round(obj.width*k),round(obj.height*k)),LANC)
 palette=headpalette+extras
 assert len(palette)<=32,(stage,len(palette))
 obj=flat(obj,palette);cut=Image.new('RGBA',(1024,1280));cut.alpha_composite(obj,(512-obj.width//2,366));ca=np.array(cut)
 ca[:HEAD_END]=bc[:HEAD_END]
 if stage!='grad':ca[1070:]=bc[1070:];ca[hm]=bc[hm]
 cut=Image.fromarray(ca);canvas=Image.new('RGBA',(1024,1280),'#00B140');ImageDraw.Draw(canvas).rectangle((0,1070,1023,1279),fill='#C88A52');canvas.alpha_composite(cut);final=canvas.convert('RGB')
 save(final,f'design/{stage}_a.png');save(cut,f'design/source/{stage}_a_cutout.png');save(final,f'start/{stage}.png');cuts[stage]=cut
 assert np.array_equal(np.array(final)[:HEAD_END],ba[:HEAD_END])
 if stage!='grad':assert np.array_equal(np.array(final)[1070:],ba[1070:])
 cols=len(np.unique(ca[:,:,:3][ca[:,:,3]>0],axis=0));assert cols<=32
 bbox=cut.getchannel('A').getbbox();sprite=cut.crop((bbox[0],366,bbox[2],bbox[3]));small=flat(sprite.resize((round(sprite.width*110/704),round(sprite.height*110/704)),LANC),palette);smalls[stage]=small
 hbox=BC.crop((0,366,1024,824));hbox=hbox.crop(hbox.getchannel('A').getbbox());head=flat(hbox.resize((round(hbox.width*48/hbox.height),48),LANC),headpalette)
 proof=Image.new('RGBA',(240,180),'#FFF8EC');d=ImageDraw.Draw(proof);d.rectangle((8,28,137,153),fill=wall);d.rectangle((8,138,137,153),fill=desk);proof.alpha_composite(small,(73-small.width//2,28));proof.alpha_composite(head,(186-head.width//2,70));text(proof,(9,7),stage.upper()+' / 110 px',10);text(proof,(153,45),'HEAD / 48 px',10);text(proof,(9,159),'原尺寸 → 最近邻 ×4',11);save(proof.convert('RGB').resize((960,720),NEAR),f'design/{stage}_a_check.png')
 checks[stage]={'file':f'start/{stage}.png','candidate':f'design/{stage}_a.png','canvas_px':[1024,1280],'cap_top':[512,366],'desk_anchor':[512,1070],'head_to_desk_px':704,'head_max_pixel_difference':0,'opaque_rgb_colors':cols,'owner_approved':False,'sha256':hashlib.sha256((ART/f'start/{stage}.png').read_bytes()).hexdigest(),'hand_policy':'generated diploma grip' if stage=='grad' else 'original hand pixels preserved','source':f'design/round5/source/outfit_{stage}.png'}
board=Image.new('RGB',(1600,980),'#FFF8EC');text(board,(24,16),'NABO / 四级 → 六级 → 进阶 → 毕业',30)
for i,(stage,title,wall,desk,_) in enumerate(SPECS):
 x=20+i*395;text(board,(x,75),title,21);board.paste(Image.open(ART/f'start/{stage}.png').resize((376,470),LANC),(x,115));ob=smalls[stage].resize((smalls[stage].width*3,smalls[stage].height*3),NEAR);d=ImageDraw.Draw(board);d.rectangle((x+10,615,x+365,958),fill=wall);d.rectangle((x+10,945,x+365,958),fill=desk);board.paste(ob,(x+188-ob.width//2,615),ob)
text(board,(24,590),'上：起始图；下：主页 110 px ×3。头部与小学 A 完全相同；四套待审。',19);save(board,'preview/outfit_remaining.png')
m=json.loads((ART/'manifest.json').read_text());m['remaining_outfits']=checks;m['remaining_video_handoff']={'file':'reference/GROK_REMAINING.zh_CN.md','stages':['cet4','cet6','adv','grad'],'shots':['01_idle','02_happy','03_sad'],'expected_videos':12,'videos_received':False,'owner_outfit_approval_pending':True};(ART/'manifest.json').write_text(json.dumps(m,ensure_ascii=False,indent=2)+'\n');(WORK/'wardrobe_validation.json').write_text(json.dumps(checks,ensure_ascii=False,indent=2)+'\n');print(json.dumps(checks,ensure_ascii=False))
