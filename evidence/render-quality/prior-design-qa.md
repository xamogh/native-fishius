# Fish popover reference styling QA

final result: passed

The latest request keeps fish-anchored popover behavior and restores the
reference design, with decorative bubbles and related effects omitted.
No remaining actionable P0, P1 or P2 findings were found in this scope.

## Comparison and evidence

- Source visual: `evidence/fish-details/reference.png`, 1338 by 1012 pixels.
- Final native capture: `evidence/fish-popover-reference/final.png`, 1704 by
  786 pixels from an 852 by 393 point window at 2x density.
- Both source and final implementation were opened together in the same
  comparison input. The selected state is baby Molly, 0% growth, hungry with
  11h 59m remaining, 250 coins, zero pearls and 14/80 XP.
- The intended viewport and placement now differ: the reference is a centered
  card, while the requested implementation is anchored to the actual fish.
  Comparison therefore uses the card's relative proportions and hierarchy,
  rather than absolute whole-screen pixel differences.
- The original card body and header span about 808 by 650 source pixels. The
  popover scales that coordinate system uniformly to 580 by 466.6 logical
  units. Fish placement, normal aquarium controls and clear background remain
  part of the accepted popover behavior.
- Full-resolution review also focused on the title, care/XP rows, growth track
  and stage markers. Separate crops were unnecessary because the labels and
  icons were readable in the full capture.
- Edge evidence includes `667x375-edge-80-90.png` and
  `852x393-edge-980-500.png` in the same evidence folder.

## Fidelity review

| Surface | Result |
| --- | --- |
| Typography | Restored the large gold stage heading, centered uppercase species name, bold white growth label, gold care/percentage text and two centered footer lines. Uses the bundled Luckiest Guy font with blue outlines and gold title shading. |
| Layout | Restored the reference card proportions, raised title pill and vertical row spacing. The movable pointer targets the selected fish. A small close control remains available. |
| Color and material | Gold double border, glossy cyan/teal interior, white highlights and dark cyan-rimmed growth track follow the reference. |
| Artwork | The revised raster frame keeps the food orb, XP star and five neutral stage circles. Decorative bubbles, seaweed, particles and the fixed bottom pointer were removed. No separate glowing fish preview is drawn. |
| Content | All care, growth, stage and sale values remain live. Baby, fed, Junior, Adult, sick, egg and dead states still work. |

## Changes made during review

1. The previous simple popover had a small left-aligned title, plain cyan
   border and tab-like stages. These were material design differences from
   the requested reference. Restored the hierarchy, gold frame, artwork and
   circular stages. `first-pass.png` records the first restored render.
2. Aligned the active stage marker with the revised asset's measured circle
   centers. `final.png` records the aligned design.
3. The top-corner pointer was too shallow when it connected to the title.
   It now connects to the body edge when outside the raised title. The final
   `667x375-edge-80-90.png` shows this correction.

## Validation

- Desktop and iOS builds pass. Logs are `desktop-build.log` and `ios-build.log`
  under `evidence/fish-popover-reference`.
- All three CTest suites pass: domain contracts, existing mobile controls and
  fish details. See `final-tests.log` in that folder.
- After the pointer refinement, the details suite was rerun and passed at
  669 by 506, 804 by 415, 852 by 393 and 667 by 375 points. It verifies live
  states, center/corner positions, selecting another fish, aquarium tools,
  closing and outside taps. See `details-tests.log`.
- Visually checked iPhone 17 Pro at 874 by 402 points and switched from Molly
  to Guppy. The Adult state, filled bar and active circle updated correctly.
- Final simulator evidence: `iphone-final.png` in the same folder. The live
  preview is left open without writing ordinary saved progress.
- `git diff --check` passes. No commit or push was performed.

## Asset and remaining polish

The built-in image generation tool produced
`assets/details/popover-frame.png`. Its prompts, alpha checks and feature
coordinates are in `assets/details/popover-frame.provenance.json`.

P3: Illustrated lettering, tiny raster edge highlights and ring bevels still
vary slightly from the reference. These do not change the expected design or
popover behavior. Physical-device installation was not tested.

The prior simplified-popover QA report is preserved at
`evidence/fish-popover-reference/prior-design-qa.md`.
