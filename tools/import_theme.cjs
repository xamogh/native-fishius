// Package generated RGBA sprites. Only empty canvas margins are cropped.
const fs=require('node:fs');
const path=require('node:path');
const crypto=require('node:crypto');
const sharp=require(process.env.SHARP_MODULE||'/Users/amoghrijal/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
const root=path.resolve(__dirname,'..');
async function main(){
 const recordPath=path.join(root,'assets/theme/provenance.json');
 const record=JSON.parse(fs.readFileSync(recordPath,'utf8'));
 for(const asset of record.assets){
  const dest=path.join(root,'assets/theme',asset.file);
  if(asset.import){
   const meta=await sharp(asset.generated_source).metadata();
   if(!meta.hasAlpha)throw new Error(`${asset.file}: generated source has no alpha channel`);
   const {data,info}=await sharp(asset.generated_source).ensureAlpha().raw().toBuffer({resolveWithObject:true});
   let left=info.width,top=info.height,right=-1,bottom=-1;
   for(let y=0;y<info.height;y++)for(let x=0;x<info.width;x++)if(data[(y*info.width+x)*4+3]>=128){
    left=Math.min(left,x);right=Math.max(right,x);top=Math.min(top,y);bottom=Math.max(bottom,y);
   }
   if(right<left)throw new Error(`${asset.file}: empty alpha channel`);
   left=Math.max(0,left-3);top=Math.max(0,top-3);right=Math.min(info.width-1,right+3);bottom=Math.min(info.height-1,bottom+3);
   asset.canvas_crop={left,top,width:right-left+1,height:bottom-top+1};
   await sharp(asset.generated_source).extract(asset.canvas_crop).png().toFile(dest);
  }
  const meta=await sharp(dest).metadata(),stats=await sharp(dest).stats();
  asset.size=[meta.width,meta.height];asset.alpha=meta.hasAlpha;
  asset.alpha_range=meta.hasAlpha?[stats.channels[3].min,stats.channels[3].max]:null;
  if(asset.file!=='reef.png'&&(!asset.alpha||asset.alpha_range[0]!==0||asset.alpha_range[1]!==255))throw new Error(`${asset.file}: invalid transparency`);
  asset.sha256=crypto.createHash('sha256').update(fs.readFileSync(dest)).digest('hex');
 }
 fs.writeFileSync(recordPath,JSON.stringify(record,null,2)+'\n');
 const manifestPath=path.join(root,'assets/manifest.json'),manifest=JSON.parse(fs.readFileSync(manifestPath,'utf8'));
 for(const a of record.assets)manifest.assets['theme/'+a.file]={size:a.size,pivot:[.5,.5],source:'New ImageGen artwork from visual references; see theme/provenance.json',sha256:a.sha256};
 fs.writeFileSync(manifestPath,JSON.stringify(manifest,null,2)+'\n');
 console.log(JSON.stringify(record.assets.map(a=>({file:a.file,size:a.size,alpha:a.alpha,alpha_range:a.alpha_range})),null,2));
}
main().catch(error=>{console.error(error);process.exitCode=1;});
