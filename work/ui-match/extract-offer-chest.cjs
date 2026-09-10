const sharp = require('/Users/amoghrijal/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
const fs = require('fs');
const path = require('path');
const root = path.resolve(__dirname, '../..');
const input = '/Users/amoghrijal/.codex/generated_images/01a0873d-a2fa-7d12-b47f-7b3558d5f0a4/exec-4a631149-76ad-4cc7-9367-7e6ee8b165c8.png';
async function main() {
  const {data,info} = await sharp(input).ensureAlpha().raw().toBuffer({resolveWithObject:true});
  const {width,height}=info;
  const visited = new Uint8Array(width*height), queue = new Int32Array(width*height);
  let head=0,tail=0;
  const add=index=>{
    if(visited[index])return;
    visited[index]=1;
    const p=index*4,maximum=Math.max(data[p],data[p+1],data[p+2]),minimum=Math.min(data[p],data[p+1],data[p+2]);
    if(maximum>28)return;
    queue[tail++]=index;
  };
  for(let x=0;x<width;x++){add(x);add((height-1)*width+x);}
  for(let y=0;y<height;y++){add(y*width);add(y*width+width-1);}
  while(head<tail){
    const index=queue[head++],x=index%width,y=Math.floor(index/width);data[index*4+3]=0;
    if(x>0)add(index-1);if(x+1<width)add(index+1);if(y>0)add(index-width);if(y+1<height)add(index+width);
  }
  for(let p=0;p<data.length;p+=4)if(Math.max(data[p],data[p+1],data[p+2])<15)data[p+3]=0;
  let minX=width,minY=height,maxX=0,maxY=0;
  for(let y=0;y<height;y++)for(let x=0;x<width;x++)if(data[(y*width+x)*4+3]){
    minX=Math.min(minX,x);minY=Math.min(minY,y);maxX=Math.max(maxX,x);maxY=Math.max(maxY,y);
  }
  const out=path.join(root,'assets/skin/offer-chest.png');
  await sharp(data,{raw:{width,height,channels:4}}).extract({left:minX,top:minY,width:maxX-minX+1,height:maxY-minY+1}).extend({top:4,bottom:4,left:4,right:4,background:{r:0,g:0,b:0,alpha:0}}).png().toFile(out);
  const meta=await sharp(out).metadata();
  await sharp({create:{width:meta.width,height:meta.height,channels:3,background:'#007bab'}}).composite([{input:out,left:0,top:0}]).png().toFile(path.join(__dirname,'offer-chest-cyan-preview.png'));
  await sharp(path.join(root,'assets/skin/offers.png')).extract({left:630,top:390,width:1320,height:130}).png().toFile(path.join(root,'assets/skin/offer-water-strip.png'));
  console.log(JSON.stringify({file:out,width:meta.width,height:meta.height,hasAlpha:meta.hasAlpha,sourceBounds:[minX,minY,maxX-minX+1,maxY-minY+1],strip:'assets/skin/offer-water-strip.png',stripDimensions:[1320,130]}));
}
main().catch(error=>{console.error(error);process.exit(1)});
