# Normal cream frame overlay

Created with built-in Image Gen on 2026-09-17. The selected gold overlay supplied geometry, and tank-1-goldfish.png supplied the normal cream color reference.

Output: `normal-rim.png`, RGBA 1254 x 1254. Center alpha is zero. Alpha-at-least-8 bounds are `[120, 87, 1132, 1133]`, using exclusive right and bottom coordinates. Fit these bounds to the same destination rectangle as the selected gold overlay. The alpha extraction pass slightly enlarged the art within the canvas, so the new bounds must be used.

Generated source: `/Users/amoghrijal/.codex/generated_images/01a0ae27-7937-73b2-9a27-4d718a5575e6/exec-f8a5b3cc-a4cf-4b2b-9d39-22f13263ec25.png`.

## Generation prompt

Use case: precise-object-edit. Asset type: reusable NORMAL STATE cream game UI frame overlay with genuine transparency. Image 1 is the exact frame shape and geometry to edit, a gold circular rim with an attached blank bottom number plate and transparent center/exterior. Image 2 is the color reference only, the goldfish sprite with a soft normal cream frame. Change ONLY Image 1's gold face fill to pale ivory and soft buttercream matching the normal frame in Image 2. Keep Image 1's exact geometry, circle position, outer silhouette, bottom plate position and shape, thin dark brown outlines, inner edge, subtle upper-left highlight, canvas dimensions 1254 x1254 and all transparency unchanged. The NORMAL frame should be predominantly pale ivory/cream with restrained warm beige lower-edge shading. Avoid saturated yellow, gold or orange except the very subtle warm lower bevel. The bottom number plate must match the pale cream rim and remain blank. The entire circle center and entire outside background must be real transparent alpha. Keep ONLY the frame and attached plate. No fish, aquarium, water, plants, sand, symbols, letters or digits. This is a transparent overlay to unify normal tank frames; it must match the selected overlay geometry so both can align.

## Transparency prompt

Remove all background, both outside and inside this cream circular frame. Return an actual transparent PNG cutout containing only the cream frame and its attached blank plate. The large hole in the center must be truly transparent. The exterior must be truly transparent. Keep the frame artwork and its positions unchanged.
