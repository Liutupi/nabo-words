"""Rebuild requested round-one sizes, palette proofs and composites from ImageGen sources.
No source illustration or file outside art/ is modified. Requires Pillow and numpy.
"""
from pathlib import Path
import json
import subprocess
import numpy as np
from PIL import Image, ImageDraw, ImageFont

ART = Path(__file__).resolve().parents[1]
SRC = ART / 'design/source'
WALL, TRIM, TOP, DESK, PAPER = '#F6D7A7', '#E3B57C', '#C88A52', '#B07845', '#FFF8EC'
NEAR, LANC = Image.Resampling.NEAREST, Image.Resampling.LANCZOS
INK = '#121829'
font_file = subprocess.check_output(['fc-match', '-f', '%{file}', 'PingFang SC'], text=True).strip()
def font(size): return ImageFont.truetype(font_file, size)
def label(im, xy, txt, size=16, fill=INK): ImageDraw.Draw(im).text(xy, txt, font=font(size), fill=fill)
def save(im, name):
    p = ART / name
    p.parent.mkdir(parents=True, exist_ok=True)
    im.save(p, optimize=True)
    return im

def trim(im):
    im = im.convert('RGBA')
    a = im.getchannel('A').point(lambda v: 255 if v >= 96 else 0)
    im.putalpha(a)
    return im.crop(a.getbbox())

NABO_PALETTE = ['#211B1A','#403531','#665249','#FFF9F5','#E7E1E8','#BFB8C3',
 '#2C608C','#95AECD','#FFCEA9','#EFAC89','#CF876A','#F5AE9D','#FFF8EC',
 '#E5DEDD','#FFD263','#E9AD3A','#BF8330','#BD5650','#873D3B','#918C8A',
 '#3B3B3B','#41656D','#29454E','#8D9AAD','#485A71','#18202E','#30282A',
 '#9B5641','#F7EFE8','#F6B997','#DACCC4','#F4BD49']
DOUDOU_PALETTE = ['#10291B','#1D431E','#426E27','#68A132','#8ABD38',
 '#A4CE45','#C0E46C','#D4ED8A','#E7EFAC','#F7F3C9','#FFF6DD',
 '#FFBD77','#ECA653','#FFDA65','#D6AD38','#5A8A31']
PROP_PALETTE = ['#2D201C','#571F27','#913B3B','#CB5850','#ED7867',
 '#F8DFB3','#FFF1D2','#E1A638','#FFD660','#173342','#2C5260',
 '#437C88','#6AA0A6','#9EC1BF','#C88A52','#B07845']

def flat(im, palette):
    """Map fills to explicit flat swatches, no dithering or RGB565 gradients."""
    im = im.convert('RGBA')
    p = Image.new('P', (1,1))
    rgb = [tuple(bytes.fromhex(x[1:])) for x in palette]
    p.putpalette([v for col in rgb for v in col] + list(rgb[-1]) * (256-len(rgb)))
    out = im.convert('RGB').quantize(palette=p, dither=Image.Dither.NONE).convert('RGBA')
    out.putalpha(im.getchannel('A').point(lambda v: 255 if v >= 128 else 0))
    return out

def scale_h(im, h, palette=None):
    out = im.resize((round(im.width*h/im.height),h), LANC)
    return flat(out,palette) if palette else out

def onto(im, sprite, x, y): im.alpha_composite(sprite, (round(x),round(y)))

# Isolate the illustrated person from the explicitly requested chroma backdrop/desk.
# The generated source is deliberately retained, so this exact export is reversible.
people = {}
metadata = {}
for variant in ('a','b'):
    raw = Image.open(SRC / f'primary_{variant}.png').convert('RGB')
    arr = np.asarray(raw).astype(np.int16)
    r,g,b = arr[:,:,0],arr[:,:,1],arr[:,:,2]
    green = (g>80)&(g>r*1.30)&(g>b*1.25)
    left = np.flatnonzero(~green[:,0] & (np.arange(raw.height)>raw.height*.55))
    desk_y = int(left[0])
    mask = ~green
    # Find the person before the horizontal table line, excluding any table pixels.
    yy,xx = np.where(mask[:desk_y-3])
    cap_top, x0, x1 = int(yy.min()), int(xx.min()), int(xx.max())+1
    wood = (r>125)&(r<240)&(g>65)&(g<185)&(b<145)&(r>g*1.17)&(g>b*1.08)
    mask[desk_y-3:] &= ~wood[desk_y-3:]
    skin = (r>235)&(g>145)&(g<225)&(b>130)
    hand_columns = np.any(skin[desk_y-10:desk_y+14],axis=0)
    mask[desk_y-3:desk_y+3] &= hand_columns
    mask[:, :x0] = False; mask[:, x1:] = False
    rgba = raw.convert('RGBA'); rgba.putalpha(Image.fromarray(np.uint8(mask)*255))
    crop = rgba.crop((x0,cap_top,x1,min(raw.height,desk_y+50)))
    scale = 704 / (desk_y-cap_top)
    person = crop.resize((round(crop.width*scale), round(crop.height*scale)), LANC)
    person = flat(person, NABO_PALETTE)
    # Cap top y=366, table edge y=1070, center x=512 in both candidates.
    cutout = Image.new('RGBA',(1024,1280))
    px,py = 512-person.width//2,366
    onto(cutout, person, px,py)
    key = Image.new('RGBA',(1024,1280),'#00B140')
    ImageDraw.Draw(key).rectangle((0,1070,1023,1279),fill=TOP)
    onto(key,cutout,0,0)
    save(key.convert('RGB'),f'design/primary_{variant}.png')
    save(cutout,f'design/source/primary_{variant}_cutout.png')
    people[variant] = cutout
    # Head-only crop ends just below the jaw, above the shirt/straps.
    head_bottom = cap_top + round((desk_y-cap_top)*.65)
    head = trim(rgba.crop((x0,cap_top,x1,head_bottom)))
    head48 = scale_h(head,48,NABO_PALETTE)
    main = scale_h(person,round(person.height*110/704),NABO_PALETTE)
    # Native-size proof, then nearest-neighbor enlargement, with separate backgrounds.
    proof = Image.new('RGBA',(240,180),PAPER)
    d = ImageDraw.Draw(proof)
    d.rectangle((8,28,137,153), fill=WALL)
    d.rectangle((8,138,137,153),fill=TOP)
    onto(proof,main,73-main.width//2,28)
    onto(proof,head48,186-head48.width//2,70)
    label(proof,(9,7),f'{variant.upper()} / 110 px',12)
    label(proof,(153,45),'HEAD / 48 px',10)
    label(proof,(9,159),'原尺寸 → 最近邻 ×4',11)
    save(proof.convert('RGB').resize((960,720),NEAR),f'design/primary_{variant}_check.png')
    metadata[variant] = {'cap_top':[512,366], 'desk_anchor':[512,1070],
      'head_to_desk_px':704,'canvas_px':[1024,1280], 'source_desk_y':desk_y,
      'head_crop_source':[x0,cap_top,x1,head_bottom]}

# Companion: keep the four designed forms, remove source margins and export 4x sprites.
sheet = Image.open(SRC/'doudou_sheet.png').convert('RGBA')
# Gaps observed in the source separate the non-overlapping four characters.
bands = [(0,450),(450,870),(870,1310),(1310,sheet.width)]
forms = {}
for (name,height),(x0,x1) in zip([('egg',28),('baby',32),('teen',40),('final',48)],bands):
    obj = trim(sheet.crop((x0,0,x1,sheet.height)))
    obj = scale_h(obj,height*4,DOUDOU_PALETTE)
    padded=Image.new('RGBA',(((obj.width+3)//4)*4,obj.height))
    onto(padded,obj,(padded.width-obj.width)//2,0)
    forms[name] = save(padded,f'design/doudou_{name}.png')
# Transparent evolution sheet: four aligned bottoms, shared scale rather than equal sizes.
out = Image.new('RGBA',(1280,520))
for i,(name,obj) in enumerate(forms.items()):
    big = obj.resize((obj.width*2,obj.height*2),NEAR)
    onto(out,big,i*320+160-big.width//2,424-big.height)
    label(out,(i*320+100,450),name.upper(),20, '#31502D')
    label(out,(i*320+100,480),f'{[28,32,40,48][i]} px',17,'#31502D')
save(out,'design/doudou_sheet.png')
proof=Image.new('RGBA',(256,102),TOP)
for i,(name,obj) in enumerate(forms.items()):
    tiny=flat(obj.resize((round(obj.width/4),obj.height//4),LANC),DOUDOU_PALETTE)
    onto(proof,tiny,i*64+32-tiny.width//2,70-tiny.height)
    label(proof,(i*64+14,81),f'{tiny.height}px',10,'#FFF8EC')
save(proof.convert('RGB').resize((1024,408),NEAR),'design/doudou_check.png')

# Exact 4x prop canvases, fitting without distorting aspect ratio.
props = {}
for name,size in [('lamp',(24,44)),('clock',(28,28)),('pencilcase',(34,12))]:
    obj=trim(Image.open(SRC/f'{name}.png'))
    w,h=size[0]*4,size[1]*4
    s=min(w/obj.width,h/obj.height)
    obj=flat(obj.resize((round(obj.width*s),round(obj.height*s)),LANC),PROP_PALETTE)
    canvas=Image.new('RGBA',(w,h))
    onto(canvas,obj,(w-obj.width)//2,h-obj.height)
    props[name]=save(canvas,f'props/{name}.png')

# Requested art composition follows the prototype's coordinates and layers.
home=Image.new('RGBA',(240,320),WALL)
d=ImageDraw.Draw(home);d.rectangle((0,132,239,134),fill=TRIM)
def target_prop(name):
    ob=props[name]
    return flat(ob.resize((ob.width//4,ob.height//4),LANC),PROP_PALETTE)
onto(home,target_prop('clock'),20,38)
# Half-body height: exactly 110 px from cap top (y66) to table edge (y176).
person=people['a']
mini=person.resize((160,200),LANC) # 704/6.4 = 110.
mini=flat(mini,NABO_PALETTE)
onto(home,mini,40,9) # cap 366/6.4 +9 rounds to 66; edge1070/6.4+9 rounds176.
d=ImageDraw.Draw(home);d.rectangle((0,176,239,181),fill=TOP);d.rectangle((0,182,239,213),fill=DESK)
# Hands overlap the table edge, as in the starting image, but torso stays behind it.
hand_overlay=mini.copy();ha=np.asarray(hand_overlay.getchannel('A')).copy();ha[:167]=0
hand_overlay.putalpha(Image.fromarray(ha));onto(home,hand_overlay,40,9)
onto(home,target_prop('lamp'),17,132)
onto(home,target_prop('pencilcase'),70,184)
baby=flat(forms['baby'].resize((round(forms['baby'].width/4),32),LANC),DOUDOU_PALETTE)
onto(home,baby,196-baby.width//2,144)
# Program-drawn example UI copied in placement, without modifying the prototype.
d=ImageDraw.Draw(home);d.rectangle((0,0,239,23),fill='#3B3A36')
label(home,(8,4),'小学 · 北京',12,'#FFFFFF');label(home,(163,5),'7 天   12 金',10,'#F5D580')
d.rectangle((0,214,239,319),fill=INK)
label(home,(12,222),'已掌握',11,'#8C93A8');label(home,(54,218),'150',19,'#FFFFFF')
label(home,(95,224),'词',11,'#8C93A8');label(home,(179,224),'小学 21%',10,'#E8B84A')
d.rounded_rectangle((12,242,227,246),radius=2,fill='#2A3350')
d.rounded_rectangle((12,242,57,246),radius=2,fill='#E8B84A')
d.rounded_rectangle((12,256,227,311),radius=9,fill='#1E2740')
label(home,(83,264),'今日学习',17,'#FFFFFF');label(home,(53,290),'10 个新词 · 8 个复习',11,'#8C93A8')
save(home.convert('RGB'),'preview/home_primary.png')
save(home.convert('RGB').resize((480,640),NEAR),'preview/home_primary_2x.png')

# Single review sheet for comparing choices and size proofs.
board=Image.new('RGB',(1600,1480),PAPER)
label(board,(44,28),'NABO 单词护照 / 第 1 轮',34)
label(board,(44,78),'小学服装 A / V 领 + 砖红肩带',21)
label(board,(540,78),'小学服装 B / 圆领 + 深青肩带',21)
for idx,v in enumerate(('a','b')):
    key=Image.open(ART/f'design/primary_{v}.png').resize((384,480),LANC)
    board.paste(key,(44+idx*496,120))
    check=Image.open(ART/f'design/primary_{v}_check.png')
    # Keep pixel proofs at an integer 2x here; original file is the requested 4x.
    board.paste(check.resize((480,360),NEAR).crop((0,0,480,310)),(28+idx*496,618))
    label(board,(44+idx*496,940),'缩略检查 ×2；完整 ×4 另附',17)
label(board,(1054,78),'主页 / 最近邻 ×2',21)
board.paste(Image.open(ART/'preview/home_primary_2x.png'),(1054,120))
label(board,(1054,788),'道具 / 最终尺寸 ×4',21)
x=1070
for name in ('lamp','clock','pencilcase'):
    ob=props[name]
    board.paste(ob,(x,830),ob)
    x += ob.width+15
label(board,(44,1005),'豆豆 / 28 → 32 → 40 → 48 px，最近邻 ×4',24)
board.paste(Image.open(ART/'design/doudou_check.png').resize((1024,408),NEAR),(44,1050))
save(board,'preview/round1_contact.png')

existing_manifest_path=ART/'manifest.json'
existing_manifest=json.loads(existing_manifest_path.read_text()) if existing_manifest_path.exists() else {}
manifest={'round':1,'selected_primary':existing_manifest.get('selected_primary'),
 'selection':existing_manifest.get('selection'),'generator':'Codex built-in ImageGen',
 'start_candidates':metadata,'props':{},'companion_designs':{},'previews':{
 'home_primary.png':{'size':[240,320],'character_cap_y':66,'desk_y':176,'character_center_x':120,
 'clock_center':[34,52],'lamp_base':[29,176],'companion_base':[196,176],
 'note':'Art composition only; menu/status text is program-drawn.'}}}
for name,ob in props.items():
    manifest['props'][f'props/{name}.png']={'canvas_px':list(ob.size),'target_px':[ob.width//4,ob.height//4],
      'anchor_px':[ob.width//2,ob.height] if name!='clock' else [ob.width//2,ob.height//2],
      'anchor_type':'bottom_center' if name!='clock' else 'center'}
for name,ob in forms.items():
    visible=np.asarray(ob);colors=len(np.unique(visible[:,:,:3][visible[:,:,3]>0],axis=0))
    manifest['companion_designs'][f'design/doudou_{name}.png']={'canvas_px':list(ob.size),
      'target_px':[round(ob.width/4),ob.height//4],'target_height_px':ob.height//4,
      'anchor_px':[ob.width//2,ob.height],'anchor_type':'bottom_center','opaque_rgb_colors':colors}
for key,value in existing_manifest.items():
    if key not in manifest:
        manifest[key]=value
(ART/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
print('Wrote round-one PNGs and manifest under',ART)
print('Companion visible colors:',[x['opaque_rgb_colors'] for x in manifest['companion_designs'].values()])
