// Cut the reference's painted controls into runtime layers. ImageGen supplies
// only the blank material behind editable labels; visible icon pixels are kept.
const fs = require('node:fs/promises');
const path = require('node:path');
const sharp = require(process.env.SHARP_MODULE || '/Users/amoghrijal/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
const root = path.resolve(__dirname, '..');
const work = path.join(root, 'work/fish-popover-match');
const out = path.join(root, 'assets/fish-popover');
const box = (left,top,width,height) => ({left,top,width,height});
const mask = (w,h,shape) => Buffer.from(`<svg xmlns="http://www.w3.org/2000/svg" width="${w}" height="${h}">${shape}</svg>`);
const rounded = (w,h,r) => mask(w,h,`<rect width="${w}" height="${h}" rx="${r}" fill="white"/>`);
async function main(){
 await fs.mkdir(out,{recursive:true});
 const source=await sharp(path.join(work,'reference.png')).ensureAlpha().png().toBuffer();
 const clean=await sharp(path.join(work,'clean-widgets-generated.png')).resize(1338,1002,{fit:'fill'}).ensureAlpha().png().toBuffer();
 const cut = async (name,rect,alpha,input=source) => {
  let layer=sharp(input).extract(rect);if(alpha)layer=layer.composite([{input:alpha,blend:'dest-in'}]);
  await layer.png().toFile(path.join(out,name+'.png'));
 };
 // Alpha masks trace existing raster silhouettes. They never draw replacement art.
 await cut('close',box(779,155,100,100),mask(100,100,'<ellipse cx="49" cy="50" rx="47" ry="48" fill="white"/>'));
 await cut('heart',box(700,217,60,53),mask(60,53,'<path d="M29 9 C17 -4 3 0 2 15 C0 30 18 42 28 49 C32 52 38 44 45 37 C56 27 61 13 53 5 C46 -2 36 1 29 9 Z" fill="white"/>'));
 await cut('clock',box(275,457,47,45),mask(47,45,'<ellipse cx="22" cy="22" rx="21" ry="21" fill="white"/>'));
 await cut('info',box(251,690,45,44),mask(45,44,'<circle cx="22" cy="21" r="20" fill="white"/>'));
 await cut('coin',box(487,536,55,55),mask(55,55,'<ellipse cx="27" cy="27" rx="26" ry="26" fill="white"/>'));
 await cut('xp',box(672,536,54,53),mask(54,53,'<path d="M27 2 C32 1 34 10 36 14 L47 17 C55 18 50 26 43 32 L44 43 C46 51 39 52 28 46 C19 50 11 52 11 44 L12 34 C4 28 -1 22 4 19 L18 15 C20 9 21 2 27 2 Z" fill="white"/>'));
 await cut('stage-current',box(258,390,47,49),mask(47,49,'<ellipse cx="24" cy="24" rx="22" ry="23" fill="white"/>'));
 await cut('stage-next',box(397,395,37,38),mask(37,38,'<ellipse cx="18" cy="19" rx="17" ry="17" fill="white"/>'));
 await cut('growth-track',box(304,401,90,25));
 // The original glossy button, including its bag, keeps the source RGB. Only
 // the editable word is replaced with clean, unlettered blue button material.
 const button=await sharp(source).extract(box(552,664,300,107)).png().toBuffer();
 const pixels=await sharp(button).raw().toBuffer(),strip=Buffer.alloc(168*74*4);
 for(let y=0;y<74;y++)for(let x=0;x<168;x++)for(let c=0;c<4;c++){
  const t=x/167,a=((y+13)*300+108)*4+c,b=((y+13)*300+278)*4+c;
  strip[(y*168+x)*4+c]=Math.round(pixels[a]*(1-t)+pixels[b]*t);
 }
 const blankStrip=await sharp(strip,{raw:{width:168,height:74,channels:4}}).png().toBuffer();
 const buttonMask=mask(300,107,'<path d="M44 2 L253 2 C284 2 297 17 298 49 L298 60 C297 92 282 102 253 102 L44 102 C14 102 2 87 2 59 L2 48 C2 18 15 3 44 2 Z" fill="white"/>');
 await sharp(button).composite([{input:blankStrip,left:109,top:13},{input:buttonMask,blend:'dest-in'}]).png().toFile(path.join(out,'rehome-button.png'));
 const buttonStrip=await sharp(button).extract(box(108,0,4,107)).resize(217,107,{fit:'fill'}).png().toBuffer();
 await sharp(button).composite([{input:buttonStrip,left:42,top:0},{input:buttonMask,blend:'dest-in'}]).png().toFile(path.join(out,'button.png'));
 await cut('bag',box(588,677,66,77),mask(66,77,'<path d="M18 5 L25 9 L33 3 L42 8 L37 20 L49 31 C59 41 63 58 59 66 C58 72 48 75 28 74 C10 73 1 68 2 61 C1 44 12 26 25 21 Z" fill="white"/>'));
 // Clean card layers match the reference dimensions and retain its soft fills.
 const growth=await sharp(clean).extract(box(237,326,619,189)).png().toBuffer();
 const timerBacking=await sharp(growth).extract(box(109,124,58,58)).png().toBuffer();
 await sharp(growth).composite([{input:timerBacking,left:28,top:124},{input:rounded(619,189,36),blend:'dest-in'}]).png().toFile(path.join(out,'growth-card.png'));
 const rewards=await sharp(clean).extract(box(237,526,619,135)).resize(619,139,{fit:'fill'}).png().toBuffer();
 // The text-free generated card retains coin/star artwork. Erase those slots
 // using clean material from their own row so rewards can use exact source icons.
 const patches=[];
 for(const y of [6,70])for(const x of [241,427])patches.push({input:await sharp(rewards).extract(box(335,y,68,63)).png().toBuffer(),left:x,top:y});
 await sharp(rewards).composite([...patches,{input:rounded(619,139,29),blend:'dest-in'}]).png().toFile(path.join(out,'reward-card.png'));
 await fs.writeFile(path.join(out,'controls-provenance.json'),JSON.stringify({reference:'work/fish-popover-match/reference.png',referencePixels:[1338,1002],generatedBacking:'work/fish-popover-match/clean-widgets-generated.png',tool:'Built-in ImageGen',script:'tools/prepare_fish_popover.cjs',note:'Separate controls use source pixels. Clean backing supplies only editable card material. Text, amounts and progress remain native and live.'},null,2)+'\n');
 console.log('Prepared reference popover controls.');
}
main().catch(e=>{console.error(e);process.exit(1);});
