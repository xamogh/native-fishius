# Tuning guide

## Numerical authority

Keep catalog prices, XP, stage durations, and staged rewards in generated content. Do not change them in UI code. Preserve milliseconds even when a label rounds a short schedule to five minutes.

The reference world is 1088 by 635. Taps, feeding, placement, dragging, and tool cursors use the full tank height. Ordinary swimming starts below y=56 and extends to y=635 with fish-height adjustment. Food and egg centers settle 48 world units above the bottom, 16 units higher than the previous landing line. Food and eggs placed below that limit move up to it on the next movement step. UI coordinates and aquarium coordinates are transformed separately. Artwork must keep its own aspect ratio. Decor keeps its existing perspective scale independently of the playable bounds.

## Movement and meshes

The domain owns cruise speed, depth targets, dash timing, animated facing changes, pursuit, and arrival. Canvas owns the traveling body wave and outline geometry. Useful contract values are cruise 20 to 42 units per second, a 280 to 420 millisecond turn, minimum turn width 0.12, maximum pitch 0.42 radians, and 24 longitudinal mesh samples.

The wave amplitude increases toward the tail with a 1.4 exponent. Its base amplitude is five percent of nominal length, and its phase rate depends on speed and the square-root size multiplier. Keep phase continuous. Dead fish use a desaturated asset and no living body wave.

Food starts at 38 units per second, accelerates at 34, caps at 105, rests for 30 seconds, and fades over 1.8 seconds. Arrival uses a clamped reachable depth rather than requiring a large fish's center to touch the floor.

An empty-water tap creates three fading rings over 1.3 seconds. Up to 12 ripples can be visible at once. Fish within 260 world units speed up along their current course for up to 1.1 seconds, then resume normal swimming and feeding. Taps do not start a turn. Eggs, hidden fish and fish in other tanks do not react. Reduced motion uses one fixed ring and a gentler speed boost. Ripples and speed boosts are temporary and clear when switching tanks or suspending the game.

## Presentation

Fish, plants and decorations use the shared `tankArtScale` of 1.2, making their width and height 20 percent larger in the tank. Fish selection, care overlays and swimming clearance follow the larger size. Sell mode also allows 20 percent more local movement so nearby fish can settle with enough space. Decor previews and placement bounds use the same scale as placed items. Catalog dimensions and saved manual sizes stay unchanged.

The current HUD uses Clay bounds from `hud.cpp` and the four-unit tokens in `hud_tokens.hpp`. `shop_theme.cpp` draws the shared Shop and dialog surfaces. The tank switcher and Shop tank cards use the bounds in `hud_tank_switcher.cpp`.

`dialog_motion.cpp` gives dialogs and popovers a 600 ms entrance. They start at 82 percent size, overshoot by about 5 percent and settle with a smaller rebound. The backdrop stays still. The transform keeps frames inside the safe area, and input follows the last rendered position. Closing and reopening restarts the entrance. Reduce motion shows the final size immediately; turning motion back on does not replay an open dialog.

Menu preparation draws every Shop catalog page during the loading screen to cache artwork, texture levels and labels. It checks loading events between pages. The text cache is capped at 96 MiB so prepared card labels stay available at phone and tablet pixel densities. The full-screen Shop also skips drawing the aquarium behind it. `menu_loading_tests.cpp` checks that Shop is opaque and that scrolling introduces no image or text cache misses at desktop, phone and tablet densities.

Render tests cover the current layout at phone, tablet, desktop and portrait sizes. `tests/startup_smoke.cmake` checks default startup, bundled assets, Shop variants and review saves. Startup time is available in the `startup_ms` report field.

The jar pivots around its mouth. The net pivots around its hoop. Receipts last 800 milliseconds. Repeated actions use a bounded effect queue.

Use `--fixture`, `--capture`, `--sequence`, and `--report` for controlled review. Fixture balances and fish populations are synthetic review state unless a comparison document specifically establishes that they came from a supplied screenshot.

## Asset regeneration

Fish images are transparent PNGs with matching true grayscale corpse images and alpha masks for outlines. Required metadata includes stable ID, pixel dimensions, logical dimensions, pivot, provenance, and hash. White-alpha masks must preserve the exact source silhouette. An unseen species interpretation is not an original-game asset.

Canonical source artwork and its provenance are stored in `assets/species`. Review every changed species at both shop size and aquarium size. Do not accept an import just because every filename exists.
