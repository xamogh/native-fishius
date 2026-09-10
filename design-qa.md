# Aquarium screen QA

final result: passed

No remaining P0, P1, or P2 findings in the aquarium view. This is a visual and
interaction acceptance result, not a claim that every output pixel is identical
to the photograph. Small differences in restored scenery and raster edges remain.

## Requested spacing update

Shop is restored to its original 1394 by 710 logical-unit panel. The aquarium
and its controls remain behind a dimmed backdrop. Pointer handling is modal:
an outside tap closes Shop and consumes the gesture. Controls behind Shop
cannot activate from that tap or from taps inside the panel. Product cards
retain their original height, and View Deals remains clear of bottom navigation.

Desktop and iOS builds pass. Native finger-event checks pass at 804 by 415,
852 by 393, and 667 by 375 points. They verify that backdrop taps over Food,
Sell, Mastery, notifications, and Tanks only dismiss Shop, that a separate tap
can then activate the control, and that Shop tabs, offers, and buying still work.
The restored modal was visually inspected on iPhone 17 Pro. Evidence:
`evidence/aquarium-match/shop-modal-ui-tests.log` and
`evidence/aquarium-match/shop-modal-iphone.png`.

The earlier compact Shop layout and direct background-menu switching were
superseded by this requested full-size modal behavior. The `shop-layout-*`
files retain evidence of that earlier iteration only.

The next requested adjustment moves Shop 32 logical units left and up to 20
units lower, while keeping its artwork inside the canvas. Tanks now opens
with a 24-unit gap to the right of its full button artwork. Its touch area
uses the same offset as its rendering. Select, Food, Mastery, and Sell remain
visible and usable while Tanks is open.

Desktop and iOS builds pass. Updated UI checks pass at all three mobile sizes,
including direct switching from Tanks to the right-side controls and taps
inside the relocated menu. Inspected the final iPhone 17 Pro render, tapped
Food with Tanks open, and left Tanks open for user review. Evidence:
`evidence/aquarium-match/tanks-layout-ui-tests.log` and
`evidence/aquarium-match/tanks-layout-iphone.png`.

After the reference comparison, the user requested even gaps between the four
main right-side icons and removal of Medicine for now. The column now uses
24 logical units between controls. Mastery moved up by 26 logical units.
Medicine is hidden, and Food directly activates feeding.

Both desktop and iOS builds pass. Existing native UI interaction checks pass
at 804 by 415, 852 by 393, and 667 by 375 points. The updated layout was also
visually checked on the iPhone 17 Pro simulator and left open for testing.
Evidence: `evidence/aquarium-match/spacing-ui-tests.log` and
`evidence/aquarium-match/spacing-iphone.png`. The comparisons below record the
earlier screenshot-matching pass, before these requested changes.

## Comparison target

- Source: `design/reference/aquarium.png`, 2498 by 1290 pixels. This is the app
  content crop from the user's 2618 by 1406 screenshot, at x=70, y=55.
- Reference viewport: 1608 by 830 logical units, displayed in an 804 by 415
  point native SDL window. This project is C++/SDL, so there is no CSS viewport
  or browser runtime.
- State: aquarium open, Select active, Medicine visible, six fish in the source
  poses, 250 coins, 0 pearls, level 1, 14/80 XP. The `aquarium --still` fixture
  holds fish positions for comparison and never overwrites normal saves.
- Render: `evidence/aquarium-match/iteration-6.png`, 3216 by 1660 pixels, from
  the actual Metal renderer. Both source and render were reduced to 1608 by 830
  for comparison. `final-reference-size.png` is the normalized render.
- Full comparison: `evidence/aquarium-match/comparison-final.png`. Reference
  above, implementation below. This combined image was opened and reviewed.
- Focused comparisons: `focus-wallet.png`, `focus-level.png`, `focus-tools.png`,
  `focus-tanks.png`, `focus-shop.png`, and `focus-fish.png` in the same folder.
  Reference is on the left. These were also opened and reviewed.
- The photographed bezel and black rounded corners are excluded from the
  fidelity target. Device cutouts remain owned by iOS.

## Mobile verification

- Built and installed the native iOS simulator app using iOS 26.5.
- Inspected the iPhone 17 Pro screen in Simulator. Its content viewport is
  874 by 402 points. The 2622 by 1206 capture is
  `evidence/aquarium-match/iphone-17-pro-final.png`. The raw portrait-oriented
  capture was rotated 270 degrees for viewing without changing its density.
- This phone is wider than the source. The reef uses a cover crop, the screen
  remains filled, controls keep their height, and corner controls stay near
  the display edges. Left controls clear the cutout. The right controls remain
  clear in the inspected landscape orientation.
- Tapped Food and Sell in Simulator and inspected their active states.
  Simulator use also showed fish sales updating the balance and level.
- Native SDL finger-event tests pass at 804 by 415, 852 by 393, and 667 by 375
  points. They cover Tanks, Shop, Mastery, notifications, Settings, expanded
  44-point touch targets, buying and placing an egg, Food, Medicine, Move,
  Stash, Bag restore, Sell, Done, and live wallet changes.
- A final 60-frame animated Metal smoke check passed at 852 by 393 points.
  Evidence: `evidence/aquarium-match/live-smoke.json`.
- The simulator was shut down after testing. The final build is available
  through `Run iPhone Simulator.command`, which installs the current bundle.
- Evidence: `evidence/aquarium-match/ui-tests.log`. Build logs for desktop and
  iOS are in the same folder. Browser console checks do not apply to SDL.
- Physical iPhone installation and the opposite landscape orientation were
  not tested. No claim of full device certification is made.

## Fidelity surfaces

| Surface | Result |
| --- | --- |
| Fonts and typography | Source lettering is retained on all primary illustrated controls and the matching status values. Live changed values use the bundled Lilita One font. Labels are readable at the inspected mobile sizes. |
| Spacing and layout | Right tool column, Medicine row, top status units, and two bottom corner buttons use measured source coordinates. The former left navigation and bottom toolbar are removed from the aquarium view. |
| Colors and materials | Original cyan glass, white lettering, blue strokes, orange gold, fish colors, and most exposed reef pixels are retained. |
| Image quality | Source-resolution sprite cutouts replace the earlier approximate artwork. Masks include button lettering, foliage, and bubbles. The actual app draws separate moving fish and interactive controls. |
| Copy and content | SELECT, FOOD, Medicine, MASTERY, SELL, TANKS, SHOP, and reference status values match. Supplemental actions appear only after interaction. |

## Comparison history

1. The original capture, `before.png`, had a left navigation column, an open
   Tanks panel, different reef artwork, and mismatched fish. These were P1
   differences. Replaced the aquarium layout, default panel, artwork, and
   reference fixture. Captured `iteration-1.png` and `comparison-1.png`.
2. First render showed fish at twice the intended size, a P1 error. Corrected
   the adult sprite scale. Also corrected excess right spacing on iPhone.
   Captured `iteration-2.png` and `comparison-2.png`.
3. Bottom buttons and the level star had clipped decorations and poor masks,
   P2 differences. Traced their source silhouettes and updated the destination
   dimensions. Captured `iteration-3.png` and `comparison-3.png`.
4. Broad background patches and artificial corner edges were visible around
   the extracted controls, P2 differences. Rebuilt the restoration from source
   alpha masks and removed the photographed corners. Captured
   `iteration-4.png`, `comparison-4.png`, and focused regions.
5. Reduced mask-edge drift and inspected `iteration-5.png` and
   `comparison-5.png`. The wider iPhone capture then exposed faint original
   fish and plant remnants away from their moving/repositioned sprites, a P2
   mobile defect.
6. Cleared the complete source fish and corner-art footprints and strengthened
   edge removal. Captured `iteration-6.png`, `comparison-final.png`, focused
   comparisons, and the final iPhone screenshot. The remnants are gone. No
   further P0/P1/P2 differences were found in the inspected view.

## Follow-up polish and limits

- P3: Restored water and sand around a few sprite edges differ slightly from
  the source. The source does not contain the hidden scenery. These areas use
  the generated restoration, with original visible scenery retained elsewhere.
- P3: Antialiasing and tiny silhouette edges vary between the source raster and
  the scaled Metal render. This prevents literal pixel identity.
- Animated gameplay and saved balances naturally differ from a frozen photo.
  The comparison fixture is separate from ordinary progress.
- Existing domain tests: 23/24 pass. The documented pre-existing failure is
  `one pellet feeds one fish with stable tie order`, at `d.pellets().empty()`.
  The new UI tests pass. This screen update does not claim to fix that domain
  fixture or complete the wider game's outstanding feature list.

## Completed checks

- [x] Compare the same reference viewport and state.
- [x] Inspect full and focused comparisons after fixes.
- [x] Verify a real iPhone simulator render.
- [x] Test primary actions through normalized SDL touch events.
- [x] Verify native builds and asset bundling.
- [x] Keep source artwork, generation provenance, and screenshots in the repo.
- [x] Preserve ordinary save data. No push was performed.
