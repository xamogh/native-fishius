// Preserve masters separately from the mobile runtime's smaller sprites/icons.
const fs=require('node:fs/promises');
const path=require('node:path');
const sharp=require(process.env.SHARP_MODULE||'sharp');
async function main(){
 const root=path.resolve(__dirname,'..');
 let items=JSON.parse(await fs.readFile(path.join(root,'assets/content.json'),'utf8')).decorations.items;
 if(process.argv.includes('--partial')){const available=[];for(const item of items){try{await fs.access(path.join(root,'assets',item.asset));available.push(item);}catch{}}items=available;}
 const masters=path.join(root,'design/decor-art/masters'),icons=path.join(root,'assets/decor/icons');
 await fs.mkdir(masters,{recursive:true});await fs.mkdir(icons,{recursive:true});
 const checks=[];
 for(const item of items){
  const runtime=path.join(root,item.asset.startsWith('assets/')?item.asset:'assets/'+item.asset),master=path.join(masters,item.id+'.png');
  let exists=true;try{await fs.access(master);}catch{exists=false;}
  if(!exists){const input=await fs.readFile(runtime);await sharp(input).resize(1024,1024,{fit:'inside',withoutEnlargement:true}).png().toFile(master);}
  const data=await fs.readFile(master);const meta=await sharp(data).metadata();
  if(!meta.hasAlpha)throw Error(item.id+' has no transparency');
  const {data:pixels,info}=await sharp(data).ensureAlpha().raw().toBuffer({resolveWithObject:true});
  let transparent=0,opaque=0,minX=info.width,minY=info.height,maxX=-1,maxY=-1;
  for(let y=0;y<info.height;y++)for(let x=0;x<info.width;x++){const a=pixels[(y*info.width+x)*4+3];if(a===0)transparent++;if(a>32){opaque++;minX=Math.min(minX,x);minY=Math.min(minY,y);maxX=Math.max(maxX,x);maxY=Math.max(maxY,y);}}
  if(transparent<info.width*info.height*.05||opaque<info.width*info.height*.04)throw Error(item.id+' has invalid alpha coverage');
  const derived=await sharp(data).resize(512,512,{fit:'inside',withoutEnlargement:true}).png().toBuffer();
  const tmp=runtime+'.tmp';await fs.writeFile(tmp,derived);await fs.rename(tmp,runtime);
  await sharp(data).resize(256,256,{fit:'contain',background:{r:0,g:0,b:0,alpha:0}}).png().toFile(path.join(icons,item.id+'.png'));
  checks.push({id:item.id,width:meta.width,height:meta.height,transparent,opaque,alpha_bounds:[minX,minY,maxX,maxY],source:item.art.source});
 }
 const motionChecks=[];
 if(!process.argv.includes('--partial'))for(const id of ['PD-12','PL-11','PL-07-base','PL-07-wheel']){
  const master=path.join(root,'design/decor-art/motion',id+'.png');const data=await fs.readFile(master),meta=await sharp(data).metadata();
  if(!meta.hasAlpha)throw Error(id+' motion layer has no alpha');
  await sharp(data).resize(512,512,{fit:'inside',withoutEnlargement:true}).png().toFile(path.join(root,'assets/decor/motion',id+'.png'));
  motionChecks.push({id,width:meta.width,height:meta.height,has_alpha:meta.hasAlpha});
 }
 await fs.mkdir(path.join(root,'evidence'),{recursive:true});await fs.writeFile(path.join(root,'evidence/decor-art-audit.json'),JSON.stringify({count:checks.length,checks,motion_layers:motionChecks},null,2));
 // Contact sheets retain each item's ID for visual inspection at icon scale.
 for(let page=0;page<Math.ceil(items.length/24);page++){
  const layers=[];for(let i=0;i<24&&page*24+i<items.length;i++){
   const item=items[page*24+i],left=(i%6)*170,top=Math.floor(i/6)*180;
   const icon=await sharp(path.join(icons,item.id+'.png')).resize(136,136).toBuffer();layers.push({input:icon,left:left+17,top:top+4});
   const label=Buffer.from(`<svg width="170" height="32"><text x="85" y="23" text-anchor="middle" font-family="Arial" font-size="17" fill="#163f48">${item.id}</text></svg>`);layers.push({input:label,left,top:top+143});
  }
  await sharp({create:{width:1020,height:720,channels:4,background:'#c8e5e4'}}).composite(layers).png().toFile(path.join(root,`evidence/decor-art-contact-${page+1}.png`));
 }
 console.log(`Prepared ${checks.length} masters, runtime sprites and icons.`);
}
main().catch(e=>{console.error(e);process.exit(1)});
