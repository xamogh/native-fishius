// Prepare the supplied artwork as independent native dialog layers. ImageGen
// supplies concealed clean backing and the pearl variant. All other visible
// illustration and control pixels come directly from the user's reference.
const fs = require('node:fs/promises');
const path = require('node:path');
const sharp = require(process.env.SHARP_MODULE || '/Users/amoghrijal/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
const root = path.resolve(__dirname, '..');
const work = path.join(root, 'work/general-dialog');
const out = path.join(root, 'assets/general-dialog');
const W = 888, H = 732;
const crop = (left, top, width, height) => ({left, top, width, height});
const svgMask = (w,h,shape) => Buffer.from(`<svg xmlns="http://www.w3.org/2000/svg" width="${w}" height="${h}" viewBox="0 0 ${w} ${h}">${shape}</svg>`);
const rectMask = (w,h) => svgMask(w,h,`<rect width="${w}" height="${h}" rx="8" fill="white"/>`);
async function main(){
 await fs.mkdir(out,{recursive:true});
 const source = await sharp(path.join(work,'reference.png')).ensureAlpha().png().toBuffer();
 const clean = await sharp(path.join(work,'clean-generated.png')).resize(W,H,{fit:'fill'}).ensureAlpha().png().toBuffer();
 const underlay = await sharp(path.join(work,'underlay/blank-panel-generated.png')).resize(W,H,{fit:'fill'}).ensureAlpha().png().toBuffer();
 const buttonMask = svgMask(652,135,`<path fill="white" d="M55 2 L596 2 C632 2 650 22 650 62 L650 70 C650 110 632 128 598 128 L56 128 C19 128 2 109 2 70 L2 60 C2 22 20 2 55 2 Z"/>`);
 const buttonBackingMask = svgMask(656,139,`<path fill="white" stroke="white" stroke-width="7" d="M57 4 L598 4 C634 4 652 24 652 64 L652 72 C652 112 634 130 600 130 L58 130 C21 130 4 111 4 72 L4 62 C4 24 22 4 57 4 Z"/>`);
 const patch = async (box) => ({input:await sharp(clean).extract(box).composite([{input:rectMask(box.width,box.height),blend:'dest-in'}]).png().toBuffer(),left:box.left,top:box.top});
 // Clean the editable title, illustration and message slots. Keep the exact
 // outer frame, bubbles, wood outline and all unedited source detail.
 let frame = await sharp(source).composite([
  await patch(crop(198,20,502,75)),
  await patch(crop(97,122,703,415)),
  {input:await sharp(underlay).extract(crop(117,541,656,139)).composite([{input:buttonBackingMask,blend:'dest-in'}]).png().toBuffer(),left:117,top:541},
  {input:await sharp(underlay).extract(crop(762,41,103,104)).png().toBuffer(),left:762,top:41}
 ]).png().toBuffer();
 // This path is only a cutout mask of the existing raster silhouette.
 const silhouette = svgMask(W,H,`
  <path fill="white" d="M137 66 C102 65 76 76 61 95 C47 113 44 147 43 192 C39 320 38 482 42 592 C44 643 45 671 67 693 C89 714 249 715 445 715 C626 715 755 715 798 704 C836 693 844 656 847 600 C852 486 852 270 841 136 C835 91 807 75 780 70 L759 68 L759 53 C759 21 743 14 714 10 C609 -2 283 -3 166 12 C144 17 134 34 137 66 Z"/>
  <circle cx="83" cy="73" r="22" fill="white"/>
 `);
 frame = await sharp(frame).composite([{input:silhouette,blend:'dest-in'}]).png().toBuffer();
 await fs.writeFile(path.join(out,'frame.png'),frame);

 const hero = crop(100,123,696,284);
 const heroCorner = {input:await sharp(underlay).extract(crop(774,123,22,24)).png().toBuffer(),left:674,top:0};
 await sharp(source).extract(hero).composite([heroCorner]).png().toFile(path.join(out,'coins.png'));
 const pearl = await sharp(path.join(work,'pearl/pearl-dialog-generated.png')).resize(W,H,{fit:'fill'}).ensureAlpha().png().toBuffer();
 await sharp(pearl).extract(hero).composite([heroCorner]).png().toFile(path.join(out,'pearls.png'));
 const button = await sharp(source).extract(crop(119,543,652,135)).png().toBuffer();
 await sharp(button).composite([{input:buttonMask,blend:'dest-in'}]).png().toFile(path.join(out,'open-shop.png'));
 const strip = await sharp(button).extract(crop(76,0,38,135)).resize(458,135,{fit:'fill'}).png().toBuffer();
 await sharp(button).composite([{input:strip,left:118,top:0},{input:buttonMask,blend:'dest-in'}]).png().toFile(path.join(out,'button.png'));
 const closeMask = svgMask(100,100,'<circle cx="50" cy="49" r="48" fill="white"/>');
 await sharp(source).extract(crop(764,43,100,100)).composite([{input:closeMask,blend:'dest-in'}]).png().toFile(path.join(out,'close.png'));
 await fs.writeFile(path.join(out,'provenance.json'),JSON.stringify({
  reference:'work/general-dialog/reference.png',referencePixels:[W,H],
  generatedBacking:'work/general-dialog/clean-generated.png',
  generatedControlBacking:'work/general-dialog/underlay/blank-panel-generated.png',
  generatedPearls:'work/general-dialog/pearl/pearl-dialog-generated.png',
  tool:'Built-in ImageGen',script:'tools/prepare_general_dialog.cjs',
  hero:[hero.left,hero.top,hero.width,hero.height],
  note:'Source frame and controls retained; editable slots cleaned with ImageGen; pearl hero generated from source. No image is enlarged.'
 },null,2)+'\n');
}
main().catch(e=>{console.error(e);process.exit(1);});
