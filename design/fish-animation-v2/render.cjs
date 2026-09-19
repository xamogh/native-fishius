const fs = require('node:fs/promises');
const path = require('node:path');
const { spawn } = require('node:child_process');
const { once } = require('node:events');
const { createCanvas, loadImage, GlobalFonts } = require('@napi-rs/canvas');
const root = __dirname;

async function main() {
  const { drawFish, drawStage, poseFor } = await import('./animation.mjs');
  GlobalFonts.registerFromPath(path.join(root, 'fonts/Nunito-SemiBold.ttf'), 'Nunito');
  const rigs = JSON.parse(await fs.readFile(path.join(root, 'rigs.json'), 'utf8'));
  const images = {};
  for (const [id, rig] of Object.entries(rigs.species)) {
    images[id] = {};
    for (const name of Object.keys(rig.parts)) images[id][name] = await loadImage(path.join(root, 'parts', id, name + '.png'));
  }
  const canvas = createCanvas(1080, 680), ctx = canvas.getContext('2d');
  drawStage(ctx, rigs, images, 0);
  await fs.writeFile(path.join(root, 'poster.png'), await canvas.encode('png'));
  const review = createCanvas(1440, 760), rc = review.getContext('2d');
  rc.fillStyle = '#e6f6f4'; rc.fillRect(0, 0, review.width, review.height);
  const poses = ['Rest', 'Blink closed', 'Feeding gulp'];
  for (let row = 0; row < 2; row++) for (let column = 0; column < 3; column++) {
    const id = row ? 'guppy' : 'neonTetra';
    const pose = poseFor(id, 0, 'idle');
    if (column === 1) pose.blink = 1;
    if (column === 2) pose.gulp = 1;
    rc.fillStyle = '#234e5c'; rc.textAlign = 'center'; rc.font = '700 18px Nunito';
    rc.fillText(rigs.species[id].name + ' / ' + poses[column], column * 480 + 240, row * 380 + 42);
    drawFish(rc, rigs.species[id], images[id], pose, column * 480, row * 380 + 45, 480, 315);
  }
  await fs.writeFile(path.join(root, 'contact-sheet.png'), await review.encode('png'));
  if (!process.argv.includes('--all')) { console.log('Rendered poster and pose review.'); return; }

  const exportsDir = path.join(root, 'sprites');
  await fs.mkdir(exportsDir, { recursive: true });
  const frameWidth = 384, frameHeight = 256, fps = 24, columns = 8;
  const clips = { idle: 48, swim: 48, blink: 12, feed: 18 };
  const qa = { alpha: true, noClippedFrames: true, loopEndpoints: {}, frameCount: 0, maxBorderAlpha: 0 };
  for (const [id, rig] of Object.entries(rigs.species)) {
    const metadata = { species: id, facing: rig.facing, frameWidth, frameHeight, anchor: [.5, .5], fps, alpha: 'straight', clips: {} };
    for (const [mode, count] of Object.entries(clips)) {
      const atlas = createCanvas(frameWidth * columns, frameHeight * Math.ceil(count / columns)), ac = atlas.getContext('2d');
      const frame = createCanvas(frameWidth, frameHeight), fc = frame.getContext('2d');
      const frames = [];
      for (let index = 0; index < count; index++) {
        fc.clearRect(0, 0, frameWidth, frameHeight);
        drawFish(fc, rig, images[id], poseFor(id, index / fps, mode), 0, 2, frameWidth, 252);
        const pixels = fc.getImageData(0, 0, frameWidth, frameHeight).data;
        for (let x = 0; x < frameWidth; x++) qa.maxBorderAlpha = Math.max(qa.maxBorderAlpha, pixels[x * 4 + 3], pixels[((frameHeight - 1) * frameWidth + x) * 4 + 3]);
        for (let y = 0; y < frameHeight; y++) qa.maxBorderAlpha = Math.max(qa.maxBorderAlpha, pixels[y * frameWidth * 4 + 3], pixels[(y * frameWidth + frameWidth - 1) * 4 + 3]);
        const x = index % columns * frameWidth, y = Math.floor(index / columns) * frameHeight;
        ac.drawImage(frame, x, y);
        frames.push({ x, y, w: frameWidth, h: frameHeight, durationMs: 1000 / fps });
        qa.frameCount++;
      }
      if (mode === 'idle' || mode === 'swim') {
        const a = createCanvas(frameWidth, frameHeight), b = createCanvas(frameWidth, frameHeight);
        drawFish(a.getContext('2d'), rig, images[id], poseFor(id, 0, mode), 0, 2, frameWidth, 252);
        drawFish(b.getContext('2d'), rig, images[id], poseFor(id, count / fps, mode), 0, 2, frameWidth, 252);
        const ap = a.getContext('2d').getImageData(0, 0, frameWidth, frameHeight).data, bp = b.getContext('2d').getImageData(0, 0, frameWidth, frameHeight).data;
        let total = 0; for (let i = 0; i < ap.length; i++) total += Math.abs(ap[i] - bp[i]);
        const meanError = total / ap.length;
        qa.loopEndpoints[id + '/' + mode] = { meanChannelError: meanError, pass: meanError < .01 };
        if (meanError >= .01) throw new Error(id + '/' + mode + ': loop does not join cleanly');
      }
      const file = id + '-' + mode + '.png';
      await fs.writeFile(path.join(exportsDir, file), await atlas.encode('png'));
      metadata.clips[mode] = { file, loop: mode === 'idle' || mode === 'swim', durationMs: count / fps * 1000, frames };
      console.log('Exported', file, count, 'frames');
    }
    await fs.writeFile(path.join(exportsDir, id + '.json'), JSON.stringify(metadata, null, 2) + '\n');
  }
  if (qa.maxBorderAlpha > 0) throw new Error('An exported frame touches its outer border');
  await fs.writeFile(path.join(root, 'qa.json'), JSON.stringify(qa, null, 2) + '\n');

  const ffmpeg = spawn('ffmpeg', ['-y', '-loglevel', 'error', '-f', 'rawvideo', '-pixel_format', 'rgba', '-video_size', '1080x680', '-framerate', '24', '-i', 'pipe:0', '-an', '-c:v', 'libx264', '-preset', 'fast', '-crf', '19', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', path.join(root, 'fish-animation-preview.mp4')], { stdio: ['pipe', 'ignore', 'pipe'] });
  let errors = ''; ffmpeg.stderr.on('data', data => { errors += data; });
  const done = new Promise((resolve, reject) => { ffmpeg.on('error', reject); ffmpeg.on('close', code => code === 0 ? resolve() : reject(new Error(errors))); });
  for (let frame = 0; frame < 12 * fps; frame++) {
    drawStage(ctx, rigs, images, frame / fps);
    const rgba = ctx.getImageData(0, 0, canvas.width, canvas.height).data;
    if (!ffmpeg.stdin.write(Buffer.from(rgba.buffer, rgba.byteOffset, rgba.byteLength))) await once(ffmpeg.stdin, 'drain');
    if (frame % 96 === 0) console.log('Rendering preview', Math.round(frame / fps), '/ 12 seconds');
  }
  ffmpeg.stdin.end(); await done;
  console.log('Preview video and sprite sheets complete.');
}
main().catch(error => { console.error(error); process.exitCode = 1; });
