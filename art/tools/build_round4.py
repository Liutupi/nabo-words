"""Export round-four ImageGen sources to requested 4x PNGs and inspectable size proofs.
Only art/ is written; existing approved Doudou/three prop designs are reused unchanged.
"""
from pathlib import Path
import json,math,subprocess,shutil,hashlib
import numpy as np
from PIL import Image,ImageDraw,ImageFont
ART=Path(__file__).resolve().parents[1];WORK=ART/'design/round4';SRC=WORK/'source'
JOBS=json.loads((WORK/'prompts.json').read_text())['jobs']
NEAR=Image.Resampling.NEAREST;LANC=Image.Resampling.LANCZOS
PAPER='#FFF8EC';INK='#121829'
FONT=subprocess.check_output(['fc-match','-f','%{file}','PingFang SC'],text=True).strip()
def label(im,xy,s,n=16,color=INK):ImageDraw.Draw(im).text(xy,s,font=ImageFont.truetype(FONT,n),fill=color)
def save(im,rel):
 p=ART/rel;p.parent.mkdir(parents=True,exist_ok=True);im.save(p,optimize=True);return im

def trim(im):
 im=im.convert('RGBA');a=im.getchannel('A').point(lambda v:255 if v>=128 else 0)
 if not a.getbbox():raise ValueError('Empty image')
 im.putalpha(a);return im.crop(a.getbbox())

def flat(im,colors):
 im=im.convert('RGBA');p=Image.new('P',(1,1));rgb=[tuple(bytes.fromhex(c[1:])) for c in colors]
 p.putpalette([v for c in rgb for v in c]+list(rgb[-1])*(256-len(rgb)))
 result=im.convert('RGB').quantize(palette=p,dither=Image.Dither.NONE).convert('RGBA')
 result.putalpha(im.getchannel('A').point(lambda v:255 if v>=128 else 0));return result

PALETTES={
'paopao':['#0B344A','#104C69','#237DA0','#317AB3','#43B3E6','#75CEF3','#ACE5F5','#D4F2F4','#FFF5D8','#FFBE84','#E9A86C','#5CA9D8','#F9E8B4','#E9F9F2','#1B567E','#B9E8F5'],
'xingxing':['#261431','#341A4D','#6B4590','#9D75CA','#C7A6EA','#DFC7F4','#FFF1CB','#E8D19E','#F8CD50','#D29A2B','#FFC08D','#E89F73','#FFF8E4','#B58BDE','#8857B2','#EBD8F5'],
'huohuo':['#401A17','#5C1D17','#8B3522','#A74E2E','#DF6B29','#F59A3E','#FFC46B','#FF792A','#FFA63F','#FFDD84','#FFF1C9','#FFBA8F','#E98B63','#FFD65A','#C89632','#FFE6AF'],
'langlang':['#0B2F3D','#123B47','#0B7184','#1999AE','#3FC2C6','#78DDD6','#A4EAE0','#D8F3E9','#FFF2CD','#E7DDAF','#F6A581','#D47C61','#FFC5A2','#FFC68C','#FFF8E2','#22929A'],
'jiguang':['#35203E','#4D244B','#8A3F79','#B85C9D','#D981B6','#F1ABCF','#F6C7E2','#FFF0D2','#BAA7E5','#967AC7','#F2A18F','#FFBEA8','#E9C055','#D99B3D','#E98DBC','#FFE0E5','#FDC3CB']}
GENERAL=['#2D201C','#173342','#1E3F43','#315D2C','#57923D','#87B846','#BDD66A','#6D3B2D','#945735','#C17D4A','#D99D62','#FFF1D2','#E3D2AB','#465D6C','#8197A3','#B6C4CA','#2787B8','#63B9D9','#A9DCE5','#8C3338','#CC5952','#F28D71','#A87328','#D9A33D','#FFD560','#F7E891','#6A5B8C','#A28BC4','#E5DCF0','#F4F5E9','#E97531','#FFC76F']
STAGES=[('primary','小学','#F6D7A7','#C88A52'),('junior','初中','#BFE3E0','#B47C55'),('senior','高中','#C9D3EE','#916243'),('cet4','四级','#D9D4C7','#715A48'),('cet6','六级','#E8DDF0','#7E5A47'),('adv','进阶','#24324F','#5B483B')]
SPECIES=[('doudou','豆豆'),('paopao','泡泡'),('xingxing','星星'),('huohuo','火火'),('langlang','浪浪'),('jiguang','极光')]
FORMS=[('egg',28),('baby',32),('teen',40),('final',48)]

# Four opaque column runs are the four independent forms. Crop between true gaps,
# preserving a final halo/wing even when it is a separate connected component.
def split_sheet(path):
 im=Image.open(path).convert('RGBA');a=np.asarray(im)[:,:,3]>128
 col=a.sum(axis=0)>3;runs=[];start=None
 for i,value in enumerate(np.r_[col,False]):
  if value and start is None:start=i
  if not value and start is not None:
   if i-start>im.width*.02:runs.append((start,i))
   start=None
 if len(runs)!=4:raise ValueError(f'{path.name}: expected4 forms, got {runs}')
 assert not a[:,0].any() and not a[:,-1].any(),f'{path.name}: clipped source'
 bounds=[0]+[(runs[i][1]+runs[i+1][0])//2 for i in range(3)]+[im.width]
 return [trim(im.crop((bounds[i],0,bounds[i+1],im.height))) for i in range(4)]

assets={};missing=[]
def register(rel,im,target,group,name,anchor='bottom_center',**extra):
 arr=np.asarray(im.convert('RGBA'));visible=arr[:,:,3]>0
 cols=np.unique(arr[:,:,:3][visible],axis=0)
 ax,ay=(im.width/2,im.height/2) if anchor=='center' else (im.width/2,im.height)
 info={'id':name,'name':name,'canvas_px':list(im.size),'target_px':list(target),'anchor_px':[ax,ay],
 'target_anchor_px':[ax/4,ay/4],'anchor_type':anchor,'opaque_rgb_colors':len(cols),
 'palette':['#'+bytes(c).hex() for c in cols],'sha256':hashlib.sha256((ART/rel).read_bytes()).hexdigest(),**extra}
 assert im.size==(target[0]*4,target[1]*4),(rel,im.size,target)
 assert im.mode=='RGBA' and arr[:,:,3].min()==0 and arr[:,:,3].max()==255,rel
 assert len(cols)<=32,(rel,len(cols))
 assets[rel]=(im,group,info)

for species,cname in SPECIES:
 if species=='doudou':
  for form,h in FORMS:
   dest=ART/f'companion/{species}/{form}.png';dest.parent.mkdir(parents=True,exist_ok=True)
   original=ART/f'design/doudou_{form}.png'
   if dest.exists():assert dest.read_bytes()==original.read_bytes(),str(dest)
   else:shutil.copy2(original,dest)
   im=Image.open(dest).convert('RGBA');register(str(dest.relative_to(ART)),im,(im.width//4,h),'companion',f'{species}_{form}',species=species,form=form,source=f'design/doudou_{form}.png')
  continue
 path=SRC/f'{species}_sheet.png'
 if not path.exists():missing.append(path.name);continue
 for (form,h),obj in zip(FORMS,split_sheet(path)):
  nw=round(obj.width*(h*4)/obj.height);im=flat(obj.resize((nw,h*4),LANC),PALETTES[species])
  canvas=Image.new('RGBA',(math.ceil(nw/4)*4,h*4));canvas.alpha_composite(im,((canvas.width-nw)//2,0))
  rel=f'companion/{species}/{form}.png';save(canvas,rel)
  register(rel,canvas,(canvas.width//4,h),'companion',f'{species}_{form}',species=species,form=form,source=f'design/round4/source/{species}_sheet.png')

# Keep the three accepted props exactly; the remaining nine are new exports.
old_props={'lamp':(24,44),'clock':(28,28),'pencilcase':(34,12)}
for name,target in old_props.items():
 im=Image.open(ART/f'props/{name}.png').convert('RGBA')
 register(f'props/{name}.png',im,target,'props',name,'center' if name=='clock' else 'bottom_center')

for job in JOBS:
 if job.get('group')=='companion':continue
 source=SRC/(job['id']+'.png')
 if not source.exists():missing.append(source.name);continue
 group=job['group'];name=job['id'].split('_',1)[1];tw,th=job['target_px']
 obj=trim(Image.open(source))
 pixel_grid=group in ('toys','ui')
 scale=min(tw*(1 if pixel_grid else 4)/obj.width,th*(1 if pixel_grid else 4)/obj.height)
 obj=flat(obj.resize((max(1,round(obj.width*scale)),max(1,round(obj.height*scale))),LANC),GENERAL)
 if pixel_grid:
  canvas=Image.new('RGBA',(tw,th));canvas.alpha_composite(obj,((tw-obj.width)//2,th-obj.height))
  canvas=canvas.resize((tw*4,th*4),NEAR)
 else:
  canvas=Image.new('RGBA',(tw*4,th*4));canvas.alpha_composite(obj,((canvas.width-obj.width)//2,canvas.height-obj.height))
 rel=f'toys/primary/{name}.png' if group=='toys' else f'{group}/{name}.png';save(canvas,rel)
 anchor='center' if group=='ui' or name in ('map','cert') else 'bottom_center'
 register(rel,canvas,(tw,th),group,name,anchor,pixel_grid_4x=pixel_grid,source=f'design/round4/source/{job["id"]}.png')

if 'ui/flame.png' in assets:
 im=assets['ui/flame.png'][0];ar=np.array(im);lum=(ar[:,:,0]*.2126+ar[:,:,1]*.7152+ar[:,:,2]*.0722)
 shades=np.array([[57,65,81],[98,109,126],[150,160,174],[194,200,207]],dtype=np.uint8)
 index=np.clip((lum/64).astype(int),0,3);ar[:,:,:3]=shades[index]
 dim=Image.fromarray(ar);save(dim,'ui/flame_dim.png');register('ui/flame_dim.png',dim,(12,12),'ui','flame_dim','center',pixel_grid_4x=True,state='inactive',derived_from='ui/flame.png')

# Previews are real export pixels on their stage backgrounds, never generative mockups.
if all(f'companion/{s}/final.png' in assets for s,_ in SPECIES):
 family=Image.new('RGBA',(1440,330),PAPER)
 for i,(s,cname) in enumerate(SPECIES):
  im=assets[f'companion/{s}/final.png'][0]
  family.alpha_composite(im,(i*240+120-im.width//2,220-im.height))
  label(family,(i*240+85,248),cname,27)
  label(family,(i*240+68,285),'48 px ×4',18)
 save(family.convert('RGB'),'preview/companion_family.png')
 growth=Image.new('RGBA',(1120,1510),PAPER)
 for row,((s,cname),stage) in enumerate(zip(SPECIES,STAGES)):
  y=row*248;ImageDraw.Draw(growth).rectangle((0,y,1119,y+247),fill=stage[2])
  label(growth,(12,y+10),cname,22,'#FFF8EC' if row==5 else INK)
  for col,(form,h) in enumerate(FORMS):
   ob=assets[f'companion/{s}/{form}.png'][0];growth.alpha_composite(ob,(col*280+140-ob.width//2,y+210-ob.height))
   label(growth,(col*280+102,y+217),form+' '+str(h),15,'#FFF8EC' if row==5 else INK)
 save(growth.convert('RGB'),'preview/companion_growth.png')
 native=Image.new('RGBA',(272,570),PAPER)
 for row,((s,cname),stage) in enumerate(zip(SPECIES,STAGES)):
  y=row*95;d=ImageDraw.Draw(native);d.rectangle((0,y,271,y+94),fill=stage[2]);d.rectangle((0,y+66,271,y+94),fill=stage[3])
  label(native,(4,y+3),cname,10,'#FFF8EC' if row==5 else INK)
  for col,(form,h) in enumerate(FORMS):
   ob=assets[f'companion/{s}/{form}.png'][0];ob=ob.resize((ob.width//4,h),LANC)
   native.alpha_composite(ob,(col*64+45-ob.width//2,y+66-h))
   label(native,(col*64+30,y+76),str(h),10,'#FFF8EC')
 save(native.convert('RGB').resize((1088,2280),NEAR),'preview/companion_size_check.png')

prop_order=['pencilcase','clock','lamp','plant','globe','shelf','cert','phones','laptop','mug','map','trophy']
prop_stages=[0,0,0,0,0,1,1,2,2,3,4,5]
if all(f'props/{x}.png' in assets for x in prop_order):
 proof=Image.new('RGBA',(240,272),PAPER)
 for i,(name,st) in enumerate(zip(prop_order,prop_stages)):
  x=i%3*80;y=i//3*68;d=ImageDraw.Draw(proof);d.rectangle((x,y,x+79,y+67),fill=STAGES[st][2])
  if name not in ['clock','shelf','plant','cert','map']:d.rectangle((x,y+48,x+79,y+67),fill=STAGES[st][3])
  ob=assets[f'props/{name}.png'][0];ob=ob.resize((ob.width//4,ob.height//4),LANC)
  proof.alpha_composite(ob,(x+40-ob.width//2,y+48-ob.height));label(proof,(x+4,y+53),name,9,'#FFF8EC' if st==5 else INK)
 save(proof.convert('RGB').resize((960,1088),NEAR),'preview/props_check.png')

toy_order=['duck','rocket','cactus','snowglobe','cube','dino','plane','hourglass','shell','windmill']
if all(f'toys/primary/{n}.png' in assets for n in toy_order):
 proof=Image.new('RGBA',(300,370),PAPER)
 for col,name in enumerate(toy_order):label(proof,(60+col*24,2),str(col+1),10)
 for row,stage in enumerate(STAGES):
  for bgidx,bg in enumerate(stage[2:4]):
   y=20+row*48+bgidx*24;ImageDraw.Draw(proof).rectangle((0,y,299,y+23),fill=bg)
   label(proof,(2,y+4),stage[1]+('墙' if bgidx==0 else '桌'),9,'#FFF8EC' if row==5 else INK)
   for col,name in enumerate(toy_order):
    ob=assets[f'toys/primary/{name}.png'][0].resize((12,12),NEAR);proof.alpha_composite(ob,(66+col*24,y+6))
 y=320;label(proof,(2,y+3),'剪影',10)
 for col,name in enumerate(toy_order):
  ob=assets[f'toys/primary/{name}.png'][0].resize((12,12),NEAR);sil=Image.new('RGBA',(12,12),INK);sil.putalpha(ob.getchannel('A'))
  proof.alpha_composite(sil,(66+col*24,y+4))
 save(proof.convert('RGB').resize((1200,1480),NEAR),'preview/toys_check.png')
 silhouette=Image.new('RGBA',(1000,270),PAPER)
 for col,name in enumerate(toy_order):
  ob=assets[f'toys/primary/{name}.png'][0];sil=Image.new('RGBA',ob.size,INK);sil.putalpha(ob.getchannel('A'))
  silhouette.alpha_composite(ob,(col*100+26,26));silhouette.alpha_composite(sil,(col*100+26,111));label(silhouette,(col*100+8,185),name,13)
 save(silhouette.convert('RGB'),'preview/toys_silhouette.png')
if all('ui/'+x+'.png' in assets for x in ['coin','flame','flame_dim','freeze']):
 proof=Image.new('RGBA',(320,160),PAPER)
 for row,bg in enumerate([PAPER,INK]):
  ImageDraw.Draw(proof).rectangle((0,row*80,319,row*80+79),fill=bg)
  for col,name in enumerate(['coin','flame','flame_dim','freeze']):
   ob=assets['ui/'+name+'.png'][0];proof.alpha_composite(ob,(col*80+16,row*80+6));label(proof,(col*80+4,row*80+58),name,12,'#FFF8EC' if row else INK)
 save(proof.convert('RGB'),'preview/ui_check.png')

manifest_path=ART/'manifest.json';manifest=json.loads(manifest_path.read_text())
for rel,(im,group,info) in assets.items():manifest.setdefault(group,{})[rel]=info
for name in ['companion_family','companion_growth','companion_size_check','props_check','toys_check','toys_silhouette','ui_check']:
 p=ART/f'preview/{name}.png'
 if p.exists():manifest.setdefault('previews',{})[name+'.png']={'file':f'preview/{name}.png','size':list(Image.open(p).size),'round':4}
manifest['round4']={'expected_static_assets':50,'exported_static_assets':len(assets),'missing_sources':missing,
 'canonical_groups':['companion','props','toys','ui'],'source_designs':'design/round4/source/',
 'prompts':'design/round4/prompts.json','generator':'Codex built-in ImageGen',
 'alpha_policy':'binary; edge premixing is deferred to packer','icons':'three icon types; flame includes active/inactive variants',
 'scope':'Round 4 only. The 50 additional toys and 8 accessories added as round 5 are not included.',
 'qa':'QA.md','qa_zh_CN':'QA.zh_CN.md',
 'inventory':'design/round4/README.md','inventory_zh_CN':'design/round4/README.zh_CN.md'}
manifest_path.write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
summary={'expected':50,'exported':len(assets),'missing_sources':missing,'color_counts':{p:i['opaque_rgb_colors'] for p,(_,_,i) in assets.items()}}
(WORK/'export_summary.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n')
print(json.dumps({'exported':len(assets),'missing_sources':missing},ensure_ascii=False))
