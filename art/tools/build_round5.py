"""Export round-five static art and requested native/4x review compositions."""
from pathlib import Path
import json,hashlib,subprocess
import numpy as np
from PIL import Image,ImageDraw,ImageFont
ART=Path(__file__).resolve().parents[1];WORK=ART/'design/round5';SRC=WORK/'source'
JOBS=json.loads((WORK/'prompts.json').read_text())['jobs']
NEAR=Image.Resampling.NEAREST;LANC=Image.Resampling.LANCZOS
PAPER='#FFF8EC';INK='#121829'
FONT=subprocess.check_output(['fc-match','-f','%{file}','PingFang SC'],text=True).strip()
def label(im,xy,s,n=10,color=INK):ImageDraw.Draw(im).text(xy,s,font=ImageFont.truetype(FONT,n),fill=color)
def save(im,rel):
 p=ART/rel;p.parent.mkdir(parents=True,exist_ok=True);im.save(p,optimize=True)
 return im
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
PALETTE=['#2D201C','#173342','#1E3F43','#315D2C','#57923D','#87B846','#BDD66A','#6D3B2D','#945735','#C17D4A','#D99D62','#FFF1D2','#E3D2AB','#465D6C','#8197A3','#B6C4CA','#2787B8','#63B9D9','#A9DCE5','#8C3338','#CC5952','#F28D71','#A87328','#D9A33D','#FFD560','#F7E891','#6A5B8C','#A28BC4','#E5DCF0','#F4F5E9','#E97531','#FFC76F','#F1ABCF','#D981B6','#B85C9D','#A4EAE0','#1999AE','#0B7184','#101622']
def flat(im):
 p=Image.new('P',(1,1));rgb=[tuple(bytes.fromhex(c[1:])) for c in PALETTE]
 p.putpalette([v for c in rgb for v in c]+list(rgb[-1])*(256-len(rgb)))
 out=im.convert('RGB').quantize(palette=p,dither=Image.Dither.NONE).convert('RGBA')
 out.putalpha(im.getchannel('A').point(lambda v:255 if v>=128 else 0))
 ar=np.array(out);mask=ar[:,:,3]>0
 if len(np.unique(ar[:,:,:3][mask],axis=0))>32:
  reduced=out.convert('RGB').quantize(32,dither=Image.Dither.NONE).convert('RGBA');reduced.putalpha(out.getchannel('A'));out=reduced
 return out

STAGES=[('primary','小学','#F6D7A7','#C88A52'),('junior','初中','#BFE3E0','#B47C55'),('senior','高中','#C9D3EE','#916243'),('cet4','四级','#D9D4C7','#715A48'),('cet6','六级','#E8DDF0','#7E5A47'),('adv','进阶','#24324F','#5B483B')]
PRIMARY=['duck','rocket','cactus','snowglobe','cube','dino','plane','hourglass','shell','windmill']
manifest_path=ART/'manifest.json';m=json.loads(manifest_path.read_text());assets={};missing=[];migration={}
for name in PRIMARY:
 old=ART/f'toys/{name}.png';new=ART/f'toys/primary/{name}.png';new.parent.mkdir(parents=True,exist_ok=True)
 if old.exists():
  if new.exists():assert old.read_bytes()==new.read_bytes();old.unlink()
  else:old.rename(new)
 assert new.exists(),str(new)
 previous=m.setdefault('toys',{}).pop(f'toys/{name}.png',None)
 info=previous or m['toys'][f'toys/primary/{name}.png']
 info['stage']='primary';m['toys'][f'toys/primary/{name}.png']=info
 assets[f'toys/primary/{name}.png']=Image.open(new).convert('RGBA')
 migration[f'toys/{name}.png']=f'toys/primary/{name}.png'

for job in JOBS:
 if job['group']=='outfits':continue
 source=SRC/(job['id']+'.png')
 if not source.exists():missing.append(job['id']);continue
 im=Image.open(source).convert('RGBA');a=im.getchannel('A').point(lambda v:255 if v>=128 else 0)
 assert a.getbbox(),job['id'];im.putalpha(a);im=im.crop(a.getbbox())
 tw,th=job['target_px'];k=min(tw/im.width,th/im.height)
 im=flat(im.resize((max(1,round(im.width*k)),max(1,round(im.height*k))),LANC))
 py=(th-im.height)//2 if job.get('anchor_type')=='center' else th-im.height
 canvas=Image.new('RGBA',(tw,th));canvas.alpha_composite(im,((tw-im.width)//2,py))
 canvas=canvas.resize((tw*4,th*4),NEAR)
 group=job['group'];name=job['name'];stage=job.get('stage')
 rel=f'toys/{stage}/{name}.png' if group=='toys' else f'accessories/{name}.png';save(canvas,rel);assets[rel]=canvas
 ar=np.array(canvas);colors=np.unique(ar[:,:,:3][ar[:,:,3]>0],axis=0);assert len(colors)<=32
 anchor=job.get('anchor_type','bottom_center');ax,ay=(tw*2,th*2) if anchor=='center' else (tw*2,th*4)
 asset_id=name if group=='toys' else 'acc_'+name
 info={'id':asset_id,'name':asset_id,'slug':name,'canvas_px':list(canvas.size),'target_px':[tw,th],'anchor_px':[ax,ay],'target_anchor_px':[ax/4,ay/4],'anchor_type':anchor,'opaque_rgb_colors':len(colors),'palette':['#'+bytes(c).hex() for c in colors],'sha256':digest(ART/rel),'source':f'design/round5/source/{job["id"]}.png','pixel_grid_4x':True}
 if stage:info['stage']=stage
 if group=='accessories':info['slot']=job['slot'];info['layer']='behind' if name=='cape' else 'front'
 m.setdefault(group,{})[rel]=info

# These are placement suggestions for the delivered layers, not changes to the prototype.
PLACEMENT={
 'flower':{'width_ratio':.36,'x_halfwidth':-.55,'y_height':.16},
 'bow':{'width_ratio':.58,'x_halfwidth':0,'y_height':.16},
 'scarf':{'width_ratio':.70,'x_halfwidth':0,'y_height':.88},
 'star':{'width_ratio':.33,'x_halfwidth':.55,'y_height':.16},
 'shades':{'width_ratio':.78,'x_halfwidth':0,'y_height':.55,'form_y_height':{'baby':.56,'final':.53}},
 'cape':{'width_ratio':1.55,'height_ratio':.55,'x_halfwidth':0,'y_height':.70},
 'phones':{'width_ratio':1.06,'x_halfwidth':0,'y_height':.57},
 'gradcap':{'width_ratio':.75,'x_halfwidth':0,'y_height':.10}}
for name,p in PLACEMENT.items():
 rel=f'accessories/{name}.png'
 if rel in m.get('accessories',{}):m['accessories'][rel]['placement_suggestion']=p

orders={'primary':PRIMARY}
for stage,_,_,_ in STAGES[1:]:orders[stage]=[j['name'] for j in JOBS if j.get('stage')==stage and j['group']=='toys']
all_native=Image.new('RGBA',(300,6*102),PAPER)
for row,(stage,cname,wall,desk) in enumerate(STAGES):
 native=Image.new('RGBA',(300,102),wall);d=ImageDraw.Draw(native);d.rectangle((0,49,299,72),fill=desk);d.rectangle((0,73,299,101),fill=PAPER)
 label(native,(3,2),cname+' / 12 px',10,PAPER if stage=='adv' else INK)
 for col,name in enumerate(orders[stage]):
  x=30+col*26;label(native,(x,17),str(col+1),8,PAPER if stage=='adv' else INK)
  rel=f'toys/{stage}/{name}.png'
  if rel not in assets:continue
  ob=assets[rel].resize((12,12),NEAR);native.alpha_composite(ob,(x,32));native.alpha_composite(ob,(x,55))
  sil=Image.new('RGBA',ob.size,INK);sil.putalpha(ob.getchannel('A'));native.alpha_composite(sil,(x,82))
 save(native.convert('RGB'),f'preview/toys_{stage}_native.png')
 board=Image.new('RGBA',(1200,560),PAPER);label(board,(12,5),cname+' / 原尺寸 + 最近邻 ×4',18);board.alpha_composite(native,(450,35));board.alpha_composite(native.resize((1200,408),NEAR),(0,150));save(board.convert('RGB'),f'preview/toys_{stage}.png')
 all_native.alpha_composite(native,(0,row*102))
save(all_native.convert('RGB').resize((1200,2448),NEAR),'preview/toys_all.png')

accessory_names=list(PLACEMENT)
wear=Image.new('RGBA',(576,192),'#F6D7A7')
for row,form in enumerate(['baby','final']):
 base=70+row*96;ImageDraw.Draw(wear).rectangle((0,base,575,base+25),fill='#C88A52')
 for col,name in enumerate(accessory_names):
  cx=col*72+36;label(wear,(col*72+3,row*96+2),name,9)
  body=Image.open(ART/f'companion/doudou/{form}.png').convert('RGBA');body=body.resize((body.width//4,body.height//4),LANC);bw,bh=body.size
  rel=f'accessories/{name}.png'
  if rel not in assets:wear.alpha_composite(body,(cx-bw//2,base-bh));continue
  p=PLACEMENT[name];ob=assets[rel].resize((12,10),NEAR)
  k=bw*p['width_ratio']/12;hh=round(bh*p['height_ratio']) if 'height_ratio' in p else round(10*k)
  ob=ob.resize((max(1,round(12*k)),max(1,hh)),NEAR)
  anchor=m['accessories'][rel]['target_anchor_px'];tx=cx+round(bw/2*p['x_halfwidth']);ty=base-bh+round(bh*p.get('form_y_height',{}).get(form,p['y_height']))
  pos=(round(tx-anchor[0]*ob.width/12),round(ty-anchor[1]*ob.height/10))
  if name=='cape':wear.alpha_composite(ob,pos);wear.alpha_composite(body,(cx-bw//2,base-bh))
  else:wear.alpha_composite(body,(cx-bw//2,base-bh));wear.alpha_composite(ob,pos)
save(wear.convert('RGB'),'preview/accessories_native.png')
board=Image.new('RGBA',(2304,1010),PAPER);label(board,(12,5),'豆豆幼崽 / 完全体 · 原尺寸与最近邻 ×4；披风在身体后方',22);board.alpha_composite(wear,(864,40));board.alpha_composite(wear.resize((2304,768),NEAR),(0,242));save(board.convert('RGB'),'preview/accessories.png')

m['round5']={'new_static_assets_expected':58,'new_static_assets_exported':len(assets)-10,'total_toys':len(m['toys']),'accessories':len(m.get('accessories',{})),'missing_sources':missing,'generator':'Codex built-in ImageGen','source_jobs':62,'prompts':'design/round5/prompts.json','qa':'QA.md','qa_zh_CN':'QA.zh_CN.md','toy_path_migration':migration,'prototype_integration':'not changed; consume manifest and accessory placement suggestions'}
for name in ['toys_all','accessories','accessories_native']+[f'toys_{s}' for s,_,_,_ in STAGES]+[f'toys_{s}_native' for s,_,_,_ in STAGES]:
 m.setdefault('previews',{})[name+'.png']={'file':f'preview/{name}.png','size':list(Image.open(ART/f'preview/{name}.png').size),'round':5}
manifest_path.write_text(json.dumps(m,ensure_ascii=False,indent=2)+'\n')
(WORK/'export_summary.json').write_text(json.dumps(m['round5'],ensure_ascii=False,indent=2)+'\n')
print(json.dumps(m['round5'],ensure_ascii=False))
