// Slice Image Gen's transparent parts sheets without repainting the artwork.
const fs = require('node:fs/promises');
const path = require('node:path');
const sharp = require('sharp');

const root = __dirname;
const names = ['portrait', 'body', 'tail', 'dorsal', 'side', 'belly', 'near-closed', 'far-closed', 'mouth-open'];
// Image Gen placed the first row across uneven column widths. These reviewed
// source rectangles isolate the three cutouts without cutting their silhouettes.
const firstRow = {
  neonTetra: [[0, 0, 490, 420], [490, 0, 430, 420], [935, 0, 319, 420]],
  guppy: [[0, 0, 445, 460], [450, 0, 435, 460], [900, 0, 354, 460]],
};

function retainSprite(raw, width, height) {
  const labels = new Int32Array(width * height);
  const queue = new Int32Array(width * height);
  const sizes = [0];
  let label = 0;
  for (let start = 0; start < labels.length; start++) {
    if (labels[start] || raw[start * 4 + 3] < 32) continue;
    label++;
    let read = 0, write = 1;
    queue[0] = start; labels[start] = label;
    while (read < write) {
      const at = queue[read++], x = at % width, y = Math.floor(at / width);
      for (let dy = -1; dy <= 1; dy++) for (let dx = -1; dx <= 1; dx++) {
        if (x + dx < 0 || x + dx >= width || y + dy < 0 || y + dy >= height) continue;
        const next = at + dy * width + dx;
        if (!labels[next] && raw[next * 4 + 3] >= 32) { labels[next] = label; queue[write++] = next; }
      }
    }
    sizes[label] = write;
  }
  const minimum = Math.max(60, Math.max(...sizes) * .0015);
  const keep = new Uint8Array(labels.length);
  for (let i = 0; i < labels.length; i++) if (sizes[labels[i]] >= minimum) {
    const x = i % width, y = Math.floor(i / width);
    // Keep each retained component's original soft alpha edge as well.
    for (let dy = -2; dy <= 2; dy++) for (let dx = -2; dx <= 2; dx++) {
      if (x + dx >= 0 && x + dx < width && y + dy >= 0 && y + dy < height) keep[i + dy * width + dx] = 1;
    }
  }
  for (let i = 0; i < keep.length; i++) if (!keep[i]) raw[i * 4 + 3] = 0;
  return raw;
}

async function main() {
  const result = {};
  for (const species of ['neonTetra', 'guppy']) {
    const source = path.join(root, 'source', species + '.png');
    const { data, info } = await sharp(source).ensureAlpha().raw().toBuffer({ resolveWithObject: true });
    let transparent = 0;
    for (let i = 3; i < data.length; i += 4) if (data[i] === 0) transparent++;
    if (transparent < info.width * info.height * 0.05) {
      throw new Error(species + ': source has no usable transparent background; regenerate it before slicing.');
    }
    const directory = path.join(root, 'parts', species);
    await fs.mkdir(directory, { recursive: true });
    const parts = {};
    for (let index = 0; index < names.length; index++) {
      const region = index < 3 ? firstRow[species][index] : [Math.round(index % 3 * info.width / 3), index < 6 ? 480 : 840, Math.round(info.width / 3), index < 6 ? 330 : info.height - 840];
      const [x0, y0, width, height] = region;
      const raw = await sharp(source).extract({ left: x0, top: y0, width, height }).ensureAlpha().raw().toBuffer();
      retainSprite(raw, width, height);
      let left = width, top = height, right = -1, bottom = -1;
      for (let y = 0; y < height; y++) for (let x = 0; x < width; x++) {
        if (raw[(y * width + x) * 4 + 3] > 8) {
          left = Math.min(left, x); right = Math.max(right, x);
          top = Math.min(top, y); bottom = Math.max(bottom, y);
        }
      }
      if (right < left) throw new Error(species + ': missing cell ' + names[index]);
      if (left <= 0 || right >= width - 1 || top <= 0 || bottom >= height - 1) {
        throw new Error(species + ': sprite crosses cell boundary: ' + names[index]);
      }
      left = Math.max(0, left - 3); top = Math.max(0, top - 3);
      right = Math.min(width - 1, right + 3); bottom = Math.min(height - 1, bottom + 3);
      const bounds = { left, top, width: right - left + 1, height: bottom - top + 1 };
      const file = 'parts/' + species + '/' + names[index] + '.png';
      await sharp(raw, { raw: { width, height, channels: 4 } }).extract(bounds).png().toFile(path.join(root, file));
      parts[names[index]] = { file, bounds: { ...bounds, left: x0 + left, top: y0 + top } };
    }
    result[species] = { source: 'source/' + species + '.png', width: info.width, height: info.height, transparentFraction: transparent / (info.width * info.height), parts };
  }
  await fs.writeFile(path.join(root, 'extraction.json'), JSON.stringify(result, null, 2) + '\n');
  console.log(JSON.stringify(result, null, 2));
}
main().catch(error => { console.error(error); process.exitCode = 1; });
