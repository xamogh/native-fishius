const sharp = require('/Users/amoghrijal/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
const fs = require('fs');
const path = require('path');
const root = path.resolve(__dirname, '../..');
const input = '/Users/amoghrijal/.codex/generated_images/01a0873d-a2fa-7d12-b47f-7b3558d5f0a4/exec-8b454e51-47c3-4484-ab28-e6b44a75316f.png';
async function main() {
  const {data,info} = await sharp(input).ensureAlpha().raw().toBuffer({resolveWithObject:true});
  const {width,height}=info;
  const visited = new Uint8Array(width*height), queue = new Int32Array(width*height);
  let head=0,tail=0;
  const add=index=>{
    if(visited[index])return;
    visited[index]=1;
    const p=index*4,maximum=Math.max(data[p],data[p+1],data[p+2]),minimum=Math.min(data[p],data[p+1],data[p+2]);
    if(maximum-minimum>40)return;
    queue[tail++]=index;
  };
  for(let x=0;x<width;x++){add(x);add((height-1)*width+x);}
  for(let y=0;y<height;y++){add(y*width);add(y*width+width-1);}
  while(head<tail){
    const index=queue[head++],x=index%width,y=Math.floor(index/width);data[index*4+3]=0;
    if(x>0)add(index-1);if(x+1<width)add(index+1);if(y>0)add(index-width);if(y+1<height)add(index+width);
  }
  let minX=width,minY=height,maxX=0,maxY=0;
  for(let y=0;y<height;y++)for(let x=0;x<width;x++)if(data[(y*width+x)*4+3]){
    minX=Math.min(minX,x);minY=Math.min(minY,y);maxX=Math.max(maxX,x);maxY=Math.max(maxY,y);
  }
  const raw=await sharp(data,{raw:{width,height,channels:4}}).extract({left:minX,top:minY,width:maxX-minX+1,height:maxY-minY+1}).extend({top:4,bottom:4,left:4,right:4,background:{r:0,g:0,b:0,alpha:0}}).raw().toBuffer({resolveWithObject:true});
  const w=raw.info.width,h=raw.info.height,options={raw:{width:w,height:h,channels:4}};
  const out=path.join(root,'assets/skin/fish-yellow-tang.png');
  await sharp(raw.data,options).png().toFile(out);
  const white=Buffer.from(raw.data);
  for(let p=0;p<white.length;p+=4)white[p]=white[p+1]=white[p+2]=255;
  await sharp(white,options).png().toFile(path.join(root,'assets/skin/fish-yellow-tang-mask.png'));
  await sharp(raw.data,options).grayscale().png().toFile(path.join(root,'assets/skin/fish-yellow-tang-dead.png'));
  await sharp({create:{width:w,height:h,channels:3,background:'#008ac8'}}).composite([{input:out,left:0,top:0}]).png().toFile(path.join(__dirname,'yellow-tang-cyan-preview.png'));
  const records={file:out,width:w,height:h,hasAlpha:true,sourceDimensions:[width,height],sourceBounds:[minX,minY,maxX-minX+1,maxY-minY+1],padding:4};
  fs.writeFileSync(path.join(__dirname,'yellow-tang-extraction.json'),JSON.stringify(records,null,2)+'\n');
  console.log(JSON.stringify(records,null,2));
}
main().catch(error=>{console.error(error);process.exit(1)});
