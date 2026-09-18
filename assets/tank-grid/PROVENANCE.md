# Tank grid artwork

Generated on 2026-09-16 with the built-in Image Gen tool. PNG alpha channels are preserved without image postprocessing.

Style reference: `design/mockups/tanks-grid-2026-09-16/01-illustrated-cards.png`.

## Active tank

Use case: stylized-concept. Create ONE isolated active aquarium tank game sprite matching the tank art in the FIRST top-left card of the provided UI reference. Reference is style and subject only, do not reproduce UI. Wide rectangular aquarium, visible object aspect ratio 2:1, front-facing with a slight view of top and right glass. Bright turquoise water, thick dark navy cartoon outlines, glossy cyan glass rim, pale sand, green aquatic plants at ends, small rounded gray cave and driftwood, two small friendly colorful fish (orange clownfish and blue fish). Polished playful hand-painted casual game illustration. Center whole tank with small transparent margins, tank fills image, no cropping. TRUE TRANSPARENT ALPHA BACKGROUND outside the tank. Water inside tank stays colored. No background scene, no floor, no text, no badge, no card, no lock, no UI. Output PNG landscape sprite.

## Empty tank

Use case: precise-object-edit. Input image is edit target: active aquarium game sprite. Make the reusable EMPTY tank variant: remove ONLY both fish, leaving water where they were. Preserve exactly the aquarium shape, glass outline, cyan rim, sand, plants, driftwood, rock cave, lighting, illustration style, framing, canvas dimensions and transparent outer background. No fish, no lock, no text, no badge, no UI. True transparent alpha outside tank. Output one transparent PNG sprite.

## Lock

Use case: stylized-concept. Generate ONE isolated small game padlock icon matching the padlock shown on the locked aquarium cards of the supplied UI mockup. Image is style reference only. Front-facing chunky rounded golden-yellow padlock body, dark brown keyhole, silver steel U-shaped shackle, thick dark navy outline, simple smooth glossy highlights, playful casual aquarium game illustration. Symmetric upright icon, centered, entire silhouette fits with narrow margins, square canvas. Actual transparent alpha background. No background, no aquarium, no fish, no text, no border, no card. Clear readable silhouette suitable at 64 pixels. Output transparent PNG.

Empty tank transparency correction prompt: Use case: background-extraction. Preserve this empty aquarium exactly. Remove the checkerboard pattern entirely, replace all area outside aquarium silhouette with actual alpha 0 transparent pixels. Checkerboard must NOT be painted in image. Export real RGBA PNG transparency. Aquarium and all interior unchanged. No fish or lock. Transparent cutout sprite.
