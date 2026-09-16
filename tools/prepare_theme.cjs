// Technical alpha packaging for ImageGen artwork. The generated colors stay intact.
const sharp=require(process.env.SHARP_MODULE||'/Users/amoghrijal/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
const path=require('node:path');
const root=path.resolve(__dirname,'..');
async function main(){
 const card=await sharp(path.join(root,'work/faceted-style/raw/card.png')).ensureAlpha().raw().toBuffer({resolveWithObject:true});
 const {width:w,height:h}=card.info;
 // Measured clipped corners of the generated card, excluding its checkerboard matte.
 const polygon=[[146,80],[826,80],[923,181],[923,1440],[832,1534],[137,1534],[49,1445],[49,181]];
 const inside=(x,y)=>polygon.every((a,i)=>{const b=polygon[(i+1)%polygon.length];return (b[0]-a[0])*(y-a[1])-(b[1]-a[1])*(x-a[0])>=0;});
 for(let y=0;y<h;y++)for(let x=0;x<w;x++){
  let coverage=0;for(const dy of [.25,.75])for(const dx of [.25,.75])if(inside(x+dx,y+dy))coverage++;
  card.data[(y*w+x)*4+3]=Math.round(255*coverage/4);
 }
 await sharp(card.data,{raw:{width:w,height:h,channels:4}}).extract({left:48,top:79,width:876,height:1456}).png().toFile(path.join(root,'assets/theme/card.png'));
 const shop=await sharp(path.join(root,'work/faceted-style/raw/shop.png')).ensureAlpha().raw().toBuffer({resolveWithObject:true});
 const sw=shop.info.width,sh=shop.info.height,seen=new Uint8Array(sw*sh),queue=new Int32Array(sw*sh);let head=0,tail=0;
 const add=(x,y)=>{if(x<0||y<0||x>=sw||y>=sh)return;const n=y*sw+x;if(seen[n])return;seen[n]=1;const i=n*4;const hi=Math.max(shop.data[i],shop.data[i+1],shop.data[i+2]),lo=Math.min(shop.data[i],shop.data[i+1],shop.data[i+2]);if(hi-lo>38)return;shop.data[i+3]=0;queue[tail++]=n;};
 for(let x=0;x<sw;x++){add(x,0);add(x,sh-1);}for(let y=0;y<sh;y++){add(0,y);add(sw-1,y);}
 while(head<tail){const n=queue[head++],x=n%sw,y=Math.floor(n/sw);add(x-1,y);add(x+1,y);add(x,y-1);add(x,y+1);}
 await sharp(shop.data,{raw:{width:sw,height:sh,channels:4}}).trim().png().toFile(path.join(root,'assets/theme/shop.png'));
 console.log('Prepared the card and shop icon with real PNG alpha.');
}
main().catch(e=>{console.error(e);process.exitCode=1;});
