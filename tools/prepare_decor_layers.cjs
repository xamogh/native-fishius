// Combine generated empty-glass patches with the unchanged source silhouette.
// The source alpha and all pixels outside the removed internal FX are retained.
const fs=require('node:fs/promises'),path=require('node:path');
const sharp=require(process.env.SHARP_MODULE||'sharp');
const root=path.resolve(__dirname,'..');
async function main(){
 const manifest=JSON.parse(await fs.readFile(path.join(root,'assets/decor/provenance/generated-assets.json'),'utf8'));
 const masks={'PD-12':[[.498,.37,.065,.045],[.5,.699,.065,.045]],'PL-11':[[.50,.44,.34,.28]]};
 for(const [id,ellipses] of Object.entries(masks)){
  const source=manifest.sources.find(x=>x.id===id),edited=manifest.motion.find(x=>x.id===id);
  const {data,info}=await sharp(source.path).ensureAlpha().raw().toBuffer({resolveWithObject:true});
  const patch=await sharp(edited.path).resize(info.width,info.height).ensureAlpha().raw().toBuffer();
  for(let y=0;y<info.height;y++)for(let x=0;x<info.width;x++){
   let blend=0;for(const [cx,cy,rx,ry] of ellipses){const distance=Math.hypot((x/info.width-cx)/rx,(y/info.height-cy)/ry);const t=Math.max(0,Math.min(1,(1-distance)/.12));blend=Math.max(blend,t*t*(3-2*t));}
   const at=(y*info.width+x)*4;for(let c=0;c<3;c++)data[at+c]=Math.round(data[at+c]*(1-blend)+patch[at+c]*blend);
  }
  await sharp(data,{raw:info}).resize(1024,1024,{fit:'inside',withoutEnlargement:true}).png().toFile(path.join(root,'design/decor-art/motion',id+'.png'));
 }
}
main().catch(e=>{console.error(e);process.exit(1)});
