// Restore only pixels hidden by the source sprites and the photographed bezel.
const sharp=require(process.env.SHARP_MODULE || '/Users/amoghrijal/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
const fs=require('fs'),path=require('path');
process.chdir(path.resolve(__dirname,'..'));
async function main(){
 const {data,info}=await sharp('design/reference/aquarium.png').ensureAlpha().raw().toBuffer({resolveWithObject:true});
 const W=info.width,H=info.height,sx=W/1608,sy=H/830;
 const cover=new Uint8Array(W*H);
 const ui={coins:[38,9],pearls:[247,9],level:[1160,3],notifications:[1434,14],menu:[1509,14],select:[1477,91],food:[1477,231],medicine:[1431,348],mastery:[1477,431],sell:[1477,563],tanks:[0,688],shop:[1248,680]};
 const layers=Object.entries(ui).map(([name,[x,y]])=>({file:`assets/aquarium/${name}.png`,left:['tanks','shop','level'].includes(name)?Math.round(x*sx):Math.floor(x*sx),top:['tanks','shop','level'].includes(name)?Math.round(y*sy):Math.floor(y*sy)}));
 const fish={molly:[1268,326,false],turn:[1066,525,false],guppy:[1105,572,true],'guppy-tilted':[1522,653,false],'guppy-lower':[1103,926,true],green:[1595,917,false]};
 for(const [name,[left,top,flop]]of Object.entries(fish))layers.push({file:`assets/aquarium/fish-${name}.png`,left,top,flop});
 for(const layer of layers){
  let pipeline=sharp(layer.file);if(layer.flop)pipeline=pipeline.flop();
  const {data:rgba,info:dim}=await pipeline.ensureAlpha().raw().toBuffer({resolveWithObject:true});
  for(let y=0;y<dim.height;y++)for(let x=0;x<dim.width;x++){
   const dx=x+layer.left,dy=y+layer.top;if(dx<0||dy<0||dx>=W||dy>=H)continue;
   const p=dy*W+dx;cover[p]=Math.max(cover[p],rgba[(y*dim.width+x)*4+3]);
  }
 }
 // Feather a narrow ring around each cutout; never replace a whole rectangle.
 const expanded=new Uint8Array(W*H),horizontal=new Uint8Array(W*H),radius=8;
 for(let y=0;y<H;y++)for(let x=0;x<W;x++){
  let value=0;for(let k=Math.max(0,x-radius);k<=Math.min(W-1,x+radius);k++)value=Math.max(value,cover[y*W+k]);horizontal[y*W+x]=value;
 }
 for(let y=0;y<H;y++)for(let x=0;x<W;x++){
  let value=0;for(let k=Math.max(0,y-radius);k<=Math.min(H-1,y+radius);k++)value=Math.max(value,horizontal[k*W+x]);expanded[y*W+x]=value;
 }
 const restored=await sharp('assets/aquarium/reef-restored.png').resize(W,H).ensureAlpha().raw().toBuffer();
 const feather=await sharp(expanded,{raw:{width:W,height:H,channels:1}}).blur(2).greyscale().raw().toBuffer();
 for(let y=0;y<H;y++)for(let x=0;x<W;x++){
  const i=y*W+x,p=i*4,px=(x+.5)/sx,py=(y+.5)/sy;
  const dx=Math.abs(px-804)-(804-130),dy=Math.abs(py-415)-(415-130);
  const edge=130-Math.hypot(Math.max(dx,0),Math.max(dy,0))-Math.min(Math.max(dx,dy),0);
  const cornerAlpha=Math.max(0,Math.min(1,(3-edge)/3));
  let restoration=0;
  // Source plants and fish have antialiased colour fringes outside their
  // opaque masks. Clear the full original footprints to prevent ghost art
  // when fish move or the display aspect ratio changes.
  for(const [rx,ry,rw,rh] of [[0,688,304,142],[1248,680,360,150],[816,210,158,67],[686,338,36,54],[711,368,106,62],[980,420,122,80],[710,596,123,77],[1027,590,107,70]]){
   const dist=Math.min(px-rx,rx+rw-px,py-ry,ry+rh-py);
   restoration=Math.max(restoration,Math.max(0,Math.min(1,(dist+4)/8)));
  }
  const alpha=Math.max(cover[i]/255,feather[i]/255,cornerAlpha,restoration);
  for(let c=0;c<3;c++)data[p+c]=Math.round(data[p+c]*(1-alpha)+restored[p+c]*alpha);
 }
 await sharp(data,{raw:{width:W,height:H,channels:4}}).png().toFile('assets/aquarium/reef.png');
 console.log('Composited the original reef with restored sprite occlusions.');
}
main().catch(error=>{console.error(error);process.exit(1)});
