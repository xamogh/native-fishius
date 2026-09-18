# Contained fish details mockup

Created with the built-in image generation tool. Source: `08-artistic-simple-layout.png`. Output: `09-contained-artwork-panels.png`.

## Implementation structure

Keep the existing dialog frame, title bar, close button, backdrop and event handling. Change only its content.

| Container | Contents | Background |
| --- | --- | --- |
| Fish profile, left | Portrait above the description, habitat and diet | Cream rounded panel with a clipped rectangular aquarium background at the top |
| Care, upper right | Four icon/value groups and a feeding note | One cream rounded panel |
| Growth, lower right | Heading and aligned five-row reward table | Cream panel, sage column header and pale green adult row |
| Price, bottom right | Purchase cost and adult profit breakdown | Cream rounded strip |

Use a two-column layout at roughly 42/58, consistent gaps and shared edges. Avoid decorative overlaps. Use the same corner radius, border and panel fill throughout.

The portrait water/plants/rocks should be one reusable image with no text and no fish. Draw the existing species sprite over it and fit it by aspect ratio. Clip the background to the portrait rectangle. Its bottom edge must end cleanly before the text region.

Keep names, badges, descriptions, values, headings, row separators and row highlights as native UI drawing and text. Do not bake them into a background image. Use separate existing icon sprites where possible. The mockup is a reference, not an asset to stretch across the dialog.

Populate care, profit, times and rewards from the species and the current game quote. Do not hardcode the Neon Tetra example across species. Use species-specific factual description data. For permanent companions, adapt the growth and reward sections to their actual behavior. Do not reintroduce growing-slot information.

The image generator may slightly redraw the original surrounding pixels. In implementation, leave the actual existing container unchanged.

## Final prompt

Use case: ui-mockup edit.
Refine this approved fish-details mockup so its art direction is easy to implement using rectangular UI containers and separate image assets.

STRICT INVARIANTS:
Preserve the original blue OUTER DIALOG container, exact size, position, corners, wave title bar, centered NEON TETRA heading, red square X close button and dimmed shop background. Keep the source aspect ratio and overall two-column composition. Edit ONLY the cream inner content. No redesigned outer frame.

DESIGN GOAL:
Artistic but clean, with every texture and illustration fully contained in its own regular rounded rectangle. Attractive painted underwater illustration, friendly readable typography, small polished illustrated icons, warm ivory and soft aqua palette. The art lives in images and subtle panel fills; the structure is simple regular boxes. No decorative leaves, brushstrokes, water washes or shadows crossing container boundaries. No irregular torn-paper edges or overlapping panels. Consistent modest corner radii, 16-20 px gutters, aligned edges, crisp padding.

CONTAINER STRUCTURE, all within existing dialog:
1. LEFT COLUMN, approximately 40%: one neat rounded rectangular fish profile card. Its upper portion is a self-contained rectangular underwater background image, fully clipped to the card: aqua water, gentle hand-painted light rays, stones and aquatic plants tucked inside its bottom corners. Overlay the big recognizable blue-and-red fish centered in that image. Small regular cream rounded badges "Common" and "Level 1" at its top. The image ends at one CLEAN HORIZONTAL EDGE. Below is a simple solid pale ivory text region with no artwork behind the writing. Show the full About content here. Think of this as a fixed background image plus separate fish sprite plus live text.
2. RIGHT COLUMN TOP: one pale cream rounded rectangular care panel. Heading "In your aquarium". Four neatly aligned icon/label/value groups in a 2-by-2 arrangement INSIDE this one panel, not four extra cards. Under the groups a single quiet line "Growth pauses when hungry." Everything contained; no plants or decoration in this panel.
3. RIGHT COLUMN MIDDLE/BOTTOM: one pale cream rounded rectangular panel headed "Growth & rewards". Inside it a simple three-column table with a gentle sage header, regular equal-height rows, light horizontal rules only, tiny fish and coin icons. Last Adult row has a simple rectangular pale-green highlight, no paint strokes.
4. RIGHT COLUMN FOOTER: one short cream rectangular strip aligned perfectly with the panels above. Price and profit breakdown separated with a thin line.
The right panels and left profile card must share top and bottom alignment. Use very subtle borders/shadows only; avoid heavy nested frames. No texture in text regions except an almost imperceptible uniform warm paper grain.

KEEP ALL CONTENT, do not simplify by removing details:
Left: "Common"; "Level 1"; "About this fish"; italic "Paracheirodon innesi"; "A small freshwater fish from the upper Amazon, known for its bright blue and red coloring."; "Habitat: Upper Amazon"; "Diet: Small insects, crustaceans and plants".
Care panel heading: "In your aquarium".
Four facts: "Hatches in" / "6 sec"; "Feed every" / "10 min"; "Adult in" / "20 min"; "Adult profit" / "+6 coins".
Note: "Growth pauses when hungry."
Table heading: "Growth & rewards".
Column labels: "Stage", "Time from purchase", "Sale reward".
Five rows exactly: "Baby | 6 sec | 5 coins"; "Junior | 5 min | 4 coins"; "Young | 11 min | 6 coins"; "Mature | 16 min | 8 coins + 1 XP"; "Adult | 20 min | 11 coins + 2 XP".
Footer: "Shop price: 5 coins" and "Includes 5 coins back + 6 coins profit".

Keep the details legible. Maintain the character of a delightful aquarium game, not a corporate dashboard. Do not reintroduce growing-slot information. No annotations, dimensions, wireframe labels, browser chrome, watermark or phone device. This is a polished final-looking screen mockup, designed from straightforward implementable boxes.
