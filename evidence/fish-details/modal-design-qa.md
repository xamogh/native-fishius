# Fish selection details QA

final result: passed

No remaining P0, P1, or P2 findings in the inspected fish details flow. The
layout and interactions pass visual QA. Generated artwork and font rendering
retain small differences from the source, so this does not claim identical
output pixels.

## Source and comparison

- Source: `evidence/fish-details/reference.png`, a byte-for-byte copy of the
  user's Screenshot 2026-09-10 at 8.31.43 PM.png.
- Implementation: `evidence/fish-details/reference-size-final.png`.
- Both images are 1338 by 1012 pixels. The native SDL window is 669 by 506
  points at 2x display density. `--capture-window` exports at that physical
  size. CSS and browser runtime checks do not apply to this C++/SDL app.
- State: selected baby Molly, zero growth, hungry with 11h 59m remaining,
  250 coins, zero pearls, level 1 with 14/80 XP, and no other open controls.
  The review fixture holds this state without writing the ordinary save.
- Source and final implementation were opened together in the same tool
  comparison input. The existing reef and background fish remain game assets;
  their precise shapes and poses are outside this details-card change.
- Focused review covered the title at x430/y110, care row at x325/y310,
  growth/stage area at x320/y390, and selected fish at x465/y755 in both
  full-resolution images. Separate crops were unnecessary: all lettering and
  the 223-pixel fish preview were clearly readable at the inspected size.
- Mobile composition was also inspected in `852x393-baby.png` and the actual
  iPhone 17 Pro simulator. `iphone-17-pro-final.png` is the raw simulator
  capture; Simulator's own display was used for the landscape visual check.

## Fidelity surfaces

| Surface | Assessment |
| --- | --- |
| Fonts and typography | Bundled Luckiest Guy supplies live display lettering. Title, species name, care line, percentage, stage names and footer use measured source positions and sizes. Gold title shading and blue outlines preserve the hierarchy. The source's illustrated letter shapes and soft shadows are not exact font glyphs. |
| Spacing and layout | The panel uses a 1338 by 1012 reference coordinate system. It stays centered and scales uniformly to fit phone height. Its gold border, title pill, pointer, five stages, footer and fish remain fully visible. |
| Colors and materials | Generated transparent raster artwork supplies cyan glass, gold borders, bubbles, food orb, XP star and stage rings. The lower panel was changed from royal blue to cyan and teal after comparison. The aquarium has a soft blur behind the card. |
| Image quality | Full-resolution panel artwork is kept sharp. The selection glow and black Molly preview use real alpha transparency. Alternate assets are archived outside the runtime asset tree. |
| Copy and state | Hunger deadlines, growth percentage, current stage and sale values come from the selected fish and workbook rules. Baby, fed, Junior, Adult, sick, egg and dead states were exercised. Hyphens follow the user's punctuation preference. |

## Findings and iteration history

1. P1: The former details panel used the old generic dialog, action buttons and
   title. Replaced it with the supplied card structure and a separate selected
   fish preview. The first rendered comparison is `reference-size-v1.png`.
2. P2: The first desktop capture had the wrong aspect ratio because the window
   manager constrained a large point-sized window. Captured at 669 by 506
   points and normalized to 1338 by 1012 pixels in `reference-size-v2.png`.
3. P2: Main text was too small, the panel interior too blue, and the initial
   blur visibly blocky. Enlarged the measured text, softened the outlines,
   selected the cyan frame revision, and changed the blur to two five-tap
   passes. `reference-size-v3.png` records the result.
4. P2: The first Molly preview was too slender with a small eye. Selected the
   second generated preview with a rounded head and larger gold eye. See
   `reference-size-final.png`.
5. P2: A live feeding receipt could overlap stage labels, and filled growth
   touched the track border. Suppressed world receipts while details is open
   and inset the fill. The final Adult screenshot, `669x506-adult.png`, shows
   the corrected state. The source and final baby view were compared again.

## Interaction and build verification

- Desktop and iOS simulator builds pass. Logs: `desktop-build.log` and
  `ios-build.log` under `evidence/fish-details`.
- All three CTest suites pass: domain contracts, existing mobile controls and
  the new fish details controls. See `evidence/fish-details/final-tests.log`.
- Details touch checks pass at 669 by 506, 804 by 415, 852 by 393 and 667 by
  375 points. They verify selection, unchanged fish state on opening,
  viewport bounds, hidden old controls, live care and growth, stage changes,
  recovery actions, and backdrop dismissal without a second action.
- The existing mobile control suite still passes at its three phone sizes,
  including Shop, Tanks, purchases, inventory and aquarium tools.
- On the actual iPhone simulator, tapped inside the card, tapped the backdrop
  over the Food position, and selected Molly again. The inside tap kept the
  card open; the outside tap only dismissed it; selection reopened it.
- A 180-frame animated Metal smoke check completed. Its p95 CPU update/draw
  submission time was 0.24 ms, excluding presentation waits. This is not a
  physical-device GPU benchmark. See `live-smoke.json`.
- The simulator is left open with the live details preview. Review mode keeps
  ordinary saved progress unchanged. Physical-device installation was not
  tested. No commit or push was performed for this change.

## Follow-up polish

- P3: Generated stage rings have stronger highlights, and the Molly's fins,
  eye and body outline differ slightly from the illustration.
- P3: Source text has hand-shaped lettering and softer shadows. Live font
  glyphs and antialiasing vary slightly.
- P3: The already implemented aquarium background has a different cover crop
  from the new 4:3 reference. This change retains the user's existing reef.

## Completed checks

- [x] Compare source and final render at equal pixel dimensions and state.
- [x] Inspect all five required fidelity surfaces and focused details.
- [x] Fix the visible layout, material and live-state issues, then recapture.
- [x] Test selection and modal gestures across mobile viewports.
- [x] Build and inspect the native iPhone app.
- [x] Preserve ordinary saves and keep the preview available.

Earlier aquarium and Shop QA is preserved at
`evidence/fish-details/prior-design-qa.md`.
