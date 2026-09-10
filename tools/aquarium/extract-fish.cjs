// Source silhouettes are masks for the supplied raster, not drawn artwork.
process.chdir(require('path').resolve(__dirname,'../..'));
const sharp = require(process.env.SHARP_MODULE || '/Users/amoghrijal/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
const fs = require('fs');
const path = require('path');
const source = 'design/reference/aquarium.png';
const sx=2498/1608, sy=1290/830;
const defs={
 'guppy-lower':{box:[710,596,123,77],flip:true,seed:[83,30],path:'M 6 25 L 8 23 L 14 22 L 22 21 L 30 21 L 40 23 L 49 26 L 58 28 L 63 27 L 59 25 L 59 22 L 61 18 L 64 11 L 63 8 L 67 8 L 71 9 L 77 12 L 80 15 L 82 18 L 81 20 L 85 20 L 96 20 L 103 22 L 108 24 L 111 24 L 114 25 L 116 28 L 116 31 L 112 35 L 108 38 L 102 41 L 95 44 L 90 45 L 88 49 L 85 52 L 82 54 L 78 56 L 76 53 L 73 53 L 71 50 L 72 46 L 72 44 L 64 43 L 60 43 L 57 46 L 53 50 L 49 54 L 45 59 L 40 64 L 35 67 L 30 68 L 26 66 L 23 64 L 21 61 L 18 56 L 17 52 L 17 49 L 19 47 L 18 43 L 15 39 L 12 33 L 9 29 Z'},
 'guppy-tilted':{box:[980,420,122,80],seed:[34,24]},
 guppy:{box:[711,368,106,62],flip:true,seed:[60,29]},
 molly:{box:[816,210,158,67],seed:[81,32]},
 turn:{box:[686,338,36,54],seed:[18,29]},
 green:{box:[1027,590,107,70],seed:[50,38],path:'M 5 34 L 7 32 L 13 30 L 21 27 L 29 25 L 36 18 L 41 14 L 46 13 L 50 11 L 54 8 L 60 7 L 61 10 L 64 13 L 65 16 L 69 19 L 71 24 L 77 23 L 83 20 L 89 18 L 94 17 L 99 17 L 102 19 L 100 23 L 98 26 L 97 31 L 96 33 L 96 42 L 94 47 L 92 54 L 88 62 L 85 66 L 81 66 L 76 63 L 70 59 L 63 52 L 55 45 L 52 47 L 54 50 L 53 54 L 50 56 L 50 60 L 47 61 L 45 58 L 41 59 L 36 57 L 35 54 L 33 54 L 32 51 L 26 50 L 18 47 L 13 44 L 9 42 L 7 38 L 5 37 Z'}
};
async function run(){
let sheet=[];
for (const [name,d] of Object.entries(defs)){
 const [x,y,w,h]=d.box;
 const crop={left:Math.round(x*sx),top:Math.round(y*sy),width:Math.round(w*sx),height:Math.round(h*sy)};
 const {data,info}=await sharp(source).extract(crop).ensureAlpha().raw().toBuffer({resolveWithObject:true});
 const W=info.width,H=info.height,N=W*H;
 let mask;
 if(d.path){
 const svg=`<svg width="${W}" height="${H}" viewBox="0 0 ${w} ${h}"><path fill="white" d="${d.path}"/></svg>`;
 const {data:md}=await sharp(Buffer.from(svg)).ensureAlpha().raw().toBuffer({resolveWithObject:true});mask=new Uint8Array(N);for(let i=0;i<N;i++)mask[i]=md[i*4+3];
 } else {
  let bg=new Uint8Array(N),q=new Int32Array(N),head=0,tail=0;
  const isWater=i=>{const p=i*4,r=data[p],g=data[p+1],b=data[p+2];return r<80&&g>145&&b>140&&b-g<100};
  const add=i=>{if(i<0||i>=N||bg[i]||!isWater(i))return;bg[i]=1;q[tail++]=i};
  for(let xx=0;xx<W;xx++){add(xx);add((H-1)*W+xx)}for(let yy=0;yy<H;yy++){add(yy*W);add(yy*W+W-1)}
  while(head<tail){let i=q[head++],xx=i%W;if(xx)add(i-1);if(xx<W-1)add(i+1);add(i-W);add(i+W)}
  // Keep the single fish component, including all pixels inside its outline.
  mask=new Uint8Array(N);head=0;tail=0;let seed=Math.round(d.seed[1]*sy)*W+Math.round(d.seed[0]*sx);
  const visit=i=>{if(i<0||i>=N||mask[i]||bg[i])return;mask[i]=255;q[tail++]=i};visit(seed);
  while(head<tail){let i=q[head++],xx=i%W;if(xx)visit(i-1);if(xx<W-1)visit(i+1);visit(i-W);visit(i+W)}
 }
 if(name==='green'||name==='guppy-lower'){
  let bg=new Uint8Array(N),q=new Int32Array(N),head=0,tail=0;
  const sand=i=>{const px=i%W/sx,py=Math.floor(i/W)/sy,p=i*4,r=data[p],g=data[p+1],b=data[p+2];return name==='green' ? px>38&&py>39&&r>115&&g>115&&r>g*.9&&b<160 : ((py>35&&px<90&&r>115&&g>115&&r>g*.9&&b<160)||(py>22&&py<35&&px>40&&px<75&&r>130&&g>115&&b<170))};
  const add=i=>{if(i<0||i>=N||bg[i]||!sand(i))return;bg[i]=1;q[tail++]=i};
  for(let i=0;i<N;i++)if(mask[i]===0)add(i);
  while(head<tail){let i=q[head++],xx=i%W;if(xx)add(i-1);if(xx<W-1)add(i+1);add(i-W);add(i+W)}
  for(let i=0;i<N;i++)if(bg[i])mask[i]=0;
 }
 const rgba=Buffer.from(data);for(let i=0;i<N;i++)rgba[i*4+3]=mask[i];
 let img=sharp(rgba,{raw:{width:W,height:H,channels:4}});if(d.flip)img=img.flop();
 await img.png().toFile(`assets/aquarium/fish-${name}.png`);
 await sharp(`assets/aquarium/fish-${name}.png`).greyscale().png().toFile(`assets/aquarium/fish-${name}-dead.png`);
 const maskRgba=Buffer.alloc(N*4,255);for(let i=0;i<N;i++)maskRgba[i*4+3]=mask[i];
 let mi=sharp(maskRgba,{raw:{width:W,height:H,channels:4}});if(d.flip)mi=mi.flop();await mi.png().toFile(`assets/aquarium/fish-${name}-mask.png`);
 const preview=await sharp(await sharp({create:{width:W,height:H,channels:3,background:'#04c7ef'}}).composite([{input:`assets/aquarium/fish-${name}.png`}]).png().toBuffer()).resize({width:W*4}).png().toBuffer();
 await sharp(preview).toFile(`work/aquarium-match/fish-preview-${name}.png`);
 let minX=W,minY=H,maxX=0,maxY=0;for(let yy=0;yy<H;yy++)for(let xx=0;xx<W;xx++)if(mask[yy*W+xx]>127){minX=Math.min(xx,minX);minY=Math.min(yy,minY);maxX=Math.max(xx,maxX);maxY=Math.max(yy,maxY)}
 const meta={name,path:`assets/aquarium/fish-${name}.png`,crop:[crop.left,crop.top,W,H],normalized:[x,y,w,h],normalizedCropCenter:[x+w/2,y+h/2],sourceCropCenter:[crop.left+W/2,crop.top+H/2],alphaBounds:d.flip?[W-maxX-1,minY,W-minX,maxY+1]:[minX,minY,maxX+1,maxY+1],flipped:!!d.flip};sheet.push(meta);console.log(name,JSON.stringify(meta));
}
fs.writeFileSync('work/aquarium-match/fish-assets.json',JSON.stringify(Object.fromEntries(sheet.map(d=>[d.name,d])),null,2));
}
run().catch(e=>{console.error(e);process.exitCode=1});
