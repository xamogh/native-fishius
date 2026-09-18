// Package ImageGen artwork into shared native textures. Source pixels are
// retained. For RGB exports, only the connected neutral exterior is removed.
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const sharp = require(process.env.SHARP_MODULE || '/Users/amoghrijal/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
const root = path.resolve(__dirname, '..');
const sizes = {food:[320,320]};

async function main() {
 const source = JSON.parse(fs.readFileSync(path.join(root,'work/lagoon-redesign/generation.json')));
 const manifestPath = path.join(root,'assets/manifest.json');
 const manifest = JSON.parse(fs.readFileSync(manifestPath));
 const records = [];
 for (const asset of source.assets.filter(asset => asset.name === 'food')) {
  const meta = await sharp(asset.source).metadata();
  const {data,info} = await sharp(asset.source).ensureAlpha().raw().toBuffer({resolveWithObject:true});
  const {width:w,height:h} = info;
  if (!meta.hasAlpha && asset.name !== 'reef') {
   const outside = new Uint8Array(w*h), queue = new Int32Array(w*h);
   let head=0, tail=0;
   const add = index => {
    if (index<0 || index>=w*h || outside[index]) return;
    const i=index*4, span=Math.max(data[i],data[i+1],data[i+2])-Math.min(data[i],data[i+1],data[i+2]);
    if (span>28) return;
    outside[index]=1;queue[tail++]=index;
   };
   for(let x=0;x<w;x++){add(x);add((h-1)*w+x);}
   for(let y=0;y<h;y++){add(y*w);add(y*w+w-1);}
   while(head<tail){const i=queue[head++],x=i%w;add(i-w);add(i+w);if(x)add(i-1);if(x+1<w)add(i+1);}
   for(let i=0;i<w*h;i++)if(outside[i])data.fill(0,i*4,i*4+4);
  }
  let left=w,top=h,right=0,bottom=0;
  for(let y=0;y<h;y++)for(let x=0;x<w;x++)if(data[(y*w+x)*4+3]>128){left=Math.min(left,x);top=Math.min(top,y);right=Math.max(right,x);bottom=Math.max(bottom,y);}
  left=Math.max(0,left-2);top=Math.max(0,top-2);right=Math.min(w-1,right+2);bottom=Math.min(h-1,bottom+2);
  const crop={left,top,width:right-left+1,height:bottom-top+1};
  const dest=path.join(root,'assets/lagoon',asset.name+'.png');
  await sharp(data,{raw:{width:w,height:h,channels:4}}).extract(crop).resize(...sizes[asset.name],{fit:['shop','food'].includes(asset.name)?'contain':'fill',background:{r:0,g:0,b:0,alpha:0}}).png().toFile(dest);
  const stats=await sharp(dest).stats();
  if(asset.name!=='reef' && (stats.channels[3].min!==0 || stats.channels[3].max!==255))throw new Error(asset.name+' has no transparent exterior');
  const sha256=crypto.createHash('sha256').update(fs.readFileSync(dest)).digest('hex');
  const record={...asset,file:asset.name+'.png',size:sizes[asset.name],sourceHadAlpha:meta.hasAlpha,crop,alphaRange:[stats.channels[3].min,stats.channels[3].max],sha256};
  records.push(record);
  manifest.assets['lagoon/'+asset.name+'.png']={size:record.size,pivot:[.5,.5],source:'Built-in ImageGen, see lagoon/provenance.json',sha256};
  console.log(asset.name,record.size,record.alphaRange);
 }
 fs.writeFileSync(path.join(root,'assets/lagoon/provenance.json'),JSON.stringify({tool:source.tool,assets:records},null,2)+'\n');
 fs.writeFileSync(manifestPath,JSON.stringify(manifest,null,2)+'\n');
}
main().catch(error=>{console.error(error);process.exitCode=1;});
