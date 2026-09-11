# Rendering and iPad layout QA, 11 September 2026

Result: passed in the desktop Metal renderer and iOS simulators.

## Rendering

- The frame target matches the window's physical pixel dimensions. It is copied
  to the screen without the old fixed 2x intermediate scale.
- Text uses high-resolution glyph textures, shared between tint and outline
  passes. Filtered levels support compressed labels. The cache is limited to
  384 entries and a 32 MiB working budget.
- Artwork uses alpha-aware area filtering for smaller texture levels. This
  reduces minification shimmer and avoids colored fringes from transparent pixels.
- Rounded geometry has a one-pixel antialias fringe and finer curved edges.
- Fish position, fin phase, pitch, turning and care labels use the same movement
  interpolation. Falling pellets also interpolate between simulation steps.
- The presentation loop targets 60 Hz even when simulator Metal accepts VSync
  without pacing frames. This does not change the 20 ms simulation step.

## iPad support

- The Ninja-built bundle now declares both iPhone and iPad in `UIDeviceFamily`.
  Previously the iPad preview used a 480 x 320 point compatibility view with a
  960 x 640 render target. The new iPad Pro 11-inch run uses 1210 x 834 points
  and 2420 x 1668 physical pixels.
- The aquarium expands vertically for tablet aspect ratios. One continuous
  background fills the screen; the second background layer has been removed.
- Bottom controls follow the safe bottom edge. Side controls keep even spacing.
  Header controls leave room for iPad window chrome and the status area.
- Shop remains a large modal. Taller tablet layouts show eight fish in two rows
  of four. Collection uses the additional height. Other menus stay centered or,
  for Tanks, anchored beside the lower-left button.
- Fish details remain a compact popover anchored to the selected fish.

Apple documents the device-family key in its [Info.plist reference](https://developer.apple.com/library/archive/documentation/General/Reference/InfoPlistKeyReference/Articles/iPhoneOSKeys.html).
The template declares it because the Ninja generator does not apply the Xcode
`TARGETED_DEVICE_FAMILY` build setting.

## Verification

- Desktop and iOS simulator builds pass.
- All four CTest suites pass: domain rules, mobile controls, fish details and
  rendering quality. Phone layouts include 804 x 415, 852 x 393 and 667 x 375.
  Tablet checks include 1024 x 768 and 1210 x 834. Details also cover 669 x 506.
- Resize checks verify native frame size, full-window composition and touch
  mapping. A fine transparency pattern checks area filtering at subpixel offsets.
  Repeated identical text frames remain pixel-identical, including shared tints.
- Movement checks cover interpolated fins and pitch, turning through the midpoint,
  continuity across simulation steps and reset after suspension.
- Native iPad checks cover Shop, backdrop dismissal, Tanks, fish details and live
  rendering. Native iPhone checks cover landscape detail rendering and notch margins.
- iPad frame pacing after 60 warm-up frames: median 16.666667 ms, p95 16.667542 ms,
  p99 16.670708 ms across 300 measured intervals. This is a simulator measurement,
  not a physical-device benchmark.
- No source artwork was regenerated. Game saves and the deferred tutorial,
  anniversary timing and unspecified reward rules were preserved.

## Evidence

See `evidence/render-quality/`:

- `final-tests.log`, `desktop-build.log`, `ios-build.log`
- `ipad-frame-pacing.json`, `iphone-frame-pacing.json`, `ios-bundle-check.json`
- `after/ipad-native-shop.png`, `after/iphone-native-details.png`
- `before/iphone-details.png` and the desktop comparisons in `after/`

Earlier failed checks and screenshots remain in the folder as iteration evidence.
`prior-design-qa.md` records the earlier popover styling review.

## iPad tool stack follow-up

- Select, Food, Mastery and Sell now form a compact stack above Shop, with
  12-unit gaps. The stack follows the bottom edge on tablet layouts.
- The Select flyout follows the relocated Select button. Phone positions stay
  unchanged.
- Desktop and iOS simulator builds pass. Mobile UI checks pass at all five
  viewport sizes, including both tablet sizes. Fish details checks also pass.
- The existing Shop backdrop test now probes the empty modal border because
  Food's new position aligns with a Buy button behind that border.
- Visually checked the running iPad app. Evidence is in
  `evidence/ipad-tool-stack/ipad-stacked-tools.png` and the test logs beside it.
