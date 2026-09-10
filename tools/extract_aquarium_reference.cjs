// Extract the supplied artwork. The masks describe source silhouettes only.
// Run with the bundled Node runtime and SHARP_MODULE set when needed.
const sharp = require(process.env.SHARP_MODULE || '/Users/amoghrijal/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
const fs = require('fs');
const path = require('path');
const root = path.resolve(__dirname, '..');
const ellipse = (x,y,rx,ry) => (px,py) => (1-Math.hypot((px-x)/rx,(py-y)/ry))*Math.min(rx,ry);
const round = (x,y,w,h,r) => (px,py) => {
  const dx=Math.abs(px-x-w/2)-(w/2-r),dy=Math.abs(py-y-h/2)-(h/2-r);
  return r-Math.hypot(Math.max(dx,0),Math.max(dy,0))-Math.min(Math.max(dx,dy),0);
};
const polygon = points => (x,y) => {
  let inside=false,nearest=1e9;
  for(let i=0,j=points.length-1;i<points.length;j=i++){
    const [ax,ay]=points[j],[bx,by]=points[i];
    if((ay>y)!=(by>y)&&x<(bx-ax)*(y-ay)/(by-ay)+ax)inside=!inside;
    const dx=bx-ax,dy=by-ay,t=Math.max(0,Math.min(1,((x-ax)*dx+(y-ay)*dy)/(dx*dx+dy*dy)));
    nearest=Math.min(nearest,Math.hypot(x-ax-t*dx,y-ay-t*dy));
  }
  return inside?nearest:-nearest;
};
const specs = [
  {name:'coins',rect:[38,9,209,73],shapes:[ellipse(74,45,32,34),round(70,16,172,60,30)],clear:[108,29,61,34,173]},
  {name:'pearls',rect:[247,9,207,74],shapes:[ellipse(282,45,33,34),round(280,16,171,61,30)],clear:[314,29,28,35,352]},
  {name:'level',rect:[1160,3,270,90],shapes:[round(1202,26,226,39,20),polygon([[1208,3],[1224,30],[1251,34],[1260,47],[1238,65],[1237,89],[1224,94],[1207,81],[1181,90],[1175,77],[1180,63],[1161,45],[1162,34],[1191,29]])]},
  {name:'notifications',rect:[1434,14,74,76],shapes:[ellipse(1472,53,35,36),ellipse(1490,27,12,12)]},
  {name:'menu',rect:[1509,14,72,76],shapes:[ellipse(1545,53,35,36)]},
  {name:'select',rect:[1477,91,127,134],shapes:[ellipse(1539,152,60,59),round(1482,181,116,43,22)]},
  {name:'food',rect:[1477,231,127,132],shapes:[ellipse(1539,291,60,58),round(1482,316,116,45,22)]},
  {name:'medicine',rect:[1431,348,174,81],shapes:[round(1433,350,169,77,28)]},
  {name:'mastery',rect:[1477,431,127,132],shapes:[ellipse(1539,490,60,59),round(1482,514,116,46,22)]},
  {name:'sell',rect:[1477,563,127,132],shapes:[ellipse(1539,622,60,59),round(1482,648,116,43,22)]},
  {name:'tanks',rect:[10,714,216+52,113],shapes:[round(16,716,113,109,44),round(55,737,212,83,35)]},
  {name:'shop',rect:[1280,689,309,137],shapes:[round(1292,691,149,134,62),round(1367,709,220,108,37)]},
];
async function main(){
  const out=path.join(root,'assets/aquarium');fs.mkdirSync(out,{recursive:true});
  const {data,info}=await sharp(path.join(root,'design/reference/aquarium.png')).ensureAlpha().raw().toBuffer({resolveWithObject:true});
  const sx=info.width/1608,sy=info.height/830,meta={source:'design/reference/aquarium.png',design:[1608,830],assets:{}};
  for(const spec of specs){
    const [x,y,w,h]=spec.rect,left=Math.floor(x*sx),top=Math.floor(y*sy),width=Math.ceil(w*sx),height=Math.ceil(h*sy);
    const rgba=Buffer.alloc(width*height*4);
    for(let py=0;py<height;py++)for(let px=0;px<width;px++){
      const ax=left+px,ay=top+py,p=(py*width+px)*4,src=(ay*info.width+ax)*4;
      const dist=Math.max(...spec.shapes.map(f=>f((ax+.5)/sx,(ay+.5)/sy)));
      data.copy(rgba,p,src,src+3);rgba[p+3]=Math.round(Math.max(0,Math.min(1,dist+.5))*255);
    }
    const options={raw:{width,height,channels:4}};
    await sharp(rgba,options).png().toFile(path.join(out,spec.name+'.png'));
    if(spec.clear){
      const [cx,cy,cw,ch,cleanX]=spec.clear;
      for(let py=0;py<height;py++)for(let px=0;px<width;px++){
        const ax=(left+px)/sx,ay=(top+py)/sy;
        if(ax<cx||ax>=cx+cw||ay<cy||ay>=cy+ch)continue;
        const source=((top+py)*info.width+Math.floor(cleanX*sx))*4,p=(py*width+px)*4;
        data.copy(rgba,p,source,source+3);
      }
      await sharp(rgba,options).png().toFile(path.join(out,spec.name+'-empty.png'));
    }
    meta.assets[spec.name]={rect:spec.rect,pixels:[width,height]};
  }
  // Refined masks keep the decorative plants, bubbles, and star edges.
  const {execFileSync}=require('child_process');
  for(const script of ['aquarium/extract-fish.cjs','aquarium/extract-bottom-buttons.cjs','aquarium/extract-level.cjs','compose_aquarium_background.cjs'])
    execFileSync(process.execPath,[path.join(root,'tools',script)],{cwd:root,stdio:'inherit'});
  meta.assets.tanks={rect:[0,688,304,142],pixels:[472,221]};
  meta.assets.shop={rect:[1248,680,360,150],pixels:[559,233]};
  meta.assets.level={rect:[1160,3,270,90],pixels:[419,140]};
  fs.writeFileSync(path.join(out,'reference-layout.json'),JSON.stringify(meta,null,2)+'\n');
}
main().catch(e=>{console.error(e);process.exit(1)});
