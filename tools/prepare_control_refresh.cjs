// Integrate the generated edits while retaining the original button silhouettes.
const fs = require('node:fs/promises');
const path = require('node:path');
const crypto = require('node:crypto');
const sharp = require(process.env.SHARP_MODULE || 'sharp');
const root = path.resolve(__dirname, '..');
const work = path.join(root, 'work/control-refresh');

async function main() {
  const manifestPath = path.join(root, 'assets/manifest.json');
  const manifest = JSON.parse(await fs.readFile(manifestPath, 'utf8'));
  // The generated files include only the requested replacements here. Using
  // patches keeps every other original pixel and the full alpha mask intact.
  const patches = {
    tanks: { x: 334, y: 112, w: 64, h: 66, feather: 4 },
    shop: { x: 444, y: 94, w: 67, h: 69, feather: 3 },
    sell: { x: 34, y: 33, w: 134, h: 109, feather: 5 },
  };
  for (const [name, box] of Object.entries(patches)) {
    const { data, info } = await sharp(path.join(work, 'originals', name + '.png')).ensureAlpha().raw().toBuffer({ resolveWithObject: true });
    const edit = await sharp(path.join(work, 'generated', name + '.png')).resize(info.width, info.height, { fit: 'fill' }).ensureAlpha().raw().toBuffer();
    for (let y = box.y; y < box.y + box.h; y++) {
      for (let x = box.x; x < box.x + box.w; x++) {
        const edge = Math.min(x - box.x, box.x + box.w - 1 - x, y - box.y, box.y + box.h - 1 - y);
        const blend = Math.min(1, edge / box.feather);
        const i = (y * info.width + x) * 4;
        for (let c = 0; c < 3; c++) data[i + c] = Math.round(data[i + c] * (1 - blend) + edit[i + c] * blend);
      }
    }
    await sharp(data, { raw: { width: info.width, height: info.height, channels: 4 } }).png().toFile(path.join(root, 'assets/aquarium', name + '.png'));
  }
  // Keep real transparency and the round sprite's native proportions.
  await sharp(path.join(work, 'generated/egg.png')).resize(256, 256).png().toFile(path.join(root, 'assets/ui/egg.png'));
  for (const asset of ['aquarium/tanks.png', 'aquarium/shop.png', 'aquarium/sell.png', 'ui/egg.png']) {
    const bytes = await fs.readFile(path.join(root, 'assets', asset));
    const meta = await sharp(bytes).metadata();
    manifest.assets[asset] = {
      ...manifest.assets[asset], size: [meta.width, meta.height],
      source: 'Built-in imagegen; prompts and integration record in work/control-refresh/provenance.json',
      sha256: crypto.createHash('sha256').update(bytes).digest('hex'),
    };
  }
  await fs.writeFile(manifestPath, JSON.stringify(manifest, null, 2) + '\n');
  console.log('Prepared Tanks, Shop, Sell and egg assets.');
}
main().catch(error => { console.error(error); process.exitCode = 1; });
