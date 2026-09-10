# Aquarium reference artwork

The user supplied `design/reference/aquarium.png`, a 2498 by 1290 pixel crop of
the app content in their screenshot. The original photo's phone frame is not
part of the game.

Buttons and fish preserve the supplied raster pixels. Their transparency masks
follow the source silhouettes. Fixed button lettering is part of the artwork.
Currency values use that lettering at the matching value and render live text
when the balance changes. Game transactions continue to use the domain state.

`reef-restored.png` was made with the built-in image-generation tool to fill
areas hidden by fish, controls, and the photo's corners. `reef.png` combines
that restoration with the exposed pixels from the original reference. The
restoration is an approximation of scenery that the screenshot does not show.
The exact prompt and provenance are in
`work/aquarium-match/reef-generation.json`.

`reference-layout.json` records the source positions in a 1608 by 830 design
area. Rebuild the assets with `tools/extract_aquarium_reference.cjs`. Its
subscripts in `tools/aquarium` extract the detailed silhouettes, then
`tools/compose_aquarium_background.cjs` clears their original footprints.
Set `SHARP_MODULE` to the installed Sharp module path if the bundled runtime
is elsewhere.

Fish retain separate alpha masks, grayscale dead variants, live movement,
selection, care, and sale behavior. The reference fixture uses the six source
poses for a repeatable visual comparison. Ordinary games use their saved fish.
