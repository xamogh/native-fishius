// Prepare reusable native UI textures from the supplied reference and imagegen clean plate.
const fs = require('node:fs/promises');
const path = require('node:path');
const crypto = require('node:crypto');
const sharp = require(process.env.SHARP_MODULE || '/Users/amoghrijal/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
const root = path.resolve(__dirname, '..');
const out = path.join(root, 'assets/tanks');
const W = 790, H = 823;
async function main() {
 await fs.mkdir(out, {recursive:true});
 const raw = async file => (await sharp(file).resize(W,H,{fit:'fill'}).ensureAlpha().raw().toBuffer());
 const original = await raw(path.join(root,'work/tank-popup/reference-panel.png'));
 const generated = await raw(path.join(root,'work/tank-popup/generated-clean-plate.png'));
 const clean = Buffer.from(original);
 // Use generated replacement pixels only inside editable labels. Match the
 // adjacent original material along all four edges to avoid rectangular seams.
 function erase(x,y,w,h) {
  const stride=w+2;
  for(let c=0;c<3;c++) {
   const correction=new Float32Array((w+2)*(h+2));
   for(let yy=0;yy<h+2;yy++)for(let xx=0;xx<w+2;xx++) {
    const p=((y+yy-1)*W+x+xx-1)*4+c;
    correction[yy*stride+xx]=(xx===0||yy===0||xx===w+1||yy===h+1)?original[p]:generated[p];
   }
   // Smooth the generated fill against the source boundary. The illustrated
   // rim, thumbnails, icons and button bevels are never changed.
   for(let iteration=0;iteration<1200;iteration++)for(let yy=1;yy<=h;yy++)for(let xx=1;xx<=w;xx++) {
    const p=yy*stride+xx;correction[p]=(correction[p-1]+correction[p+1]+correction[p-stride]+correction[p+stride])*.25;
   }
   for(let yy=0;yy<h;yy++)for(let xx=0;xx<w;xx++) {
    const p=((y+yy)*W+x+xx)*4+c;
    clean[p]=Math.max(0,Math.min(255,Math.round(correction[(yy+1)*stride+xx+1])));
   }
  }
 }
 // Row labels, current badge, upgrade heading, capacities and button values.
 [[218,112,191,38],[270,162,147,37],[525,110,134,37],[535,148,111,31],
  [489,192,90,35],[672,192,43,37],
  [218,293,204,40],[250,336,189,32],[522,316,64,36],[674,316,54,36],
  [218,418,203,40],[250,460,190,32],[218,491,185,29],[518,444,73,37],[673,444,55,37],
  [218,550,204,40],[250,593,190,31],[218,625,185,29],[516,577,78,38],[666,577,66,38],
  [218,683,204,40],[250,725,190,32],[218,758,185,30],[516,710,78,39],[666,710,66,39]].forEach(b=>erase(...b));
 const save=async(name,data,w,h)=>sharp(data,{raw:{width:w,height:h,channels:4}}).png().toFile(path.join(out,name+'.png'));
 // Extract supplied silhouettes. The mask only removes surrounding scene or
 // row pixels. It does not draw replacement artwork.
 async function extract(name,src,box,polygon) {
  const [x,y,w,h]=box,data=Buffer.alloc(w*h*4);
  const inside=(px,py)=>{let hit=false;for(let i=0,j=polygon.length-1;i<polygon.length;j=i++){const [ax,ay]=polygon[i],[bx,by]=polygon[j];if((ay>py)!=(by>py)&&px<(bx-ax)*(py-ay)/(by-ay)+ax)hit=!hit;}return hit;};
  for(let yy=0;yy<h;yy++)for(let xx=0;xx<w;xx++){
   const p=(yy*w+xx)*4,s=((y+yy)*W+x+xx)*4;original.copy(data,p,s,s+4);
   if(src!==original)src.copy(data,p,s,s+4);
   let covered=0;for(let sy=0;sy<4;sy++)for(let sx=0;sx<4;sx++)covered+=inside(xx+(sx+.5)/4,yy+(sy+.5)/4);
   data[p+3]=Math.round(255*covered/16);
  }
  await save(name,data,w,h);
 }
 // Frame uses the original header and rim; the clean generated footer repairs
 // the navigation button that occludes the source's lower left corner.
 const frame=Buffer.from(generated);
 for(let y=0;y<795;y++)for(let x=0;x<W;x++) {
  if(y<95||x<25||x>768)original.copy(frame,(y*W+x)*4,(y*W+x)*4,(y*W+x)*4+4);
 }
 // Stretch a real empty inter-row strip through the body, behind the cards.
 for(let y=95;y<795;y++)for(let x=25;x<769;x++) {
  const source=(268*W+x)*4;generated.copy(frame,(y*W+x)*4,source,source+4);
 }
 // The generated exterior is neutral checkerboard. Flood only its connected
 // exterior, retaining the illustrated blue rim and a real alpha channel.
 const visited=new Uint8Array(W*H),queue=[];
 const add=p=>{if(visited[p])return;visited[p]=1;const o=p*4;if(Math.max(generated[o],generated[o+1],generated[o+2])-Math.min(generated[o],generated[o+1],generated[o+2])<45)queue.push(p);};
 for(let x=0;x<W;x++){add(x);add((H-1)*W+x);}for(let y=0;y<H;y++){add(y*W);add(y*W+W-1);}
 for(let i=0;i<queue.length;i++){const p=queue[i],x=p%W,y=Math.floor(p/W);frame[p*4+3]=0;if(x>0)add(p-1);if(x+1<W)add(p+1);if(y>0)add(p-W);if(y+1<H)add(p+W);}
 await save('panel',frame,W,H);
 // Whole-row artwork keeps reference tank illustrations, button bevels and icons.
 const rowPoly=(w,h)=>[[29,1],[w-27,1],[w-12,5],[w-3,17],[w-1,30],[w-1,h-28],[w-5,h-13],[w-17,h-3],[w-30,h-1],[29,h-1],[14,h-5],[3,h-17],[1,h-29],[1,29],[5,14],[15,5]];
 await extract('active-1',clean,[24,92,744,176],rowPoly(744,176));
 const tops=[272,401,534,667],heights=[124,130,130,131];
 for(let i=0;i<4;i++)await extract('row-'+(i+2),clean,[26,tops[i],740,heights[i]],rowPoly(740,heights[i]));
 // Blank generated rows support ownership and active-tank changes.
 await extract('active-blank',generated,[24,92,744,176],rowPoly(744,176));
 await extract('row-blank',generated,[26,272,740,124],rowPoly(740,124));
 await extract('locked-blank',generated,[26,401,740,130],rowPoly(740,130));
 // Exact source currency and status icons for the blank, state-dependent rows.
 await extract('coin',original,[446,190,43,45],[[18,2],[29,2],[39,10],[42,23],[37,37],[25,43],[12,40],[3,30],[2,15],[9,6]]);
 await extract('pearl',original,[617,190,42,44],[[17,1],[30,4],[39,14],[41,27],[34,39],[23,43],[11,40],[3,32],[1,18],[6,7]]);
 await extract('thumbnail-lock',original,[49,460,41,51],[[12,2],[25,2],[33,10],[33,19],[39,24],[39,49],[2,49],[2,24],[6,20],[6,11]]);
 await extract('lock',original,[222,336,27,31],[[8,1],[18,1],[23,7],[23,12],[26,15],[26,29],[1,29],[1,15],[5,11],[5,6]]);
 await extract('arrow',original,[575,151,28,21],[[2,7],[15,7],[10,2],[17,1],[27,9],[27,13],[16,21],[10,18],[15,14],[2,14]]);
 await extract('fish',original,[221,166,47,30],[[2,6],[10,10],[22,6],[21,1],[31,5],[32,9],[42,12],[46,19],[37,24],[23,23],[16,28],[13,22],[7,22],[1,26],[4,17]]);
 const manifestPath=path.join(root,'assets/manifest.json');
 const manifest=JSON.parse(await fs.readFile(manifestPath,'utf8'));
 for(const name of await fs.readdir(out))if(name.endsWith('.png')){
  const file='tanks/'+name,bytes=await fs.readFile(path.join(out,name)),meta=await sharp(bytes).metadata();
  manifest.assets[file]={source:'User screenshot with built-in imagegen clean plate; see work/tank-popup/provenance.json',size:[meta.width,meta.height],sha256:crypto.createHash('sha256').update(bytes).digest('hex')};
 }
 await fs.writeFile(manifestPath,JSON.stringify(manifest,null,2)+'\n');
 console.log('Prepared tank popup frame, five rows, state variants and status icons.');
}
main().catch(e=>{console.error(e);process.exitCode=1;});
