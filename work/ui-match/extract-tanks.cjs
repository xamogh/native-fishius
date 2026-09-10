const sharp = require('/Users/amoghrijal/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
const fs = require('fs');
const path = require('path');
const root = path.resolve(__dirname, '../..');
const input = '/Users/amoghrijal/.codex/generated_images/01a0873d-a2fa-7d12-b47f-7b3558d5f0a4/exec-9a2f332f-c4e8-40ea-8ec1-cbd3963dad55.png';
async function main() {
  const atlas = [];
  const previews = [];
  const records = [];
  for (let i = 0; i < 5; i++) {
    const width=512, height=512, x=(i%3)*512, y=Math.floor(i/3)*512;
    const {data} = await sharp(input).extract({left:x,top:y,width,height}).ensureAlpha().raw().toBuffer({resolveWithObject:true});
    const visited = new Uint8Array(width*height);
    const queue = new Int32Array(width*height);
    let head=0, tail=0;
    const add = (index) => {
      if (visited[index]) return;
      visited[index]=1;
      const p=index*4, maximum=Math.max(data[p],data[p+1],data[p+2]), minimum=Math.min(data[p],data[p+1],data[p+2]);
      if (maximum-minimum > 40) return;
      queue[tail++]=index;
    };
    for(let xx=0;xx<width;xx++){add(xx);add((height-1)*width+xx);}
    for(let yy=0;yy<height;yy++){add(yy*width);add(yy*width+width-1);}
    while(head<tail){
      const index=queue[head++],xx=index%width,yy=Math.floor(index/width);
      data[index*4+3]=0;
      if(xx>0)add(index-1); if(xx+1<width)add(index+1);
      if(yy>0)add(index-width); if(yy+1<height)add(index+width);
    }
    let minX=width,minY=height,maxX=0,maxY=0;
    for(let yy=0;yy<height;yy++)for(let xx=0;xx<width;xx++)if(data[(yy*width+xx)*4+3]){
      minX=Math.min(minX,xx);minY=Math.min(minY,yy);maxX=Math.max(maxX,xx);maxY=Math.max(maxY,yy);
    }
    const fullTile=await sharp(data,{raw:{width,height,channels:4}}).png().toBuffer();
    atlas.push({input:fullTile,left:x,top:y});
    const cropped = await sharp(data,{raw:{width,height,channels:4}})
      .extract({left:minX,top:minY,width:maxX-minX+1,height:maxY-minY+1})
      .extend({top:4,bottom:4,left:4,right:4,background:{r:0,g:0,b:0,alpha:0}}).png().toBuffer();
    const out = path.join(root,'assets/skin',`tank-${i+1}.png`);
    fs.writeFileSync(out,cropped);
    const meta = await sharp(cropped).metadata();
    records.push({file:out,width:meta.width,height:meta.height,hasAlpha:meta.hasAlpha,atlasCell:[x,y,width,height],objectBounds:[minX,minY,maxX-minX+1,maxY-minY+1]});
    previews.push({input:fullTile,left:x,top:y});
  }
  await sharp({create:{width:1536,height:1024,channels:4,background:{r:0,g:0,b:0,alpha:0}}}).composite(atlas).png().toFile(path.join(root,'assets/skin/tanks-atlas.png'));
  await sharp({create:{width:1536,height:1024,channels:3,background:'#008ac8'}}).composite(previews).png().toFile(path.join(__dirname,'tank-cyan-preview.png'));
  fs.writeFileSync(path.join(__dirname,'tank-extraction.json'),JSON.stringify(records,null,2)+'\n');
  console.log(JSON.stringify(records,null,2));
}
main().catch(error=>{console.error(error);process.exit(1)});
