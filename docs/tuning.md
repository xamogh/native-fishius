# Tuning guide

## Numerical authority

Keep catalog prices, XP, stage durations, and staged rewards in generated content. Do not change them in UI code. Preserve milliseconds even when a label rounds a short schedule to five minutes.

The reference world is 1088 by 635. Ordinary swimming starts below y=56 and ends near y=512 with fish-height adjustment. UI coordinates and aquarium coordinates are transformed separately. Artwork must keep its own aspect ratio.

## Movement and meshes

The domain owns cruise speed, depth targets, dash timing, animated facing changes, pursuit, and arrival. Canvas owns the traveling body wave and outline geometry. Useful contract values are cruise 20 to 42 units per second, a 280 to 420 millisecond turn, minimum turn width 0.12, maximum pitch 0.42 radians, and 24 longitudinal mesh samples.

The wave amplitude increases toward the tail with a 1.4 exponent. Its base amplitude is five percent of nominal length, and its phase rate depends on speed and the square-root size multiplier. Keep phase continuous. Dead fish use a desaturated asset and no living body wave.

Food starts at 38 units per second, accelerates at 34, caps at 105, rests for 30 seconds, and fades over 1.8 seconds. Arrival uses a clamped reachable depth rather than requiring a large fish's center to touch the floor.

## Presentation

The intended panel opening uses 220 milliseconds and cubic-bezier(0.34,1.56,0.64,1). Evaluate the time axis of the curve rather than treating its polynomial parameter as elapsed time. The tank drawer adds a 42-unit entrance offset over 240 milliseconds.

The jar pivots around its mouth. The net pivots around its hoop. Receipts last 800 milliseconds. Toast fade begins near 1.9 seconds and removal near 2.4 seconds. A repeated action should replace or restart a bounded effect, not build an unlimited queue.

Use `--fixture`, `--capture`, `--sequence`, and `--report` for controlled review. Fixture balances and fish populations are synthetic review state unless a comparison document specifically establishes that they came from a supplied screenshot.

## Asset regeneration

Fish images are transparent PNGs with matching true grayscale corpse images and alpha masks for outlines. Required metadata includes stable ID, pixel dimensions, logical dimensions, pivot, provenance, and hash. White-alpha masks must preserve the exact source silhouette. An unseen species interpretation is not an original-game asset.

The generated-art tool contains species shape and palette decisions. Review every changed species at both shop size and aquarium size. Do not accept an import just because every filename exists.
