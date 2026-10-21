"""Export clothing-only candidates while retaining the selected primary head exactly."""
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

specs=[('junior_a','初中 A / 蓝身白袖','#BFE3E0','#B47C55'),
 ('junior_b','初中 B / 白身蓝肩','#BFE3E0','#B47C55'),
 ('senior_a','高中 A / 素面藏青','#C9D3EE','#916243'),
 ('senior_b','高中 B / 浅色滚边','#C9D3EE','#916243')]
cutouts={};smalls={};heads={};geometry={};checks={}
for name,title,wall,desk in specs:
 raw=Image.open(ART/f'design/source/{name}.png').convert('RGB')
 ar=np.asarray(raw).astype(np.int16);r,g,b=ar[:,:,0],ar[:,:,1],ar[:,:,2]
 green=(g>80)&(g>r*1.3)&(g>b*1.25)
 row=np.flatnonzero((~green[:,0])&(np.arange(raw.height)>raw.height*.55));edge=int(row[0])
 y,x=np.where(~green[:edge-3]);top=int(y.min());left=int(x.min());right=int(x.max())+1
 rgba=raw.convert('RGBA');rgba.putalpha(Image.fromarray(np.uint8(~green)*255))
 obj=rgba.crop((left,top,right,edge+40))
 k=704/(edge-top)
 obj=obj.resize((round(obj.width*k),round(obj.height*k)),LANC)
 extras=['#3268B7','#224D87','#77A8DE'] if name.startswith('junior') else ['#223454','#142338','#3B5274','#B94044','#8A2E38']
 palette=headpalette+extras
 obj=flat(obj,palette)
 candidate=Image.new('RGBA',(1024,1280));candidate.alpha_composite(obj,(512-obj.width//2,366))
 ca=np.array(candidate)
 # Only clothes between jaw and desk are new. The original head and lower desk/hands are exact.
 ca[:HEAD_END]=bc[:HEAD_END];ca[1070:]=bc[1070:];ca[hm]=bc[hm]
 candidate=Image.fromarray(ca)
 canvas=Image.new('RGBA',(1024,1280),'#00B140')
 ImageDraw.Draw(canvas).rectangle((0,1070,1023,1279),fill='#C88A52')
 canvas.alpha_composite(candidate)
 image=canvas.convert('RGB')
 save(image,f'design/{name}.png');save(candidate,f'design/source/{name}_cutout.png')
 cutouts[name]=candidate
 final=np.asarray(image)
 assert np.array_equal(final[:HEAD_END],ba[:HEAD_END]),name
 assert np.array_equal(final[1070:],ba[1070:]),name
 assert np.array_equal(final[hm],ba[hm]),name
 cols=len(np.unique(ca[:,:,:3][ca[:,:,3]>0],axis=0));assert cols<=32,(name,cols)
 delta=np.any(final!=ba,axis=2);dy,dx=np.where(delta)
 assert dy.min()>=HEAD_END and dy.max()<1070
 # Native size checks in the correct stage wall color and shared quiz warm white.
 bbox=candidate.getchannel('A').getbbox()
 sprite=candidate.crop((bbox[0],366,bbox[2],bbox[3]))
 small=flat(sprite.resize((round(sprite.width*110/704),round(sprite.height*110/704)),LANC),palette)
 hbox=BC.crop((0,366,1024,824));hbox=hbox.crop(hbox.getchannel('A').getbbox())
 head=flat(hbox.resize((round(hbox.width*48/hbox.height),48),LANC),headpalette)
 smalls[name]=small;heads[name]=head
 proof=Image.new('RGBA',(240,180),'#FFF8EC');d=ImageDraw.Draw(proof)
 d.rectangle((8,28,137,153),fill=wall);d.rectangle((8,138,137,153),fill=desk)
 proof.alpha_composite(small,(73-small.width//2,28));proof.alpha_composite(head,(186-head.width//2,70))
 text(proof,(9,7),name.replace('_',' ').upper()+' / 110 px',10)
 text(proof,(153,45),'HEAD / 48 px',10)
 text(proof,(9,159),'原尺寸 → 最近邻 ×4',11)
 save(proof.convert('RGB').resize((960,720),NEAR),f'design/{name}_check.png')
 geometry[name]={'canvas_px':[1024,1280],'cap_top':[512,366],'desk_anchor':[512,1070],
  'head_to_desk_px':704,'identity_source':'start/primary.png','identical_upper_rows':[0,830],
  'opaque_rgb_colors':cols,'selected':False,'clothing_difference_bbox':[int(dx.min()),int(dy.min()),int(dx.max()+1),int(dy.max()+1)]}
 checks[name]={'head_max_pixel_difference':0,'table_and_lower_hands_max_pixel_difference':0,
  'restored_hands_max_pixel_difference':0,'opaque_rgb_colors':cols,'differences_confined_to_clothing_band':True}

board=Image.new('RGB',(1720,1100),'#FFF8EC')
text(board,(32,22),'NABO 单词护照 / 初中、高中服装候选',34)
for i,(name,title,wall,desk) in enumerate(specs):
 x=32+i*416
 text(board,(x,85),title,23)
 board.paste(Image.open(ART/f'design/{name}.png').resize((384,480),LANC),(x,130))
 text(board,(x+10,640),'主页 110 px ×3',17)
 d=ImageDraw.Draw(board);d.rectangle((x+8,678,x+254,1030),fill=wall)
 d.rectangle((x+8,1008,x+254,1030),fill=desk)
 ob=smalls[name].resize((smalls[name].width*3,smalls[name].height*3),NEAR)
 board.paste(ob,(x+131-ob.width//2,678),ob)
 text(board,(x+260,760),'头部 48 px ×3',16)
 ob=heads[name].resize((heads[name].width*3,144),NEAR)
 board.paste(ob,(x+260,800),ob)
text(board,(32,1060),'四版头部与小学 A 逐像素相同；各自的完整 ×4 检查图另附。初中、高中各选一版后再交 Grok。',22)
save(board,'preview/round3_contact.png')

# Growth comparison follows owner selections, with A only as an unselected fallback.
current=json.loads((ART/'manifest.json').read_text())
jv=current.get('selected_junior') or 'a'
sv=current.get('selected_senior') or 'a'
jlabel='初中 '+jv.upper()+(' · 已定稿' if current.get('selected_junior') else ' · 候选')
slabel='高中 '+sv.upper()+(' · 已定稿' if current.get('selected_senior') else ' · 候选')
growth=Image.new('RGB',(1400,760),'#FFF8EC')
text(growth,(38,24),'服装成长对照 / 小学 → 初中 → 高中',32)
for i,(path,title) in enumerate([('start/primary.png','小学 A · 已定稿'),(f'design/junior_{jv}.png',jlabel),(f'design/senior_{sv}.png',slabel)]):
 x=38+i*454
 text(growth,(x,92),title,25)
 growth.paste(Image.open(ART/path).resize((420,525),LANC),(x,138))
text(growth,(38,699),'头部、白帽、表情、桌沿与构图锁定；仅通过校服表达成长。各学段方案状态见上方标记。',22)
save(growth,'preview/outfit_growth.png')
manifest_path=ART/'manifest.json';manifest=json.loads(manifest_path.read_text())
for name,info in geometry.items():
 stage,variant=name.split('_')
 info['selected']=manifest.get('selected_'+stage)==variant
manifest['round3_candidates']=geometry
manifest.setdefault('selected_junior',None);manifest.setdefault('selected_senior',None)
manifest_path.write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
validation_path=ART/'design/round3_validation.json'
validation=json.loads(validation_path.read_text()) if validation_path.exists() else {}
validation.update({'candidates':checks,'selected_junior':manifest['selected_junior'],
 'selected_senior':manifest['selected_senior'],'pending':['Junior/senior video actions after selection','Physical display']})
if not manifest['selected_junior'] or not manifest['selected_senior']:
 validation['pending'].insert(0,'Owner selection: one junior, one senior')
validation_path.write_text(json.dumps(validation,ensure_ascii=False,indent=2)+'\n')
print(json.dumps(checks,ensure_ascii=False,indent=2))
