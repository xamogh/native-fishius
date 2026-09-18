# Tank switcher art


## Clownfish direction correction

Tank 2 was revised to face right, matching the approved mockup. All other raster assets were left unchanged. The frame, scene arrangement and upper-left shine remain in place. Image Gen enlarged the art inside the same 1254 x 1254 canvas during the alpha extraction pass. Revised alpha-at-least-8 bounds are `[62, 25, 1192, 1172]`.

Final source: `/Users/amoghrijal/.codex/generated_images/01a0ae27-7937-73b2-9a27-4d718a5575e6/exec-5a23e3cf-5cbc-4e8a-9343-0a0261f61f8c.png`.

Edit prompt:

Use case: precise-object-edit. Edit ONLY the small orange-and-white clownfish in the center of this transparent aquarium UI sprite. The fish currently faces left. Turn only that fish to face RIGHT, as a horizontal mirror of the fish within its existing central position, keeping the same size, body proportions, orange colors, three white bands, happy eye and small smile. The fish head and mouth must be on its RIGHT end, and tail must be on its LEFT end. Do not mirror, redraw, move or alter anything else. Keep the upper-left glass shine exactly at the upper left. Keep plants, rocks, bubbles, sand, water, cream frame, every frame highlight, brown outline, blank bottom label, canvas size 1254 by 1254, artwork bounds and real transparent alpha outside exactly unchanged. Preserve actual RGBA transparency. No text, no number. Only change the fish direction.

Final alpha extraction prompt:

Remove the background. Return a transparent PNG cutout of the aquarium icon. Keep the icon unchanged.



## Header asset

`header.png` is a blank cream two-part header with an attached count segment. Generated with built-in Image Gen from the same approved large mockup, then cut out with a dedicated transparency pass.

Final source: `/Users/amoghrijal/.codex/generated_images/01a0ae27-7937-73b2-9a27-4d718a5575e6/exec-6a9f48c6-8ddc-4af7-8352-f7eafed7036b.png`.

The PNG is RGBA, 1944 x 809 pixels. Alpha-at-least-8 bounds are `[98, 265, 1845, 530]`. Fit those bounds to the requested 242 x 40 display rectangle. Suggested live title center is `(0.38, 0.48)` and count center is `(0.875, 0.48)` within the fitted rectangle. The generated visible ratio is 6.59, so this target fit applies a small vertical stretch. See `geometry.json`.

### Header generation prompt

Use case: background-extraction. Asset type: production transparent PNG game UI header plate. The supplied image is the approved Fishius mockup. Isolate and faithfully enlarge ONLY the small horizontal cream MY TANKS header above the tank grid at source x137..380,y414..457. Remove ALL letters and numbers. Retain the exact two-part plate design: one wide softly rounded cream left area for a live text label, with a small matching rounded count segment attached at the right. The whole visible plate has width-to-height ratio 242:40. The small right count segment is 56 units of the total 242-unit width, with its own thin curved dark brown left border; the left label area is 186 units wide. Match the original hand-painted casual game UI: pale ivory at the top, warm soft buttercream at the bottom, slim dark brown outside outline, delicate inner highlight along upper edge, restrained warm lower-edge shading and short subtle downward shadow. Flat front view. No gold saturation. Smooth high-quality clean antialiasing. Every pixel outside the plate and its very short shadow must be genuinely alpha-transparent, not a drawn transparency pattern. Center the complete plate with ample transparent margins in a wide PNG canvas. Render only this one blank plate. Absolutely no text, letters, numbers, symbols, fish, aquarium scene or other UI. Do not add texture, bolts or ornaments. Preserve the reference's simple proportions, corner radii, border weight and restrained shading.

### Header transparency prompt

Background removal: make everything outside this one cream game UI header plate fully transparent. Keep the cream rounded plate, brown outline and very short attached bottom shadow exactly. Return a cutout PNG with real alpha transparency. Transparent corners. Transparent exterior. Remove the gray pattern outside. Preserve exact proportions and blank interiors. No text.


Created with the built-in Image Gen tool on 2026-09-17. No source-code artwork recreation, procedural raster painting, or CLI image generation was used.

Approved reference: `/Users/amoghrijal/.codex/generated_images/01a0ae02-50d1-76f2-92f5-bbbde8afa966/exec-7b7cd511-f8e3-45e9-860b-f1f9565fd59f.png`, the larger original design.

## Deliverables

`normal-rim.png` is the reusable pale cream overlay for unselected tanks. Its center and exterior are transparent. See `normal-rim-prompt.md` for its generation prompts and provenance.

All six tank sprites and the selection overlay are genuine RGBA PNGs, 1254 x 1254 pixels, with blank number plates. The app should render live tank numbers on those plates. `tank-selected-overlay.png` has transparent center and exterior, so it can highlight any tank. The separate header dimensions are listed above. `geometry.json` gives exact alpha bounds and the layout convention.

The alpha extraction tool introduced small canvas-scale differences. Fit each sprite using its own alpha-at-least-8 bounds. Their visible composition, plate position, and frame proportions are closely matched, but they are not pixel-identical masks. Alpha values 1 to 7 occur in sparse invisible exterior specks, so do not fit against nonzero-alpha bounds. The generated scenes are faithful redraws of the approved mockup and have slightly more detailed fish and stones.

## Final generated sources

| Asset | Generated source |
| --- | --- |
| tank-1-goldfish.png | exec-140354d4-2561-4e14-9e2b-b30a0962fc0c.png |
| tank-2-clownfish.png | exec-5a23e3cf-5cbc-4e8a-9343-0a0261f61f8c.png |
| tank-3-blue-tang.png | exec-fe13bb47-ef33-485a-9314-63a234f6014e.png |
| tank-4-pink-fish.png | exec-7e247678-63f5-4bba-9ff9-2387442e6f3e.png |
| tank-5-yellow-fish.png | exec-adb868b1-f122-44aa-af44-bc45ced68e74.png |
| tank-6-purple-fish.png | exec-567a2445-d9c9-40ff-bf98-f42b8824dae0.png |
| tank-selected-overlay.png | exec-1efd3932-2e38-4972-b0dc-2a2c4e46f1a0.png |

All generated source files are under `/Users/amoghrijal/.codex/generated_images/01a0ae27-7937-73b2-9a27-4d718a5575e6/`. Final selected copies are in this directory; app code must use these project copies.

## Goldfish generation prompt

Use case: background-extraction. Asset type: final transparent PNG game UI sprite, one isolated tank-selection porthole. Reference image is the approved complete Fishius screen. Extract and faithfully reproduce ONLY the FIRST circular aquarium porthole at the far left of the six-tank grid, the goldfish porthole centered near (97,535), full visual extent about x17..177,y454..617 in this 1641x959 reference. Enlarge that exact design as one clean production sprite, retaining its flat front view, circle geometry, sand, rocks, two small green aquatic plants and smiling orange goldfish facing right, glossy turquoise water with small bubbles and an upper-left white glint. This is the NORMAL unselected version: change ONLY its outer selected gold rim to the same warm pale cream rim, slim dark brown perimeter, and soft inner shading used on the other five portholes. Keep the small attached rounded rectangular number plate centered overlapping the bottom of the ring, but leave the plate completely blank, no number or text. The full art must be centered in a square 1024 by 1024 transparent canvas, with about 7 percent transparent margin on every side. Match the source's simple bright hand-painted casual mobile game UI style exactly; do not redesign. Entire outer background must have genuine alpha transparency, NOT a checkerboard illustration. Remove all scene outside the circle, remove ambient background, no broad glow or cast shadow. Crisp thin brown outline, cream rim, blank cream bottom plate, original source proportions. Just one porthole sprite, no captions, no text, no digits, no other UI.

## Variant prompts

### Tank 2: clownfish

Use case: precise-object-edit. Asset type: final transparent PNG game UI sprite. Image 1 is the approved production geometry template: the circular cream-rimmed aquarium icon with blank bottom label. Image 2 is the approved full game screen for the specific tiny fish scene. Preserve Image 1's EXACT outer shape, cream ring color and shading, dark brown edges, identical canvas dimensions, identical centering and circle bounds, identical attached bottom number-plate size and location, and transparent alpha outside. Edit only the aquarium scene INSIDE the circular glass. Match the simple hand-painted casual game art style in Image 2. Keep the upper-left glass glint, turquoise water, tiny air bubbles, and pale sandy bed. Blank label only, no number, no text. Deliver actual RGBA transparency outside the outline, not an opaque background. Remove broad drop shadows and glow. Replace the orange goldfish with one cheerful orange clownfish, facing LEFT, with three clear white bands edged in dark charcoal. Recreate the SECOND porthole in Image 2 near (255,533): green leaf plants on both lower sides, little gray rounded rocks, pale sand. Keep fish simple and readable at 150 pixels. Only one fish.

### Tank 3: blue-tang

Use case: precise-object-edit. Asset type: final transparent PNG game UI sprite. Image 1 is the approved production geometry template: the circular cream-rimmed aquarium icon with blank bottom label. Image 2 is the approved full game screen for the specific tiny fish scene. Preserve Image 1's EXACT outer shape, cream ring color and shading, dark brown edges, identical canvas dimensions, identical centering and circle bounds, identical attached bottom number-plate size and location, and transparent alpha outside. Edit only the aquarium scene INSIDE the circular glass. Match the simple hand-painted casual game art style in Image 2. Keep the upper-left glass glint, turquoise water, tiny air bubbles, and pale sandy bed. Blank label only, no number, no text. Deliver actual RGBA transparency outside the outline, not an opaque background. Remove broad drop shadows and glow. Replace the orange goldfish with one cute deep royal BLUE TANG fish facing LEFT, with a vivid yellow tail and yellow side fin. Recreate the THIRD porthole in Image 2 near (415,533): green leaf plant at lower left, coral-pink branching coral at lower right, little gray rocks and pale sand. Only one fish.

### Tank 4: pink-fish

Use case: precise-object-edit. Asset type: final transparent PNG game UI sprite. Image 1 is the approved production geometry template: the circular cream-rimmed aquarium icon with blank bottom label. Image 2 is the approved full game screen for the specific tiny fish scene. Preserve Image 1's EXACT outer shape, cream ring color and shading, dark brown edges, identical canvas dimensions, identical centering and circle bounds, identical attached bottom number-plate size and location, and transparent alpha outside. Edit only the aquarium scene INSIDE the circular glass. Match the simple hand-painted casual game art style in Image 2. Keep the upper-left glass glint, turquoise water, tiny air bubbles, and pale sandy bed. Blank label only, no number, no text. Deliver actual RGBA transparency outside the outline, not an opaque background. Remove broad drop shadows and glow. Replace the orange goldfish with one cute PINK fish facing RIGHT, with rosy pink fins, light pink belly, and friendly big eye. Recreate the FOURTH porthole in Image 2 near (96,685): green aquatic plants on both sides, rounded gray stones on sandy floor. Only one fish.

### Tank 5: yellow-fish

Use case: precise-object-edit. Asset type: final transparent PNG game UI sprite. Image 1 is the approved production geometry template: the circular cream-rimmed aquarium icon with blank bottom label. Image 2 is the approved full game screen for the specific tiny fish scene. Preserve Image 1's EXACT outer shape, cream ring color and shading, dark brown edges, identical canvas dimensions, identical centering and circle bounds, identical attached bottom number-plate size and location, and transparent alpha outside. Edit only the aquarium scene INSIDE the circular glass. Match the simple hand-painted casual game art style in Image 2. Keep the upper-left glass glint, turquoise water, tiny air bubbles, and pale sandy bed. Blank label only, no number, no text. Deliver actual RGBA transparency outside the outline, not an opaque background. Remove broad drop shadows and glow. Replace the orange goldfish with one cute bright YELLOW fish facing LEFT, rounded small body, pale yellow fins, friendly large eye. Recreate the FIFTH porthole in Image 2 near (255,685): low green plants, pale gray rock on left, a small warm brown driftwood branch lying diagonally across the sandy bottom from lower left toward lower right. Only one fish.

### Tank 6: purple-fish

Use case: precise-object-edit. Asset type: final transparent PNG game UI sprite. Image 1 is the approved production geometry template: the circular cream-rimmed aquarium icon with blank bottom label. Image 2 is the approved full game screen for the specific tiny fish scene. Preserve Image 1's EXACT outer shape, cream ring color and shading, dark brown edges, identical canvas dimensions, identical centering and circle bounds, identical attached bottom number-plate size and location, and transparent alpha outside. Edit only the aquarium scene INSIDE the circular glass. Match the simple hand-painted casual game art style in Image 2. Keep the upper-left glass glint, turquoise water, tiny air bubbles, and pale sandy bed. Blank label only, no number, no text. Deliver actual RGBA transparency outside the outline, not an opaque background. Remove broad drop shadows and glow. Replace the orange goldfish with one cute purple tropical ANGELFISH facing LEFT, a tall rounded diamond body, three pale lavender vertical bands, elongated purple dorsal and lower fins, friendly large eye. Recreate the SIXTH porthole in Image 2 near (415,685): lime green plant on lower left, small green plants at right, cluster of slate-gray rocks along lower right on sandy bottom. Only one fish.

## Alpha extraction passes

Initial scene generation returned opaque RGB backgrounds. Dedicated background-removal passes created the final genuine alpha images, which were verified with Pillow before being copied into this directory.

Remove the entire background from this single game UI porthole asset. Deliver a PNG with an actual transparent alpha channel. The only opaque pixels should be the circular aquarium, cream outer ring and attached blank cream number plate. All pixels outside the artwork must have alpha zero. Keep the porthole artwork, fish, exact geometry and all colors unchanged. Remove the gray background pattern and every remnant outside the dark brown outline. Do not replace with any colored or patterned background. Isolated clean cutout, actual transparency. No added shadows. Preserve canvas and art placement.

Retry prompt used for Tanks 2 to 5:

Remove the entire background from this single game UI porthole asset. Deliver a PNG with an actual transparent alpha channel. The only opaque pixels should be the circular aquarium, cream outer ring and attached blank cream number plate. All pixels outside the artwork must have alpha zero. Keep the porthole artwork, its geometry and colors unchanged. Remove the gray background pattern and every remnant outside the dark brown outline. Do not replace with any colored or patterned background. Isolated clean cutout, actual transparency. No added shadows. Output square.

Final successful clownfish cleanup prompt:

Background removal: make everything outside the circular aquarium and attached number plate fully transparent. Keep the aquarium artwork exactly. Return a cutout PNG with real alpha transparency. Transparent corners. Transparent exterior. No gray pattern outside. Only one circular fish tank icon.

## Selected overlay prompt

Create a reusable selection overlay from this exact porthole UI icon. Keep ONLY the outer frame and attached blank number plate. Delete the entire aquarium interior (fish, water, plants, rocks, sand and glints) so that all space INSIDE the circle is genuine transparent alpha. All background OUTSIDE the frame also remains genuine transparent alpha. Change only the cream frame and blank plate to vivid saturated golden yellow, matching a selected game button: bright warm yellow face with pale top edge highlight, warmer amber lower edge, same thin dark brown outline. Keep exact geometry, silhouette, canvas size, positions and dimensions of the input cream ring and plate. No added glow or shadow. Output actual RGBA PNG. There must be nothing inside or outside the gold frame except actual transparency. No text or numbers. This will overlay the normal icon at the same pixel coordinates.
