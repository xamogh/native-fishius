// Package the current generated egg sprite.
const fs = require('node:fs/promises');
const path = require('node:path');
const crypto = require('node:crypto');
const sharp = require(process.env.SHARP_MODULE || 'sharp');
const root = path.resolve(__dirname, '..');
const work = path.join(root, 'work/control-refresh');

async function main() {
  const manifestPath = path.join(root, 'assets/manifest.json');
  const manifest = JSON.parse(await fs.readFile(manifestPath, 'utf8'));
  // Keep real transparency and the round sprite's native proportions.
  await sharp(path.join(work, 'generated/egg.png')).resize(256, 256).png().toFile(path.join(root, 'assets/ui/egg.png'));
  for (const asset of ['ui/egg.png']) {
    const bytes = await fs.readFile(path.join(root, 'assets', asset));
    const meta = await sharp(bytes).metadata();
    manifest.assets[asset] = {
      ...manifest.assets[asset], size: [meta.width, meta.height],
      source: 'Built-in imagegen; prompts and integration record in work/control-refresh/provenance.json',
      sha256: crypto.createHash('sha256').update(bytes).digest('hex'),
    };
  }
  await fs.writeFile(manifestPath, JSON.stringify(manifest, null, 2) + '\n');
  console.log('Prepared the egg asset.');
}
main().catch(error => { console.error(error); process.exitCode = 1; });
