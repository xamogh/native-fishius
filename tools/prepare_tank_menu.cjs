// Package reference artwork as independent, text-free native UI sprites.
const fs = require('node:fs/promises');
const path = require('node:path');
const crypto = require('node:crypto');
const sharp = require(process.env.SHARP_MODULE || '/Users/amoghrijal/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
const root = path.resolve(__dirname, '..');
const work = path.join(root, 'work/tank-menu-reference');
const out = path.join(root, 'assets/tank-menu');
const W = 1784, H = 1004;
const box = ([left, top, width, height]) => ({left, top, width, height});

async function main() {
 await fs.mkdir(out, {recursive:true});
 const original = await sharp(path.join(work,'reference.png')).ensureAlpha().raw().toBuffer();
 const plate = await sharp(path.join(work,'generated-clean-plate.png')).resize(1220,920).ensureAlpha().raw().toBuffer();
 const clean = Buffer.from(original);
 // Keep the original art outside the editable lettering. The generated plate
 // supplies the blank material; a short feather blends its boundary.
 const patches = [[508,205,202,63],[585,276,91,30],
  [508,344,218,59],[559,419,57,28],[695,418,40,29],
  [509,483,216,84],[558,582,59,27],[695,582,43,28],
  [509,646,218,80],[558,739,68,30],[695,739,55,30],
  [509,801,220,80],[558,894,66,27],[695,893,57,30],
  [950,838,132,49],[1286,838,51,50],[1040,631,188,28]];
 for (const [x,y,w,h] of patches)for(let c=0;c<3;c++) {
  const stride=w+2,material=new Float32Array((w+2)*(h+2));
  for(let yy=0;yy<h+2;yy++)for(let xx=0;xx<w+2;xx++) {
   const p=((y+yy-1)*W+x+xx-1)*4+c,g=((y+yy-59)*1220+x+xx-287)*4+c;
   material[yy*stride+xx]=(xx==0||yy==0||xx==w+1||yy==h+1)?original[p]:plate[g];
  }
  // Match generated blank material to all four source edges, without touching
  // the illustrated rim, icons or button silhouette.
  for(let pass=0;pass<900;pass++)for(let yy=1;yy<=h;yy++)for(let xx=1;xx<=w;xx++){
   const p=yy*stride+xx;material[p]=(material[p-1]+material[p+1]+material[p-stride]+material[p+stride])*.25;
  }
  for(let yy=0;yy<h;yy++)for(let xx=0;xx<w;xx++)clean[((y+yy)*W+x+xx)*4+c]=Math.round(material[(yy+1)*stride+xx+1]);
 }
 const entries={};
 const inside=(x,y,points)=>{let hit=false;for(let i=0,j=points.length-1;i<points.length;j=i++){
  const a=points[i],b=points[j];if((a[1]>y)!=(b[1]>y)&&x<(b[0]-a[0])*(y-a[1])/(b[1]-a[1])+a[0])hit=!hit;
 }return hit;};
 async function save(name,data,w,h,sourceBox) {
  const file=path.join(out,name+'.png');await sharp(data,{raw:{width:w,height:h,channels:4}}).png().toFile(file);
  entries['tank-menu/'+name+'.png']={source:'User tank-menu reference and built-in ImageGen clean material; see assets/tank-menu/provenance.json',size:[w,h],sha256:crypto.createHash('sha256').update(await fs.readFile(file)).digest('hex'),crop:sourceBox};
 }
 async function sprite(name,source,bounds,mask) {
  const [x,y,w,h]=bounds;
  const data=await sharp(source,{raw:{width:W,height:H,channels:4}}).extract(box(bounds)).raw().toBuffer();
  if(mask)for(let yy=0;yy<h;yy++)for(let xx=0;xx<w;xx++){
   let covered=0;for(let sy=0;sy<4;sy++)for(let sx=0;sx<4;sx++)covered+=mask(xx+(sx+.5)/4,yy+(sy+.5)/4,w,h);
   data[(yy*w+xx)*4+3]=Math.round(255*covered/16);
  }
  await save(name,data,w,h,bounds);
 }
 const rounded=radius=>(x,y,w,h)=>{
  const dx=Math.max(radius-x,0,x-(w-radius)),dy=Math.max(radius-y,0,y-(h-radius));return dx*dx+dy*dy<=radius*radius;
 };
 const polygon=points=>(x,y)=>inside(x,y,points);
 // Reuse the Lagoon panel, preserving its corners at the reference's thinner
 // border size. Baking the nine slices avoids seams in the software renderer.
 const panel=await sharp(path.join(root,'assets/lagoon/panel.png')).metadata();
 const cut=140,edge=85,sx=[0,cut,panel.width-cut,panel.width],sy=[0,cut,panel.height-cut,panel.height];
 const dx=[0,edge,1220-edge,1220],dy=[0,edge,850-edge,850],slices=[];
 for(let y=0;y<3;y++)for(let x=0;x<3;x++)slices.push({
  input:await sharp(path.join(root,'assets/lagoon/panel.png')).extract({left:sx[x],top:sy[y],width:sx[x+1]-sx[x],height:sy[y+1]-sy[y]}).resize(dx[x+1]-dx[x],dy[y+1]-dy[y],{fit:'fill'}).png().toBuffer(),left:dx[x],top:dy[y]});
 const panelData=await sharp({create:{width:1220,height:850,channels:4,background:'#00000000'}}).composite(slices).raw().toBuffer();
 await save('panel',panelData,1220,850,'Nine-slice derivative of assets/lagoon/panel.png');
 const wellBounds=[320,178,467,765],well=await sharp(original,{raw:{width:W,height:H,channels:4}}).extract(box(wellBounds)).raw().toBuffer();
 for(let y=12;y<753;y++)for(let x=11;x<456;x++)original.copy(well,(y*467+x)*4,((178+y)*W+780)*4,((178+y)*W+780)*4+4);
 for(let y=0;y<765;y++)for(let x=0;x<467;x++)if(!rounded(34)(x+.5,y+.5,467,765))well[(y*467+x)*4+3]=0;
 await save('well',well,467,765,wellBounds);
 await sprite('sign',original,[378,60,378,119],polygon([[3,61],[7,41],[16,26],[38,17],[50,16],[56,12],[169,5],[308,4],[315,9],[341,9],[360,17],[373,35],[377,66],[371,89],[359,104],[330,108],[59,117],[26,113],[9,99],[3,79]]));
 await sprite('close',original,[1374,99,109,111],(x,y)=>((x-54.5)/53.5)**2+((y-54.5)/53.5)**2<=1);
 await sprite('thumbnail',original,[351,205,140,111],polygon([[5,25],[13,13],[41,2],[120,4],[133,12],[135,79],[139,87],[134,98],[102,110],[14,104],[5,97],[4,82]]));
 await sprite('thumbnail-locked',original,[355,345,134,115],polygon([[4,25],[8,15],[37,3],[86,3],[104,0],[123,5],[133,22],[132,40],[124,51],[124,91],[113,103],[91,113],[13,107],[4,99],[4,86]]));
 await sprite('lock-badge',original,[438,346,51,51],(x,y)=>((x-25.5)/25)**2+((y-25.5)/25)**2<=1);
 await sprite('arrow',original,[1110,772,36,28],(x,y)=>{
  const p=((772+Math.floor(y))*W+1110+Math.floor(x))*4;return original[p]<120&&original[p+1]<160&&original[p+2]<180;
 });
 await sprite('coin-small',clean,[509,408,120,47],rounded(16));
 await sprite('pearl-small',clean,[636,408,120,48],rounded(16));
 await sprite('coin-button',clean,[828,815,295,96],rounded(31));
 await sprite('pearl-button',clean,[1139,815,296,96],rounded(31));
 await sprite('row-progress',clean,[508,273,245,36],rounded(18));
 await sprite('progress',original,[821,578,511,37],rounded(18));
 await sprite('status',clean,[987,622,279,44],rounded(22));
 const warning=Buffer.from(original);
 for(let y=295;y<335;y++)for(let x=1270;x<1301;x++)original.copy(warning,(y*W+x)*4,(y*W+1267)*4,(y*W+1267)*4+4);
 await sprite('warning',warning,[1200,279,120,71],rounded(29));
 // ImageGen reconstructs the preview's rim where the warning covered it.
 // Keep this one continuous image so no repair seam crosses the glass.
 const preview=Buffer.from(original);
 const gx=810,gy=243,gw=642,gh=325;
 for(let y=gy;y<gy+gh;y++)for(let x=gx;x<gx+gw;x++){
  const p=(y*W+x)*4,g=((y-58)*1220+x-286)*4;
  const blend=1;
  for(let c=0;c<3;c++)preview[p+c]=Math.round(original[p+c]*(1-blend)+plate[g+c]*blend);
 }
 await sprite('preview',preview,[810,243,642,325],rounded(26));
 // Each row is an empty nine-slice surface. Sample its unoccupied gutter to
 // clear illustrations and controls while retaining the actual source rim.
 for(const [name,bounds] of [['row-selected',[334,191,442,136]],['row',[335,475,441,156]]]) {
  const [x,y,w,h]=bounds,data=await sharp(original,{raw:{width:W,height:H,channels:4}}).extract(box(bounds)).raw().toBuffer();
  for(let yy=10;yy<h-11;yy++)for(let xx=14;xx<w-14;xx++){
   const p=(yy*w+xx)*4,g=((y+yy-58)*1220+497-286)*4;
   for(let c=0;c<3;c++)data[p+c]=plate[g+c];
  }
  for(let yy=0;yy<h;yy++)for(let xx=0;xx<w;xx++)if(!rounded(27)(xx+.5,yy+.5,w,h))data[(yy*w+xx)*4+3]=0;
  await save(name,data,w,h,bounds);
 }
 // The upgrade area is a separate empty surface so MAX and locked states
 // never leave old text, prices or active buttons underneath them.
 const bounds=[809,711,644,215],upgrade=await sharp(original,{raw:{width:W,height:H,channels:4}}).extract(box(bounds)).raw().toBuffer();
 for(let y=5;y<210;y++)for(let x=7;x<637;x++){
  const p=(y*644+x)*4,g=((Math.min(811,716+y)-58)*1220+819-286)*4;
  for(let c=0;c<3;c++)upgrade[p+c]=plate[g+c];
 }
 for(let y=0;y<215;y++)for(let x=0;x<644;x++)if(!rounded(27)(x+.5,y+.5,644,215))upgrade[(y*644+x)*4+3]=0;
 await save('upgrade',upgrade,644,215,bounds);
 const manifestPath=path.join(root,'assets/manifest.json'),manifest=JSON.parse(await fs.readFile(manifestPath,'utf8'));
 for(const [file,{crop,...entry}] of Object.entries(entries))manifest.assets[file]=entry;
 await fs.writeFile(manifestPath,JSON.stringify(manifest,null,2)+'\n');
 await fs.writeFile(path.join(out,'provenance.json'),JSON.stringify({reference:'work/tank-menu-reference/reference.png',cleanPlate:'work/tank-menu-reference/generated-clean-plate.png',prompt:'work/tank-menu-reference/imagegen-prompt.txt',assets:entries},null,2)+'\n');
 console.log('Prepared '+Object.keys(entries).length+' tank menu sprites.');
}
main().catch(error=>{console.error(error);process.exitCode=1;});
