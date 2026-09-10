const sharp=require('/Users/amoghrijal/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
const fs=require('fs');
const sx=2498/1608,sy=1290/830;
const defs={
 tanks:{box:[0,688,304,142],body:'M 20 110 C 18 97 17 75 22 58 C 27 41 37 34 49 32 C 63 30 91 30 98 34 C 108 39 115 46 118 55 L 231 55 C 247 55 261 69 263 83 L 263 108 C 261 121 252 128 239 130 L 118 130 C 113 135 107 137 97 136 L 55 136 C 38 133 26 124 20 110 Z',plants:[
 'M 21 107 C 16 102 11 93 8 83 C 5 75 3 66 1 59 C 0 55 2 51 4 51 C 7 51 7 59 10 61 C 12 66 14 69 15 62 C 15 56 17 49 15 43 C 12 38 11 34 12 29 C 13 23 17 18 21 20 C 25 22 26 28 24 34 C 22 40 20 42 22 48 L 28 44 C 28 46 30 48 29 53 L 24 65 L 23 92 Z',
 'M 229 128 C 239 124 250 116 258 102 C 261 96 264 89 264 84 C 260 83 262 76 264 72 C 267 68 272 70 275 74 C 279 83 272 89 272 98 C 275 99 278 94 280 93 C 285 90 289 93 287 100 C 283 112 273 119 267 122 C 269 127 266 134 262 137 C 255 142 250 141 243 139 C 235 141 226 139 224 136 L 224 131 Z'
 ],bubbles:[[50.2,19.7,12.1,13.1],[30.2,27.7,7.2,8],[253.1,53.1,9.6,10.4],[264.4,66.1,4.8,5.2],[283.0,83.2,3.2,3.5],[287.8,127.8,8.6,9.1],[266.4,131.0,6.1,6.2]]},
 shop:{box:[1248,680,360,150],body:'M 45 83 C 43 70 47 57 53 50 C 60 42 68 38 77 33 C 86 19 101 14 121 14 C 145 14 164 21 174 34 L 303 34 C 321 34 333 44 336 59 C 340 77 341 96 335 109 C 330 120 323 126 313 129 L 188 129 C 178 134 156 135 141 133 L 89 133 C 72 132 58 121 51 108 C 46 99 45 91 45 83 Z',plants:[
 'M 37 111 C 32 106 31 100 30 95 C 23 90 27 83 29 78 C 28 74 24 69 24 61 C 24 55 26 49 30 47 C 36 45 41 49 42 55 C 44 64 39 72 42 77 C 45 78 43 70 48 68 C 53 67 58 71 58 76 C 58 83 49 88 47 94 C 48 97 54 97 60 95 C 62 91 67 95 67 100 L 74 106 C 77 99 81 97 85 100 C 92 101 93 109 89 114 L 88 120 C 88 125 94 126 94 118 C 94 112 98 111 103 113 C 108 116 107 122 104 128 C 99 134 105 137 113 135 C 122 132 124 127 129 128 C 136 131 137 136 133 141 C 126 148 119 150 110 149 C 104 151 96 150 91 148 L 55 139 Z',
 'M 34 124 C 32 119 30 115 33 110 C 35 105 40 106 44 109 C 48 113 46 119 50 121 L 57 125 C 58 120 54 115 53 111 L 52 98 C 48 90 51 84 57 83 C 65 82 68 87 68 93 C 68 101 64 108 67 115 L 69 125 C 72 126 73 119 72 116 C 72 110 75 107 79 108 C 85 111 85 116 82 121 C 80 126 81 133 84 134 C 86 133 83 128 87 126 C 91 123 96 125 96 129 C 97 135 92 138 96 140 C 99 139 103 141 103 145 C 103 148 100 150 96 150 C 81 151 75 148 68 146 C 64 148 54 145 50 142 C 43 140 36 135 35 132 C 39 132 42 135 45 136 C 42 132 37 129 34 124 Z'
 ],bubbles:[[47.4,40.5,8.2,9.4],[26.4,96.8,9.4,10.5],[28.6,133.2,7.6,7.9],[155.9,139.6,6.6,7.2],[330.1,28.2,13.6,14.5],[345.6,49.7,5.9,6.2],[257.2,26.4,5.1,6.1]]}
};
(async()=>{let report={};for(const[n,d]of Object.entries(defs)){
 const [x,y,w,h]=d.box,left=Math.round(x*sx),top=Math.round(y*sy),W=Math.min(2498-left,Math.round(w*sx)),H=Math.min(1290-top,Math.round(h*sy));
 const crop={left,top,width:W,height:H};const {data}=await sharp('design/reference/aquarium.png').extract(crop).ensureAlpha().raw().toBuffer({resolveWithObject:true});
 const svg=s=>`<svg width="${W}" height="${H}" viewBox="0 0 ${w} ${h}">${s}</svg>`;
 const layer=async s=>{let{data:b}=await sharp(Buffer.from(svg(s))).ensureAlpha().raw().toBuffer({resolveWithObject:true});let a=new Uint8Array(W*H);for(let i=0;i<a.length;i++)a[i]=b[i*4+3];return a};
 const body=await layer(`<path fill="white" d="${d.body}"/>`);
 const plants=await layer(d.plants.map(p=>`<path fill="white" d="${p}"/>`).join(''));
 const extra=n==='shop'?await layer('<path fill="white" d="M 44 127 C 49 130 55 133 62 138 C 72 143 82 145 90 146 L 89 149 C 74 150 64 148 57 145 C 49 140 46 133 44 127 Z"/>'):new Uint8Array(W*H);
 const bubbles=await layer(d.bubbles.map(([cx,cy,rx,ry])=>`<ellipse fill="white" cx="${cx}" cy="${cy}" rx="${rx}" ry="${ry}"/>`).join(''));
 // Remove blue water from gaps between the foliage branches. Fill closed highlights.
 const N=W*H,mask=new Uint8Array(N),plantPixels=new Uint8Array(N),seen=new Uint8Array(N),q=new Int32Array(N);let head=0,tail=0;
 const vegetation=i=>{let p=i*4,r=data[p],g=data[p+1],b=data[p+2];return (g>r*1.1&&g>b*.83)||(n==='shop'&&r>g*.8&&b>g*1.25&&b>r*.85)};
 for(let i=0;i<N;i++)if(plants[i]&&vegetation(i))plantPixels[i]=plants[i];
 const add=i=>{if(i<0||i>=N||seen[i]||plantPixels[i]>0)return;seen[i]=1;q[tail++]=i};for(let i=0;i<W;i++){add(i);add((H-1)*W+i)}for(let y=0;y<H;y++){add(y*W);add(y*W+W-1)}
 while(head<tail){let i=q[head++],x=i%W;if(x)add(i-1);if(x<W-1)add(i+1);add(i-W);add(i+W)}
 for(let i=0;i<N;i++){if(!seen[i]&&plants[i])plantPixels[i]=plants[i];mask[i]=Math.max(body[i],plantPixels[i],bubbles[i],extra[i]);data[i*4+3]=mask[i]}
 // Remove device pixels that touch the outside of the traced body.
 const blocked=new Uint8Array(N);head=0;tail=0;
 const trim=i=>{if(i<0||i>=N||blocked[i])return;let p=i*4;if(mask[i]>0&&!(data[p+1]<74&&data[p+2]<90))return;blocked[i]=1;q[tail++]=i};
 for(let i=0;i<W;i++){trim(i);trim((H-1)*W+i)}for(let y=0;y<H;y++){trim(y*W);trim(y*W+W-1)}while(head<tail){let i=q[head++],x=i%W;if(x)trim(i-1);if(x<W-1)trim(i+1);trim(i-W);trim(i+W)}for(let i=0;i<N;i++)if(blocked[i])data[i*4+3]=0;
 // Antialias hard segmentation boundaries inward without altering source RGB.
 const edgeAlpha=Uint8Array.from({length:N},(_,i)=>data[i*4+3]);for(let yy=1;yy<H-1;yy++)for(let xx=1;xx<W-1;xx++){let i=yy*W+xx;if(!edgeAlpha[i])continue;let sum=edgeAlpha[i]*4+edgeAlpha[i-1]+edgeAlpha[i+1]+edgeAlpha[i-W]+edgeAlpha[i+W];data[i*4+3]=Math.min(edgeAlpha[i],Math.round(sum/8));}
 await sharp(data,{raw:{width:W,height:H,channels:4}}).png().toFile(`assets/aquarium/${n}.png`);
 const preview=await sharp({create:{width:W,height:H,channels:3,background:'#04c7ef'}}).composite([{input:`assets/aquarium/${n}.png`}]).png().toBuffer();await sharp(preview).resize({width:W*2}).png().toFile(`work/aquarium-match/${n}-preview.png`);
 report[n]={sourceCrop:crop,normalizedCrop:d.box,size:[W,H]};
 }fs.writeFileSync('work/aquarium-match/bottom-button-assets.json',JSON.stringify(report,null,2));console.log(report);
})().catch(e=>{console.error(e);process.exitCode=1})
