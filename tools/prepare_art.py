#!/usr/bin/env python3
"""Prepare transparent individual game assets, preserving supplied art when found.
New illustrations are species-specific interpretations, not original-game assets.
"""
import json,math,random,re,zipfile,hashlib
from pathlib import Path
from PIL import Image,ImageDraw,ImageOps,ImageFilter
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets';OUT.mkdir(exist_ok=True)
SRC=Path('/mnt/data');RES=OUT/'fish';RES.mkdir(exist_ok=True)
manifest={}
def save(im,path,pivot=(.5,.5),logical=None,source='New game illustration'):
 path=OUT/path;path.parent.mkdir(parents=True,exist_ok=True);im.save(path)
 manifest[str(path.relative_to(OUT))]={'size':list(im.size),'pivot':list(pivot),'logical_size':logical or list(im.size),'source':source,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
def fish_art(s):
 name=s['id'];seed=int(hashlib.sha256(name.encode()).hexdigest()[:8],16);rng=random.Random(seed)
 im=Image.new('RGBA',(700,400));d=ImageDraw.Draw(im);ink='#183c57';body='#ffab43';fin='#ef704b';mark='#ffe29c';eye=(140,156);outline=7
 palettes={'guppy':('#a8d5de','#ff9949','#38a5d8'),'emberTetra':('#f68e37','#eb6a31','#ffd165'),'neonTetra':('#298ce0','#e34663','#57e7ed'),'molly':('#3e4559','#283247','#788b98'),'platy':('#ffa936','#ec6944','#ffdc82'),'zebraDanio':('#d7dde6','#949bb1','#25558e'),'greenChromis':('#66bacc','#4087b4','#abefc6'),'yellowTang':('#f6d23f','#e0af37','#fff279'),'bubbleEyeGoldfish':('#ffb751','#f48244','#ffe59d'),'koi':('#f3ede1','#d8e0dd','#e6664f'),'axolotl':('#f6bdcc','#e47c9f','#fff1e6'),'blackDiamondStingray':('#34394d','#282d44','#d4e5f0'),'platinumArowana':('#d9e7e9','#9eaac4','#ffffff'),'goldenArowana':('#ddb755','#b9833e','#ffe8a1'),'platinumBetta':('#e6eef2','#c7d6e3','#ffffff'),'mandarinDragonet':('#229bb3','#df9a47','#fb703a'),'flameAngelfish':('#ef8136','#f1ab37','#3c4268'),'ocellarisClownfish':('#f28f35','#de6b36','#fff4df'),'discus':('#e57663','#d64754','#7ccfdb'),'queenAngelfish':('#489bb9','#dfb84a','#eedc6b'),'moorishIdol':('#f3e4ac','#eee9df','#2a3e51')}
 body,fin,mark=palettes.get(name,(rng.choice(['#8db4cc','#f59d72','#d9a878','#88bfbb','#d9b6d0']),rng.choice(['#e88669','#789ab5','#b88dcc']),rng.choice(['#fbde8b','#689fd3','#dcdde9'])))
 deep=any(x in name.lower() for x in ['angel','tang','discus','trigger','idol']);slim=any(x in name.lower() for x in ['tetra','danio','rasbora','wrasse','arowana']);arowana='Arowana' in name
 left=92;right=505 if arowana else 465;top=115 if slim else 75 if deep else 101;bottom=255 if slim else 313 if deep else 287;mid=(top+bottom)//2
 if name=='guppy':right=384;top=145;bottom=238;mid=190
 # Separate articulated fins are part of the same transparent deformable texture.
 d.polygon([(right-40,mid),(630,82 if name=='guppy' else 118),(652,mid),(630,313 if name=='guppy' else 276)],fill=fin,outline=ink,width=outline)
 for i in range(1,7):
  yy=120+i*22;d.line([(right-27,mid),(630,yy)],fill=mark,width=3)
 d.polygon([(210,top+32),(290,top-45),(384,top-9),(440,top+45)],fill=fin,outline=ink,width=outline)
 d.polygon([(280,bottom-30),(398,bottom+37),(457,bottom-42)],fill=fin,outline=ink,width=outline)
 d.ellipse((left,top,right+45,bottom),fill=body,outline=ink,width=outline)
 d.ellipse((left+36,mid+14,right-36,bottom-13),fill=mark)
 d.arc((left,top,right+45,bottom),0,355,fill=ink,width=outline)
 if name in ['neonTetra','zebraDanio','sixlineWrasse']:
  for j in range(1 if name=='neonTetra' else 4):d.line([(left+74,mid-20+j*20),(right+15,mid-15+j*15)],fill=mark if name!='zebraDanio' else '#245579',width=17 if name=='neonTetra' else 9)
 if any(x in name.lower() for x in ['clown','idol','cardinal','trigger','hamlet','pajama','angel']):
  for xx in (250,345,432):d.polygon([(xx-18,top+8),(xx+7,top+9),(xx+26,bottom-17),(xx-3,bottom-10)],fill=mark,outline=ink,width=3)
 if any(x in name.lower() for x in ['koi','mandarin','discus','rainbow','gourami']):
  for i in range(18):
   x=rng.randrange(220,max(221,right));y=rng.randrange(top+30,bottom-25);r=rng.randrange(7,18);d.ellipse((x-r,y-r,x+r,y+r),fill=fin if i%2 else mark)
 if arowana:
  for y in range(top+25,bottom-15,26):
   for x in range(225,right,32):d.arc((x,y,x+33,y+29),0,180,fill=fin,width=4)
 if name=='bubbleEyeGoldfish':d.ellipse((75,190,230,330),fill='#ffcc7d',outline=ink,width=6);eye=(147,173)
 if 'cowfish' in name.lower():d.polygon([(121,top+30),(115,top-33),(164,top+16)],fill=mark,outline=ink,width=6)
 if name=='axolotl':
  for y in (130,173,218):
   d.line([(158,y),(63,y-26),(40,y-38)],fill=ink,width=24);d.line([(158,y),(63,y-26),(40,y-38)],fill=fin,width=16)
  for x in (248,403):d.line([(x,240),(x-22,319),(x+15,325)],fill=ink,width=23);d.line([(x,240),(x-22,319),(x+15,325)],fill=body,width=14)
 if name=='blackDiamondStingray':
  im=Image.new('RGBA',(700,400));d=ImageDraw.Draw(im);d.line([(414,208),(528,235),(646,298)],fill=ink,width=19);d.polygon([(96,188),(279,57),(470,109),(533,194),(458,300),(268,329)],fill=body,outline=ink,width=8)
  for i in range(32):x=rng.randrange(175,445);y=rng.randrange(117,282);d.ellipse((x-5,y-5,x+5,y+5),fill=mark)
  eye=(156,180);mid=203
 d.ellipse((eye[0]-37,eye[1]-42,eye[0]+39,eye[1]+40),fill='#fff8e6',outline=ink,width=7)
 d.ellipse((eye[0]-25,eye[1]-30,eye[0]+31,eye[1]+31),fill='#ebc45b',outline=ink,width=4)
 d.ellipse((eye[0]-17,eye[1]-19,eye[0]+21,eye[1]+22),fill='#273748')
 d.ellipse((eye[0]-15,eye[1]-20,eye[0]-1,eye[1]-4),fill='white');d.ellipse((eye[0]+10,eye[1]+10,eye[0]+16,eye[1]+16),fill='white')
 d.arc((88,mid+14,133,mid+42),5,167,fill=ink,width=6)
 d.ellipse((265,mid+7,327,mid+63),fill=fin,outline=ink,width=5)
 bbox=im.getbbox();im=im.crop(bbox);im.thumbnail((640,340),Image.Resampling.LANCZOS);pad=Image.new('RGBA',(im.width+12,im.height+12));pad.alpha_composite(im,(6,6));return pad

# Reuse supplied/prepared named assets without reading or copying any source repository.
candidates={}
for p in SRC.rglob('*.png'):
 if ROOT in p.parents or 'node_modules' in p.parts or 'reference' in p.parts or 'ChatGPT Image' in p.name:continue
 candidates.setdefault(re.sub(r'[^a-z0-9]','',p.stem.lower()),[]).append(p)
content=json.loads((OUT/'content.json').read_text())
for s in content['species']:
 key=re.sub(r'[^a-z0-9]','',s['id'].lower());matches=candidates.get(key,[])
 im=None;source='New species-specific illustration based on supplied visual direction'
 for p in matches:
  try:
   q=Image.open(p).convert('RGBA');alpha=q.getchannel('A');bbox=alpha.getbbox()
   if bbox and alpha.getextrema()[0]<255:q=q.crop(bbox);q.thumbnail((640,340),Image.Resampling.LANCZOS);im=Image.new('RGBA',(q.width+12,q.height+12));im.alpha_composite(q,(6,6));source='Supplied/prepared art: '+p.name;break
  except OSError:pass
 if im is None:im=fish_art(s)
 save(im,Path('fish')/(s['id']+'.png'),logical=[s['length'],s['length']*im.height/im.width],source=source)
 gray=ImageOps.grayscale(im).convert('RGBA');gray.putalpha(im.getchannel('A'));save(gray,Path('fish')/(s['id']+'-dead.png'),logical=[s['length'],s['length']*im.height/im.width],source='Desaturated variant of '+s['id'])

# Select an already prepared isolated background where its name is explicit.
back=None;source='New illustrated aquarium backdrop'
for key in ['background','aquariumbackground','aquariumbackdrop','tankbackground']:
 for p in candidates.get(key,[]):
  try:
   im=Image.open(p).convert('RGB')
   if im.width>800 and im.width/im.height>1.3:back=im;source='Prepared backdrop: '+p.name;break
  except OSError:pass
 if back is not None:break
if back is None:
 w,h=1632,952;back=Image.new('RGB',(w,h));pixels=back.load()
 for y in range(h):
  t=y/h;r=int(66+36*t);g=int(190-25*t);b=int(216-8*t)
  for x in range(w):pixels[x,y]=(r,g,b)
 overlay=Image.new('RGBA',(w,h));d=ImageDraw.Draw(overlay)
 for x in (200,490,770,1030,1290):d.polygon([(x,0),(x+28,0),(x+430,750),(x+70,750)],fill=(220,255,232,22))
 d.polygon([(0,724),(220,729),(500,715),(840,724),(1180,702),(1632,736),(1632,952),(0,952)],fill='#ead397')
 d.polygon([(0,839),(1632,839),(1632,952),(0,952)],fill='#c8b184')
 d.line([(0,839),(1632,839)],fill='#947f75',width=9)
 for x,y,ww,hh in [(0,390,160,370),(81,541,194,223),(1500,490,180,261),(1400,603,167,160)]:
  d.rounded_rectangle((x,y,x+ww,y+hh),radius=58,fill='#788b9a',outline='#607b90',width=7);d.ellipse((x+13,y+14,x+ww-26,y+82),fill='#a8b2b0')
 d.arc((1040,377,1440,809),180,360,fill='#536f85',width=94);d.arc((1040,363,1440,795),180,360,fill='#859caa',width=78)
 d.line([(1080,580),(1080,744)],fill='#859caa',width=78);d.line([(1402,580),(1402,744)],fill='#859caa',width=78)
 for x,base in [(45,754),(187,761),(280,751),(1460,766),(1570,767)]:
  for k in range(5):
   col=['#2d9797','#44aba2','#5aaca7'][k%3];points=[(x+k*15,base),(x+k*15-12,base-80-k*13),(x+k*15+15,base-142-k*10),(x+k*15,base-204+k*7)];d.line(points,fill=col,width=15)
 for x,y in [(229,747),(374,757),(1468,744)]:
  for k in range(6):d.line([(x,y),(x-40+k*13,y-67-17*(k%3))],fill='#d798b1',width=16)
 back=Image.alpha_composite(back.convert('RGBA'),overlay).convert('RGB')
back.thumbnail((2176,1270),Image.Resampling.LANCZOS);save(back,Path('background.png'),source=source)
ink='#264d67'
def icon(name):
 im=Image.new('RGBA',(192,192));d=ImageDraw.Draw(im)
 if name=='coin':d.ellipse((20,20,172,172),fill='#f2b43e',outline='#996329',width=10);d.ellipse((37,34,157,155),fill='#ffd971',outline='#fff0af',width=6);d.line((88,57,109,57,109,137,87,137),fill='#ba7b28',width=13)
 elif name=='pearl':d.polygon([(96,16),(157,45),(173,108),(113,173),(39,150),(19,74)],fill='#b285e2',outline='#624b9a',width=9);d.polygon([(96,16),(107,84),(157,45)],fill='#e2c5f4');d.polygon([(19,74),(107,84),(39,150)],fill='#c9a6ec');d.polygon([(107,84),(173,108),(113,173)],fill='#9474cd')
 elif name=='egg':d.ellipse((43,28,150,172),fill='#fbf1cf',outline=ink,width=8);d.ellipse((63,59,80,86),fill='#b5d8b7');d.ellipse((105,121,129,146),fill='#b5d8b7');d.arc((56,40,129,142),195,280,fill='white',width=8)
 elif name=='food':
  d.rounded_rectangle((49,26,145,142),radius=24,fill='#edbd75',outline=ink,width=9);d.rounded_rectangle((42,111,151,157),radius=9,fill='#43868f',outline=ink,width=8);d.rectangle((54,58,141,106),fill='#faf0cc');d.ellipse((78,71,116,95),fill='#e8904f');d.polygon([(112,82),(132,73),(132,96)],fill='#e8904f')
 elif name=='net':
  d.line((95,83,166,178),fill=ink,width=24);d.line((95,83,166,178),fill='#bc9767',width=13);d.ellipse((15,12,137,124),fill='#a0d9dc',outline=ink,width=11)
  for x in range(39,130,19):d.line([(x,24),(x-10,111)],fill='#e5f3e8',width=4)
  for y in range(36,114,19):d.line([(30,y),(124,y-9)],fill='#e5f3e8',width=4)
 elif name=='tank':
  d.rounded_rectangle((18,37,174,151),radius=17,fill='#bce8e5',outline=ink,width=9);d.rectangle((28,63,164,127),fill='#65c3d2');d.rectangle((28,127,164,143),fill='#e2ca8e');d.ellipse((65,81,112,112),fill='#ef9953');d.polygon([(110,96),(136,82),(136,111)],fill='#ef9953')
 elif name=='xp':
  p=[]
  for k in range(10):a=-math.pi/2+k*math.pi/5;r=81 if k%2==0 else 40;p.append((96+math.cos(a)*r,96+math.sin(a)*r))
  d.polygon(p,fill='#f5d476',outline=ink,width=7)
 else:
  d.rounded_rectangle((28,46,164,154),radius=19,fill='#deb78c',outline=ink,width=9);d.polygon([(28,47),(96,15),(164,47),(97,85)],fill='#efd4a5',outline=ink,width=7);d.line((97,85,97,153),fill=ink,width=6)
 return im
for name in ['coin','pearl','egg','food','net','tank','xp','inventory']:
 save(icon(name),Path('ui')/(name+'.png'),pivot=(.5,.8) if name=='food' else (.395,.355) if name=='net' else (.5,.5),logical=[64,64])
for name in ['seaweed','coral','shell','arch','chest']:
 im=Image.new('RGBA',(280,240));d=ImageDraw.Draw(im)
 if name in ['seaweed','coral']:
  for k in range(7):x=90+k*15;d.line([(140,222),(x,167),(x-15,98),(x+12,30+k%3*24)],fill=ink,width=21);d.line([(140,222),(x,167),(x-15,98),(x+12,30+k%3*24)],fill='#62b59b' if name=='seaweed' else '#dc93b2',width=13)
 elif name=='arch':d.arc((33,14,251,272),180,360,fill=ink,width=67);d.arc((33,12,251,270),180,360,fill='#a1b1b7',width=48);d.rounded_rectangle((26,120,81,226),16,fill='#a1b1b7',outline=ink,width=7);d.rounded_rectangle((204,120,259,226),16,fill='#a1b1b7',outline=ink,width=7)
 elif name=='shell':
  d.pieslice((32,44,253,235),180,360,fill='#e3b3c5',outline=ink,width=8)
  for k in range(7):a=math.pi+k*math.pi/6;d.line([(143,157),(143+math.cos(a)*100,157+math.sin(a)*97)],fill='#b5779e',width=5)
  d.ellipse((100,126,184,212),fill='#eef1e0',outline=ink,width=7)
 else:d.rounded_rectangle((27,84,251,218),18,fill='#b88860',outline=ink,width=8);d.rounded_rectangle((27,54,251,143),28,fill='#cf9f6e',outline=ink,width=8);d.rectangle((75,61,90,211),fill='#ebca70');d.rectangle((186,61,201,211),fill='#ebca70');d.rounded_rectangle((124,115,162,163),8,fill='#eccb6c',outline=ink,width=5)
 save(im,Path('decor')/(name+'.png'),pivot=(.5,1),logical=[100,90])
(OUT/'manifest.json').write_text(json.dumps({'version':1,'assets':manifest},indent=2))
print(f'Prepared {len(manifest)} individually addressable assets')
