// Shared by the interactive preview and deterministic sprite/video export.
// All painted parts come from Image Gen. This module only poses those sprites.
export const TAU = Math.PI * 2;
export const clamp = (n, lo = 0, hi = 1) => Math.min(hi, Math.max(lo, n));
const smooth = x => { x = clamp(x); return x * x * (3 - 2 * x); };

function blinkPulse(time, start, duration) {
  const t = (time - start) / duration;
  if (t < 0 || t >= 1) return 0;
  if (t < .34) return smooth(t / .34);
  if (t < .51) return 1;
  return 1 - smooth((t - .51) / .49);
}

function gulpPulse(time, start, duration) {
  const t = (time - start) / duration;
  if (t < 0 || t >= 1) return 0;
  return t < .36 ? smooth(t / .36) : 1 - smooth((t - .36) / .64);
}

export function poseFor(id, time, mode = 'showcase') {
  const guppy = id === 'guppy';
  const phase = time % 12;
  let swim = mode === 'swim' ? 1 : 0;
  let blink = 0, gulp = 0;
  if (mode === 'showcase') {
    swim = smooth((phase - 4) / .5) * (1 - smooth((phase - 7) / .5));
    const starts = guppy ? [3.1, 9.5] : [2.05, 5.05, 5.34, 10.8];
    for (const start of starts) blink = Math.max(blink, blinkPulse(phase, start, guppy ? .38 : .23));
    gulp = gulpPulse(phase, guppy ? 8.7 : 8.3, guppy ? .72 : .56);
  } else if (mode === 'blink') {
    blink = blinkPulse(time, .035, guppy ? .39 : .31);
  } else if (mode === 'feed') {
    gulp = gulpPulse(time, .04, guppy ? .67 : .60);
  }
  return { time, swim, blink, gulp, guppy };
}

function part(ctx, image, spec, angle = 0, scaleX = 1, scaleY = 1, alpha = 1) {
  const [x, y, w, h] = spec.rect, [px, py] = spec.pivot || [.5, .5];
  ctx.save();
  ctx.translate(x + w * px, y + h * py);
  ctx.rotate(angle * Math.PI / 180);
  ctx.scale(scaleX, scaleY);
  ctx.globalAlpha *= alpha;
  const left = spec.featherLeft || 0, right = spec.featherRight || 0;
  const draw = (start, span, opacity = 1, verticalScale = 1) => {
    ctx.save(); ctx.globalAlpha *= opacity;
    const anchor = spec.rootY || h / 2;
    ctx.drawImage(image, image.width * start / w, 0, image.width * span / w, image.height, -w * px + start, -h * py + anchor * (1 - verticalScale), span, h * verticalScale);
    ctx.restore();
  };
  // Blend only the hidden joint edge. The painted face and body stay intact.
  const blendStep = 1;
  if (left) for (let u = 0; u < left; u += blendStep) {
    const t = (u + .5) / left;
    draw(u, Math.min(blendStep, left - u), smooth(t), 1 - (1 - (spec.rootScale || 1)) * (1 - smooth(t)));
  }
  draw(left, w - left - right);
  if (right) for (let u = 0; u < right; u += blendStep) draw(w - right + u, Math.min(blendStep, right - u), 1 - smooth((u + .5) / right));
  ctx.restore();
}

function eyelid(ctx, image, spec, amount) {
  if (amount <= 0) return;
  const [x, y, w, h] = spec.rect;
  ctx.save();
  // Reveal the painted lid downward over the eye instead of shrinking the eye.
  ctx.beginPath(); ctx.ellipse(x + w / 2, y + h / 2, w * .49, h * .49, 0, 0, TAU); ctx.clip();
  ctx.beginPath(); ctx.rect(x, y, w, h * Math.min(1, amount * 1.08)); ctx.clip();
  part(ctx, image, spec);
  ctx.restore();
}

export function drawFish(ctx, rig, images, pose, x = 0, y = 0, width = 640, height = 420) {
  const { time, swim, blink, gulp, guppy } = pose;
  const parts = rig.parts;
  const direction = rig.facing === 'left' ? -1 : 1;
  const wave = TAU * rig.tailHz * time;
  const bob = Math.sin(TAU * .5 * time) * (guppy ? 2.3 : 2.9);
  const pitch = Math.sin(TAU * .5 * time + .35) * (guppy ? .8 : 1.2);
  ctx.save(); ctx.translate(x, y); ctx.scale(width / 640, height / 420);
  ctx.translate(rig.pivot[0] + direction * gulp * 5, rig.pivot[1] + bob);
  ctx.rotate(pitch * Math.PI / 180);
  const breath = Math.sin(TAU * .5 * time + .5) * .003;
  ctx.scale(1 + breath + gulp * .014, 1 - breath + gulp * .01);
  ctx.translate(-rig.pivot[0], -rig.pivot[1]);
  part(ctx, images.tail, parts.tail, Math.sin(wave) * rig.tailDegrees * (1 + swim * .6), 1 - (guppy ? .075 : .055) * (.5 + .5 * Math.cos(wave)), 1 + Math.sin(wave - .45) * .022);
  part(ctx, images.dorsal, parts.dorsal, Math.sin(wave - .7) * 2.2);
  part(ctx, images.belly, parts.belly, Math.sin(wave + .7) * 5);
  const sideAngle = Math.sin(TAU * (guppy ? 2 : 3) * time + .8) * (guppy ? 9 : 8) * (1 + swim * .25);
  if (parts.side.behind) part(ctx, images.side, parts.side, sideAngle, 1, 1);
  part(ctx, images.body, parts.body);
  if (!parts.side.behind) part(ctx, images.side, parts.side, sideAngle, .96 + Math.sin(wave) * .045, 1);
  eyelid(ctx, images['far-closed'], parts['far-closed'], Math.min(1, blink * 1.07));
  eyelid(ctx, images['near-closed'], parts['near-closed'], blink);
  if (gulp > .005) part(ctx, images['mouth-open'], parts['mouth-open'], 0, .94 + gulp * .11, .86 + gulp * .24, smooth(gulp * 2.2));
  ctx.restore();
}

export function drawStage(ctx, rigs, images, time, mode = 'showcase') {
  const W = 1080, H = 680;
  ctx.clearRect(0, 0, W, H);
  const bg = ctx.createLinearGradient(0, 0, 0, H);
  bg.addColorStop(0, '#f0faf7'); bg.addColorStop(1, '#d9f0f1');
  ctx.fillStyle = bg; ctx.fillRect(0, 0, W, H);
  ctx.textAlign = 'left'; ctx.fillStyle = '#315e67'; ctx.font = '600 13px Nunito, sans-serif';
  ctx.fillText('FISH ANIMATION STUDY', 44, 43);
  ctx.fillStyle = '#123f50'; ctx.font = '800 32px Nunito, sans-serif';
  ctx.fillText('A little strange. A lot more alive.', 44, 88);
  ctx.font = '500 16px Nunito, sans-serif'; ctx.fillStyle = '#426a73';
  ctx.fillText('Tail swishes, soft fin movement, blinks and feeding gulps.', 44, 118);
  const status = mode === 'showcase' ? (time % 12 >= 8 && time % 12 < 9.7 ? 'Feeding gulp' : time % 12 >= 4 && time % 12 < 7.5 ? 'Active swim' : 'Idle swim + blinks') : { idle: 'Idle swim', swim: 'Active swim', blink: 'Blink', feed: 'Feeding gulp' }[mode];
  ctx.textAlign = 'right'; ctx.font = '700 13px Nunito, sans-serif'; ctx.fillStyle = '#316773'; ctx.fillText(status, 1036, 43);
  for (const [index, id] of ['neonTetra', 'guppy'].entries()) {
    const offset = index * 526;
    ctx.textAlign = 'center'; ctx.fillStyle = '#123f50'; ctx.font = '800 24px Nunito, sans-serif';
    ctx.fillText(rigs.species[id].name, 276 + offset, 175);
    const pose = poseFor(id, time, mode);
    const drift = mode === 'showcase' ? Math.sin(TAU * time / 12) * 10 * (index ? -1 : 1) : 0;
    drawFish(ctx, rigs.species[id], images[id], pose, 17 + offset + drift, 181, 520, 341.25);
    ctx.fillStyle = '#4c7078'; ctx.font = '500 14px Nunito, sans-serif';
    ctx.fillText(index ? 'Slow, sleepy and slightly unimpressed' : 'Quick, curious and a little startled', 276 + offset, 531);
    drawFish(ctx, rigs.species[id], images[id], pose, 206 + offset, 548, 140, 91.875);
  }
  ctx.strokeStyle = '#b7d8dc'; ctx.lineWidth = 1; ctx.beginPath(); ctx.moveTo(540, 169); ctx.lineTo(540, 534); ctx.stroke();
  ctx.textAlign = 'center'; ctx.fillStyle = '#52727b'; ctx.font = '600 11px Nunito, sans-serif';
  ctx.fillText('SMALL-SIZE CHECK', 540, 656);
}
