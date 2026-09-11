# Fish selection popover QA

final result: passed

The user's latest instruction replaces the centered reference card with a
popover anchored to the selected fish. Exact screenshot matching is no longer
the acceptance target. The earlier modal comparison is preserved in
`evidence/fish-details/modal-design-qa.md`.

## Result

- The selected fish stays at its actual position and keeps its normal artwork.
- A compact 520 by 306 logical-unit popover points toward the fish. It chooses
  above, below or a side based on available room, then stays inside safe bounds.
- The aquarium remains clear and keeps its normal viewport and visible controls.
- Selecting another fish switches the popover in one tap. Aquarium tools still
  work. Empty water and the close button dismiss it.
- Hunger, growth, stage, sale values and recovery actions remain live.
- Shop keeps its existing modal backdrop behavior.

## Visual verification

Inspected the native iPhone 17 Pro simulator at 874 by 402 points. Switched
from Molly to Guppy, activated Food, then selected Molly again. The pointer
moved with the selected fish, the card chose a suitable side, and the controls
remained visible and usable. The app is left open in a save-safe live preview.

Also inspected the captured top-left and bottom-right edge states:
`evidence/fish-popover/667x375-edge-80-90.png` and
`evidence/fish-popover/852x393-edge-980-500.png`.
The card, text and pointer remain visible, and the selected fish is not covered.

The screenshot helper now captures the actual displayed composition rather
than stretching the game frame into narrower window proportions. Viewport
captures use physical display pixels, including the existing reef outside the
game viewport where applicable.

## Checks

- Desktop and iOS builds pass.
- All three CTest suites pass: domain, existing mobile UI, fish details.
- Popover tests cover 669 by 506, 804 by 415, 852 by 393 and 667 by 375 points.
- Tests cover the center and four corners, unchanged fish state and viewport,
  live care/growth, recovery, another-fish selection, tools and dismissal.
- `git diff --check` passes.
- Evidence: `evidence/fish-popover/final-tests.log`, `desktop-build.log`,
  `ios-build.log`, and `iphone-final.png` in the same folder.

The existing cyan panel artwork and Lilita One labels match the aquarium's
controls. Text is contained, stages remain readable, and no extra preview fish
or fullscreen backdrop is drawn. Physical iPhone installation was not tested.
No commit or push was performed.
