// Keep the supplied illustration pixels. ImageGen replaces only old lettering
// and the empty panel face, then native text is rendered once by the app.
const fs=require('node:fs/promises');
const path=require('node:path');
const crypto=require('node:crypto');
const sharp=require(process.env.SHARP_MODULE||'/Users/amoghrijal/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
const root=path.resolve(__dirname,'..'),out=path.join(root,'assets/currency-shop'),work=path.join(root,'work/currency-shop-reference');

// Match generated replacement pixels to the surrounding source material. A
// coarse harmonic correction preserves the generated surface without seams.
function blend(target,generated,width,height,[x,y,w,h]){
 const cols=Math.min(w+1,65),rows=Math.min(h+1,49);
 for(let c=0;c<3;c++){
  const correction=new Float32Array(cols*rows);
  for(let yy=0;yy<rows;yy++)for(let xx=0;xx<cols;xx++)if(!xx||!yy||xx===cols-1||yy===rows-1){
   const sx=Math.round(x+xx*w/(cols-1)),sy=Math.round(y+yy*h/(rows-1));
   const p=(Math.min(height-1,sy)*width+Math.min(width-1,sx))*4+c;
   correction[yy*cols+xx]=target[p]-generated[p];
  }
  for(let step=0;step<550;step++)for(let yy=1;yy<rows-1;yy++)for(let xx=1;xx<cols-1;xx++){
   const i=yy*cols+xx,mean=(correction[i-1]+correction[i+1]+correction[i-cols]+correction[i+cols])*.25;
   correction[i]+=1.65*(mean-correction[i]);
  }
  for(let yy=0;yy<h;yy++)for(let xx=0;xx<w;xx++){
   const gx=xx*(cols-1)/w,gy=yy*(rows-1)/h,ix=Math.floor(gx),iy=Math.floor(gy),fx=gx-ix,fy=gy-iy;
   const a=correction[iy*cols+ix]*(1-fx)+correction[iy*cols+ix+1]*fx;
   const b=correction[(iy+1)*cols+ix]*(1-fx)+correction[(iy+1)*cols+ix+1]*fx;
   const i=((y+yy)*width+x+xx)*4+c;
   target[i]=Math.max(0,Math.min(255,Math.round(generated[i]+a*(1-fy)+b*fy)));
  }
 }
}
async function main(){
 const layout=JSON.parse(await fs.readFile(path.join(out,'reference-layout.json'),'utf8'));
 const regions=JSON.parse(await fs.readFile(path.join(work,'title-regions.json'),'utf8'));
 const source=await sharp(path.join(work,'reference.png')).ensureAlpha().raw().toBuffer();
 const generatedGrid=await sharp(path.join(work,'cards-no-text-generated.png')).resize(1125,533).ensureAlpha().raw().toBuffer();
 const cards=[];
 for(const offer of layout.offers){
  const [x,y,w,h]=layout.tiles[offer.id],pixels=Buffer.alloc(w*h*4),generated=Buffer.alloc(w*h*4);
  for(let yy=0;yy<h;yy++)for(let xx=0;xx<w;xx++){
   const i=(yy*w+xx)*4,s=((y+yy)*1752+x+xx)*4,g=((y+yy-282)*1125+x+xx-316)*4;
   source.copy(pixels,i,s,s+4);generatedGrid.copy(generated,i,g,g+4);
  }
  for(const [sx,sy,sw,sh] of regions[offer.id])blend(pixels,generated,w,h,[sx-x,sy-y,sw,sh]);
  // Only the rounded card, glow and ribbon are visible. Exterior screenshot
  // water must not move with a card when the player presses it.
  const radius=31,left=2,top=2,right=w-2,bottom=h-3;
  for(let yy=0;yy<h;yy++)for(let xx=0;xx<w;xx++){
   const cx=Math.max(left+radius,Math.min(right-radius,xx+.5));
   const cy=Math.max(top+radius,Math.min(bottom-radius,yy+.5));
   let alpha=Math.max(0,Math.min(1,radius+1-Math.hypot(xx+.5-cx,yy+.5-cy)));
   if(offer.id==='starter'&&yy>=54&&yy<=110&&xx>=w-7)alpha=1;
   pixels[(yy*w+xx)*4+3]=Math.round(alpha*255);
  }
  const file='clean-'+offer.id+'.png';
  await sharp(pixels,{raw:{width:w,height:h,channels:4}}).png().toFile(path.join(out,file));
  cards.push({id:offer.id,file,source:layout.tiles[offer.id],layout:[x,y-44,w,h],text_regions:regions[offer.id]});
 }
 const w=1210,h=854;
 const frame=await sharp(path.join(out,'reference-frame.png')).ensureAlpha().raw().toBuffer();
 const generated=await sharp(path.join(work,'generated-clean-panel.png')).resize(w,830,{fit:'fill'}).extend({bottom:24,background:{r:0,g:145,b:227,alpha:1}}).ensureAlpha().raw().toBuffer();
 // Clear all tabs, cards and the footer from the background, not just their
 // nominal text boxes. No original item copy remains under moving content.
 blend(frame,generated,w,h,[175,114,872,78]);
 blend(frame,generated,w,h,[43,149,1135,661]);
 const image=sharp(frame,{raw:{width:w,height:h,channels:4}});
 const top=await image.clone().extract({left:0,top:0,width:w,height:130}).png().toBuffer();
 const middle=await image.clone().extract({left:0,top:130,width:w,height:580}).resize(w,536,{fit:'fill'}).png().toBuffer();
 const bottom=await image.clone().extract({left:0,top:710,width:w,height:144}).png().toBuffer();
 await sharp({create:{width:w,height:810,channels:4,background:'#00000000'}}).composite([
  {input:top,left:0,top:0},{input:middle,left:0,top:130},{input:bottom,left:0,top:666}
 ]).png().toFile(path.join(out,'clean-frame.png'));
 // Mask the source footer's surrounding water so it follows the new frame.
 const earn=await sharp(path.join(out,'reference-earn.png')).ensureAlpha().raw().toBuffer();
 for(let y=0;y<87;y++)for(let x=0;x<562;x++){
  const cx=Math.max(30,Math.min(532,x+.5)),cy=Math.max(30,Math.min(57,y+.5));
  earn[(y*562+x)*4+3]=Math.round(255*Math.max(0,Math.min(1,29-Math.hypot(x+.5-cx,y+.5-cy))));
 }
 await sharp(earn,{raw:{width:562,height:87,channels:4}}).png().toFile(path.join(out,'clean-earn.png'));
 await fs.writeFile(path.join(out,'clean-layout.json'),JSON.stringify({panel:[270,91,1210,810],removed_tab_height:44,cards,earn:[597,772,562,87],text:'Native labels. No pack lettering is baked into the card or frame images.'},null,2)+'\n');
 const manifestPath=path.join(root,'assets/manifest.json'),manifest=JSON.parse(await fs.readFile(manifestPath,'utf8'));
 for(const name of await fs.readdir(out))if(name.startsWith('clean-')&&name.endsWith('.png')){
  const bytes=await fs.readFile(path.join(out,name)),meta=await sharp(bytes).metadata();
  manifest.assets['currency-shop/'+name]={source:'Source artwork with ImageGen lettering removal; see work/currency-shop-reference/cards-no-text-provenance.json',size:[meta.width,meta.height],sha256:crypto.createHash('sha256').update(bytes).digest('hex')};
 }
 await fs.writeFile(manifestPath,JSON.stringify(manifest,null,2)+'\n');
 console.log('Prepared seven text-free card sprites and a shorter empty frame without tabs.');
}
main().catch(e=>{console.error(e);process.exitCode=1;});
