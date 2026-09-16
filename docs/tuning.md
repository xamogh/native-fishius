# Tuning guide

## Numerical authority

Keep catalog prices, XP, stage durations, and staged rewards in generated content. Do not change them in UI code. Preserve milliseconds even when a label rounds a short schedule to five minutes.

The reference world is 1088 by 635. Taps, feeding, placement, dragging, and tool cursors use the full tank height. Ordinary swimming starts below y=56 and extends to y=635 with fish-height adjustment. Food settles five world units above the bottom; eggs settle twelve units above it. UI coordinates and aquarium coordinates are transformed separately. Artwork must keep its own aspect ratio. Decor keeps its existing perspective scale independently of the playable bounds.

## Movement and meshes

The domain owns cruise speed, depth targets, dash timing, animated facing changes, pursuit, and arrival. Canvas owns the traveling body wave and outline geometry. Useful contract values are cruise 20 to 42 units per second, a 280 to 420 millisecond turn, minimum turn width 0.12, maximum pitch 0.42 radians, and 24 longitudinal mesh samples.

The wave amplitude increases toward the tail with a 1.4 exponent. Its base amplitude is five percent of nominal length, and its phase rate depends on speed and the square-root size multiplier. Keep phase continuous. Dead fish use a desaturated asset and no living body wave.

Food starts at 38 units per second, accelerates at 34, caps at 105, rests for 30 seconds, and fades over 1.8 seconds. Arrival uses a clamped reachable depth rather than requiring a large fish's center to touch the floor.

## Presentation

Menu motion is defined in `src/menu_motion.cpp`. Panels grow from 84 percent width and 80 percent height, rise 24 logical units, and fade in. The spring peaks near 165 milliseconds and finishes settling by 580 milliseconds. Closing uses critical damping and finishes within 320 milliseconds. Reversing an opening or closing menu preserves its current position and velocity.

Tanks expands from its lower left corner. Fish details expand from the pointer beside the selected fish. Select options open 45 milliseconds apart. Buttons compress to 95 percent over 75 milliseconds and spring back on release. Reduced Motion shows menus immediately and disables the spring effects.

The renderer draws a panel and its text into one layer at the native display resolution. Tap targets use the same transform. A closing panel retains only its artwork, with no active controls, and releases the layer when the exit ends. The Shop backdrop fades with the panel and consumes dismissal taps.

`View::prepareMenus` prepares the initial menu layouts, artwork and text before the first interactive frame. This prevents the first Shop or Mastery tap from decoding images and rasterizing labels. It uses a separate view and does not change saved state. Startup preparation time is reported as `menu_preparation_ms` by `--report`. The `menu_motion` test checks first-open cache reuse, animation timing, tap targets and Reduced Motion on phone and tablet layouts.

The jar pivots around its mouth. The net pivots around its hoop. Receipts last 800 milliseconds. Repeated actions use a bounded effect queue.

Use `--fixture`, `--capture`, `--sequence`, and `--report` for controlled review. Fixture balances and fish populations are synthetic review state unless a comparison document specifically establishes that they came from a supplied screenshot.

## Asset regeneration

Fish images are transparent PNGs with matching true grayscale corpse images and alpha masks for outlines. Required metadata includes stable ID, pixel dimensions, logical dimensions, pivot, provenance, and hash. White-alpha masks must preserve the exact source silhouette. An unseen species interpretation is not an original-game asset.

The generated-art tool contains species shape and palette decisions. Review every changed species at both shop size and aquarium size. Do not accept an import just because every filename exists.
