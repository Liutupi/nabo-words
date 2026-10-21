"""Validate the round-four art delivery; no firmware or device claims are made."""
from pathlib import Path
import hashlib
import json
import numpy as np
from PIL import Image

ART = Path(__file__).resolve().parents[1]
ROOT = ART.parent
WORK = ART / 'design/round4'
m = json.loads((ART / 'manifest.json').read_text())
species = ['doudou', 'paopao', 'xingxing', 'huohuo', 'langlang', 'jiguang']
forms = {'egg': 28, 'baby': 32, 'teen': 40, 'final': 48}
props = {'pencilcase': (34,12), 'clock': (28,28), 'lamp': (24,44),
         'plant': (16,24), 'globe': (24,30), 'shelf': (66,8),
         'cert': (36,28), 'phones': (24,14), 'laptop': (56,30),
         'mug': (18,16), 'map': (60,38), 'trophy': (16,30)}
toys = ['duck','rocket','cactus','snowglobe','cube','dino','plane','hourglass','shell','windmill']
ui = ['coin','flame','flame_dim','freeze']
expected = {f'companion/{s}/{f}.png': (None,h) for s in species for f,h in forms.items()}
expected.update({f'props/{n}.png': v for n,v in props.items()})
expected.update({f'toys/primary/{n}.png': (12,12) for n in toys})
expected.update({f'ui/{n}.png': (12,12) for n in ui})
records = {}
ids = set()
for rel, target in expected.items():
    path = ART / rel
    im = Image.open(path)
    ar = np.asarray(im)
    group = rel.split('/')[0]
    info = m[group][rel]
    assert im.mode == 'RGBA', rel
    assert im.width % 4 == im.height % 4 == 0, rel
    actual = (im.width//4, im.height//4)
    assert all(want is None or want == got for want,got in zip(target,actual)), rel
    assert list(actual) == info['target_px'] and list(im.size) == info['canvas_px'], rel
    assert set(np.unique(ar[:,:,3])) == {0,255}, rel
    colors = len(np.unique(ar[:,:,:3][ar[:,:,3]>0], axis=0))
    assert colors <= 32 and colors == info['opaque_rgb_colors'], rel
    assert info['id'] == info['name'] and info['id'] not in ids, rel
    ids.add(info['id'])
    anchor = [im.width/2,im.height/2] if info['anchor_type']=='center' else [im.width/2,im.height]
    assert info['anchor_px'] == anchor and info['target_anchor_px'] == [v/4 for v in anchor], rel
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    assert digest == info['sha256'], rel
    if group in ['toys','ui']:
        assert np.array_equal(ar, np.repeat(np.repeat(ar[::4,::4],4,0),4,1)), rel
    records[rel] = {'target_px': list(actual), 'colors': colors, 'sha256': digest}
assert np.array_equal(np.array(Image.open(ART/'ui/flame.png'))[:,:,3],
                      np.array(Image.open(ART/'ui/flame_dim.png'))[:,:,3])
baseline = json.loads((WORK/'baseline.json').read_text())
for rel, digest in baseline['protected_existing_art'].items():
    assert hashlib.sha256((ROOT/rel).read_bytes()).hexdigest() == digest, rel
jobs = json.loads((WORK/'prompts.json').read_text())['jobs']
assert len(jobs) == 27
for job in jobs:
    source = WORK/'source'/(job['id']+'.png')
    assert source.exists() and (WORK/'receipts'/(job['id']+'.json')).exists(), job['id']
for stage,candidate in [('primary','a'),('junior','b'),('senior','b')]:
    assert (ART/f'start/{stage}.png').read_bytes() == (ART/f'design/{stage}_{candidate}.png').read_bytes()
primary = np.array(Image.open(ART/'start/primary.png'))
for stage in ['junior','senior']:
    ar = np.array(Image.open(ART/f'start/{stage}.png'))
    assert np.array_equal(primary[:831], ar[:831])
    assert np.array_equal(primary[1070:], ar[1070:])
home = Image.open(ART/'preview/home_primary.png')
large = Image.open(ART/'preview/home_primary_2x.png')
assert home.size == (240,320) and large.size == (480,640)
assert np.array_equal(np.array(home.resize(large.size,Image.Resampling.NEAREST)),np.array(large))
assert m['round4']['exported_static_assets'] == 50 and not m['round4']['missing_sources']
result = {'status':'PASS', 'canonical_static_assets':len(records),
          'groups':{'companion':24,'props':12,'toys':10,'ui':4},
          'native_source_images':len(jobs),
          'protected_existing_files_unchanged':len(baseline['protected_existing_art']),
          'selection':'primary A; junior B; senior B', 'assets':records,
          'unverified':['final animation loop seams and happy/sad readability at 48 px',
                        'runtime packing, RGB565 display and physical device appearance',
                        'owner approval of round-four designs']}
(WORK/'validation.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
print(json.dumps({k:v for k,v in result.items() if k!='assets'},ensure_ascii=False))
