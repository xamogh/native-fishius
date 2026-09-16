// Export source artwork as separate native controls. ImageGen supplies the
// concealed clean backing; visible source pixels keep their original RGB.
const fs = require('node:fs/promises');
const path = require('node:path');
const crypto = require('node:crypto');
const sharp = require(process.env.SHARP_MODULE || '/Users/amoghrijal/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
const root = path.resolve(__dirname, '..');
const work = path.join(root, 'work/currency-shop-reference');
const out = path.join(root, 'assets/currency-shop');
const source = path.join(work, 'reference.png');
const panel = [270, 91, 1210, 854];
// Source screenshot coordinates, including the original card edge and glow.
const tiles = {
 'coin-small': [330,282,354,250],
 'coin-medium': [690,282,346,250],
 'coin-large': [1042,282,372,250],
 'pearl-small': [316,535,265,280],
 'pearl-medium': [588,535,277,280],
 'pearl-large': [869,535,278,280],
 'starter': [1150,535,291,280],
 'tab-coins': [463,209,282,70],
 'tab-pearls': [747,209,265,70],
 'tab-bundles': [1017,209,283,70],
 'earn': [597,816,562,87],
 'close': [1377,140,96,95]
};
const offers = [
 {id:'coin-small',name:'Small Coin Pack',coins:2500,pearls:0,price:'$0.99'},
 {id:'coin-medium',name:'Medium Coin Pack',coins:12000,pearls:0,price:'$3.99'},
 {id:'coin-large',name:'Large Coin Pack',coins:35000,pearls:0,price:'$9.99'},
 {id:'pearl-small',name:'Small Pearl Pack',coins:0,pearls:15,price:'$1.99'},
 {id:'pearl-medium',name:'Medium Pearl Pack',coins:0,pearls:40,price:'$4.99'},
 {id:'pearl-large',name:'Large Pearl Pack',coins:0,pearls:90,price:'$9.99'},
 {id:'starter',name:'Starter Bundle',coins:25000,pearls:30,price:'$4.99'}
];
function inside(x,y,polygon) {
 let hit=false;
 for(let i=0,j=polygon.length-1;i<polygon.length;j=i++){
  const [ax,ay]=polygon[i],[bx,by]=polygon[j];
  if((ay>y)!=(by>y)&&x<(bx-ax)*(y-ay)/(by-ay)+ax)hit=!hit;
 }
 return hit;
}
async function main(){
 await fs.mkdir(out,{recursive:true});
 const {data:original,info}=await sharp(source).ensureAlpha().raw().toBuffer({resolveWithObject:true});
 const [px,py,w,h]=panel,rgba=Buffer.alloc(w*h*4);
 // Trace the supplied frame silhouette. These coordinates mask existing art;
 // they do not draw a replacement frame, icon, or illustration.
 const silhouette=[
  [272,280],[272,253],[279,219],[294,192],[321,174],[352,163],
  [462,162],[468,154],[480,150],[480,140],[490,139],[502,153],
  [509,143],[510,127],[520,122],[530,129],[533,146],[540,142],
  [541,121],[554,118],[568,130],[572,122],[574,110],[591,105],
  [626,100],[640,96],[656,99],[657,111],[648,136],[676,143],
  [680,154],[687,164],[691,132],[704,112],[731,102],[803,97],
  [972,96],[1102,101],[1145,109],[1164,121],[1176,140],
  [1181,132],[1192,131],[1198,119],[1209,116],[1219,129],
  [1228,128],[1235,131],[1247,128],[1256,135],[1259,147],
  [1249,160],[1387,161],[1401,147],[1420,141],[1441,145],
  [1458,158],[1468,176],[1470,196],[1465,212],[1454,225],
  [1469,254],[1476,287],[1476,791],[1472,832],[1459,867],
  [1437,892],[1437,914],[1433,932],[1420,942],[1394,944],
  [1376,938],[1360,930],[1339,930],[1321,920],[1301,919],
  [1279,913],[450,913],[435,922],[408,929],[386,928],[366,921],
  [348,921],[335,910],[316,905],[301,888],[288,864],[280,834]
 ];
 const generatedPath=path.join(work,'generated-clean-panel.png');
 const clean=await sharp(generatedPath).resize(w,830,{fit:'fill'}).ensureAlpha().raw().toBuffer();
 for(let y=0;y<h;y++)for(let x=0;x<w;x++){
  const i=(y*w+x)*4,s=((py+y)*info.width+px+x)*4;
  original.copy(rgba,i,s,s+4);
  let covered=0;
  for(let sy=0;sy<4;sy++)for(let sx=0;sx<4;sx++)covered+=inside(px+x+(sx+.5)/4,py+y+(sy+.5)/4,silhouette);
  rgba[i+3]=Math.round(covered*255/16);
 }
 // Recover the small leaf silhouettes from their actual source colors. Flood
 // only the connected cyan water outside the two header ornaments.
 for(const [x,y,bw,bh] of [[461,95,228,68],[1168,111,112,52]]){
  const seen=new Uint8Array(bw*bh),queue=[];
  const add=(xx,yy)=>{
   if(xx<0||yy<0||xx>=bw||yy>=bh)return;
   const j=yy*bw+xx;if(seen[j])return;seen[j]=1;
   const s=((y+yy)*info.width+x+xx)*4;
   const [r,g,b]=original.subarray(s,s+3);
   if(b>r*1.25&&g>r*1.25&&b>g*.90)queue.push(j);
  };
  for(let xx=0;xx<bw;xx++){add(xx,0);add(xx,bh-1);}
  for(let yy=0;yy<bh;yy++){add(0,yy);add(bw-1,yy);}
  const water=new Uint8Array(bw*bh);
  for(let n=0;n<queue.length;n++){
   const j=queue[n],xx=j%bw,yy=Math.floor(j/bw);water[j]=1;
   add(xx-1,yy);add(xx+1,yy);add(xx,yy-1);add(xx,yy+1);
  }
  // Close small breaks between pale specular highlights and green leaf
  // bodies. Cyan highlights belong to the leaves, even when they share the
  // surrounding water's hue. All restored RGB still comes from the source.
  const dilated=new Uint8Array(bw*bh),closed=new Uint8Array(bw*bh),radius=6;
  for(let yy=0;yy<bh;yy++)for(let xx=0;xx<bw;xx++){
   for(let dy=-radius;dy<=radius&&!dilated[yy*bw+xx];dy++)for(let dx=-radius;dx<=radius;dx++){
    if(dx*dx+dy*dy>radius*radius)continue;
    const ax=xx+dx,ay=yy+dy;
    if(ax>=0&&ay>=0&&ax<bw&&ay<bh&&!water[ay*bw+ax]){dilated[yy*bw+xx]=1;break;}
   }
  }
  for(let yy=0;yy<bh;yy++)for(let xx=0;xx<bw;xx++){
   let keep=true;
   for(let dy=-radius;dy<=radius&&keep;dy++)for(let dx=-radius;dx<=radius;dx++){
    if(dx*dx+dy*dy>radius*radius)continue;
    const ax=xx+dx,ay=yy+dy;
    if(ax>=0&&ay>=0&&ax<bw&&ay<bh&&!dilated[ay*bw+ax]){keep=false;break;}
   }
   closed[yy*bw+xx]=keep?1:0;
  }
  for(let yy=0;yy<bh;yy++)for(let xx=0;xx<bw;xx++){
   const i=((y+yy-py)*w+x+xx-px)*4;
   rgba[i+3]=closed[yy*bw+xx]?255:0;
  }
 }
 // These two mint highlights share the water color. Preserve their complete
 // measured leaf outlines instead of treating that highlight as empty water.
 for(const leaf of [
  [[508,133],[510,127],[516,124],[523,126],[529,132],[533,142],[537,153],[537,163],[532,169],[523,164],[515,156],[510,145]],
  [[1228,135],[1233,130],[1240,128],[1247,132],[1250,140],[1248,150],[1242,164],[1234,175],[1227,178],[1224,167],[1225,150]]
 ]){
  const xs=leaf.map(p=>p[0]),ys=leaf.map(p=>p[1]);
  for(let y=Math.min(...ys);y<=Math.max(...ys);y++)for(let x=Math.min(...xs);x<=Math.max(...xs);x++)
   if(inside(x+.5,y+.5,leaf))rgba[((y-py)*w+x-px)*4+3]=255;
 }
 // Remove tiny detached water highlights introduced by the exterior matte.
 const visited=new Uint8Array(w*h);
 for(let start=0;start<w*h;start++){
  if(visited[start]||rgba[start*4+3]<128)continue;
  const component=[start];visited[start]=1;
  for(let k=0;k<component.length;k++){
   const i=component[k],x=i%w,y=Math.floor(i/w);
   for(const n of [x>0?i-1:-1,x+1<w?i+1:-1,y>0?i-w:-1,y+1<h?i+w:-1])
    if(n>=0&&!visited[n]&&rgba[n*4+3]>=128){visited[n]=1;component.push(n);}
  }
  if(component.length<100)for(const i of component)rgba[i*4+3]=0;
 }
 // Include the rim's two-pixel luminous fringe and soften only the alpha.
 const alpha=Buffer.alloc(w*h);
 for(let y=0;y<h;y++)for(let x=0;x<w;x++){
  let coverage=0;
  for(let dy=-2;dy<=2;dy++)for(let dx=-2;dx<=2;dx++){
   if(dx*dx+dy*dy>4)continue;
   const ax=x+dx,ay=y+dy;
   if(ax>=0&&ay>=0&&ax<w&&ay<h)coverage=Math.max(coverage,rgba[(ay*w+ax)*4+3]);
  }
  alpha[y*w+x]=coverage;
 }
 const {data:soft,info:maskInfo}=await sharp(alpha,{raw:{width:w,height:h,channels:1}}).blur(.65).raw().toBuffer({resolveWithObject:true});
 for(let i=0;i<w*h;i++)rgba[i*4+3]=soft[i*maskInfo.channels];
 // The source XP star slightly overlaps the title's upper glow. The live HUD
 // owns that star, so its orange tip must not be baked into the shop asset.
 for(let y=91;y<105;y++)for(let x=1118;x<1145;x++)rgba[((y-py)*w+x-px)*4+3]=0;
 // The idle composition is one intact source layer, avoiding hairline seams
 // when adjacent controls are sampled at fractional display scales.
 const idle=Buffer.from(rgba);
 // Replace only pixels hidden by the independently rendered tiles. At rest,
 // their source artwork covers these regions exactly. On press the clean
 // panel is revealed, so controls do not leave a second copy underneath.
 for(const [name,[x,y,tw,th]] of Object.entries(tiles)){
  await sharp(source).extract({left:x,top:y,width:tw,height:th}).png().toFile(path.join(out,'reference-'+name+'.png'));
  for(let yy=y;yy<y+th;yy++)for(let xx=x;xx<x+tw;xx++){
   const dx=xx-px,dy=yy-py;if(dx<0||dy<0||dx>=w||dy>=h)continue;
   const i=(dy*w+dx)*4;
   // Keep the source rim behind the close control. Its outer tile is masked
   // below so no screenshot aquarium rectangle is retained.
   if(name==='close')continue;
   const s=(Math.min(dy,829)*w+dx)*4;clean.copy(rgba,i,s,s+3);
  }
 }
 // No navigation artwork belongs to the shop frame. Repair the tiny lower
 // right overlap with the generated blue rim, behind native corner controls.
 for(let y=735;y<830;y++)for(let x=1146;x<w;x++){
  const i=(y*w+x)*4;clean.copy(rgba,i,i,i+3);
 }
 await sharp(rgba,{raw:{width:w,height:h,channels:4}}).png().toFile(path.join(out,'reference-backing.png'));
 // Retain the same generated repair underneath the native Shop corner.
 for(let y=735;y<830;y++)for(let x=1146;x<w;x++){
  const i=(y*w+x)*4;clean.copy(idle,i,i,i+3);
 }
 await sharp(idle,{raw:{width:w,height:h,channels:4}}).png().toFile(path.join(out,'reference-frame.png'));
 // The close circle has its own alpha so aquarium pixels do not move with it.
 const close=await sharp(path.join(out,'reference-close.png')).ensureAlpha().raw().toBuffer();
 for(let y=0;y<95;y++)for(let x=0;x<96;x++){
  const d=Math.hypot((x-48)/46,(y-47.5)/46);
  close[(y*96+x)*4+3]=Math.round(255*Math.max(0,Math.min(1,(1-d)*23)));
 }
 await sharp(close,{raw:{width:96,height:95,channels:4}}).png().toFile(path.join(out,'reference-close.png'));
 // A true transparent source pearl supplies the alternate selected tab.
 const pearl=await sharp(source).extract({left:799,top:218,width:52,height:52}).ensureAlpha().raw().toBuffer();
 for(let y=0;y<52;y++)for(let x=0;x<52;x++){
  const d=Math.hypot(x-26,y-26);pearl[(y*52+x)*4+3]=Math.round(255*Math.max(0,Math.min(1,25-d)));
 }
 await sharp(pearl,{raw:{width:52,height:52,channels:4}}).png().toFile(path.join(out,'reference-pearl-icon.png'));
 for(const [name,box,shape] of [
  ['coin-icon',[520,219,59,52],[[18,2],[37,2],[48,6],[51,14],[45,20],[53,24],[54,34],[57,37],[57,45],[48,50],[17,50],[5,46],[3,38],[7,32],[7,25],[12,20],[11,13],[13,7]]],
  ['gift-icon',[1071,218,56,54],[[8,9],[19,9],[17,2],[25,1],[31,9],[36,2],[43,2],[47,8],[43,13],[51,16],[51,27],[48,30],[48,48],[32,53],[12,48],[10,29],[4,26],[4,17]]]
 ]){
  const [x,y,w,h]=box,data=await sharp(source).extract({left:x,top:y,width:w,height:h}).ensureAlpha().raw().toBuffer();
  for(let yy=0;yy<h;yy++)for(let xx=0;xx<w;xx++){
   let count=0;for(let sy=0;sy<4;sy++)for(let sx=0;sx<4;sx++)count+=inside(xx+(sx+.5)/4,yy+(sy+.5)/4,shape);
   data[(yy*w+xx)*4+3]=Math.round(count*255/16);
  }
  await sharp(data,{raw:{width:w,height:h,channels:4}}).png().toFile(path.join(out,'reference-'+name+'.png'));
 }
 const metadata={source:'work/currency-shop-reference/reference.png',screen:[40,22,1674,930],panel,tiles,offers,backing:'generated-clean-panel.png',notes:'Source lettering retained in each preview offer. Payments are not connected.'};
 await fs.writeFile(path.join(out,'reference-layout.json'),JSON.stringify(metadata,null,2)+'\n');
 const manifestPath=path.join(root,'assets/manifest.json'),manifest=JSON.parse(await fs.readFile(manifestPath,'utf8'));
 for(const name of await fs.readdir(out))if(name.startsWith('reference-')&&name.endsWith('.png')){
  const bytes=await fs.readFile(path.join(out,name)),meta=await sharp(bytes).metadata();
  manifest.assets['currency-shop/'+name]={source:'Supplied currency shop screenshot and built-in ImageGen clean backing; see currency-shop/reference-layout.json',size:[meta.width,meta.height],sha256:crypto.createHash('sha256').update(bytes).digest('hex')};
 }
 await fs.writeFile(manifestPath,JSON.stringify(manifest,null,2)+'\n');
 console.log('Prepared reference shop frame, seven offers, tabs, close and Earn Coins.');
}
main().catch(e=>{console.error(e);process.exitCode=1;});
