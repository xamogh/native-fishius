# Exact generation prompts

Mode: built-in ImageGen. Reference paths below are relative to the repository root unless noted.

## 1. Egg placement

```text
Use case: ui-mockup
Concept name: Placing Guppy Eggs
Create ONE production-quality 1920 x 1080 landscape screenshot mockup of Fishius, our existing native aquarium game. The user's correction is crucial: the mockup must be true to the existing game. This is an art-directed refinement of our real egg-placement screen, with its actual controls, 2D fish and rules.
INPUT REFERENCES, IN PRIORITY ORDER:
1. Real Fishius egg-placement screenshot: authoritative UI locations, interaction and screen organization. Replace its old test balances and price with the exact current values below. Do not copy mesh-debug stripes or tiny rendering glitches.
2. Real Fishius aquarium background: use THIS open sandy underwater environment. Retain cyan water, soft shafts of light, a wide pale sand floor and a few faceted rocks at the far edges.
3. Real Fishius egg sprite: preserve the cream egg shape with mint spots and a cyan lower glow.
4. Real Fishius Platy sprite: species/art identity reference. Match our glossy painted 2D fish style and friendly eyes.
5. Clash of Clans gameplay screenshot: secondary inspiration ONLY for readable bold lettering, tactile button depth and clear grouping. Fishius takes priority for layout, palette, shapes and gameplay.

The frame shows the real action after a player has bought and placed TWO Guppy eggs at level 1. There are FOUR small starter fish in the water, one blue/red Neon Tetra, one blue-bodied orange-tail Guppy, one orange Platy, one black Molly, plus TWO small cream-and-mint eggs. They total six growing slots. Each egg cost 10 coins. Buying eggs grants no XP. Both eggs have just been placed and have not yet hatched. No extra fish silhouettes, no floating coins, no instant income, no breeding UI.
Our fish are small side-view sprites, roughly 90 to 135 pixels wide on this canvas, swimming at different positions in the middle water. The eggs are about 42 pixels tall, placed at two separate water positions. Keep over 65% of the water and sand visually open, as in the actual game. This is a simple player tank: no built-in dense coral jungle, buildings, treasure chest, fantasy creatures or new decorations. No aquarium glass frame around the whole scene.

EXACT UI AND POSITIONS:
- Top left: the existing gold star badge with "1", joined to the teal XP bar reading "0 / 80 XP".
- Top right: two compact teal horizontal currency counters, gold star coin icon "230" with green plus, iridescent pearl icon "0" with green plus. To their right, one envelope control and one gear control in small teal squares. Keep currencies and utility buttons in that order.
- Right edge: preserve THREE cream rounded-square navigation buttons in a vertical column, in order "MASTERY" with gold star, "PLAY" with teal triangle, and "FOOD" with the familiar blue food jar. This order and edge must not change.
- Bottom right: the existing LARGE ROUND yellow "SHOP" button with a red-and-white fish shop awning. It must remain round and in its existing position.
- Lower left: two small teal square controls, net above bag/storage box.
- Bottom left: the existing teal tank selector with its glass jar/fish icon, blue left/right arrows and exact three-line copy "Tank 1", "Growing 6/10", "Display 0/8". Growing slots and display slots remain separate.
- Near bottom center, above the sand and between the two corner controls: the real compact dark-teal placement strip. Exact white copy "Tap to place Guppy. 10 coins each". At its right end, one raised green button "DONE". This is the focused action. Keep the strip around 800px wide, not the entire screen width.
No screen title, logo, tutorial panel, extra growth bar, extra action or marketing copy.

ART DIRECTION:
Preserve the real Fishius palette: bright cyan and turquoise, pale cream, navy lettering, warm yellow and green action accent. Keep its cream rounded navigation surfaces and strong teal borders. Add modest lower bevels and clean highlights for the tactile clarity of the Clash reference. Typography is Baloo 2-like headings and Nunito-like labels, as in Fishius, at most two type styles. White letters only on dark controls, deep navy on cream. No heavy black outlined white type on every label. Clean alignment, readable labels, generous spacing, all controls within safe frame margins. Keep the same scale hierarchy and clear water as our actual screen. This must look like Fishius in play.

OUTPUT: one cohesive game screen only. No device bezel, OS status bar, browser chrome, collage, external labels, presentation frame or watermark. Do not implement or depict features absent from the brief. Preserve the input fish designs and actual mechanics.
```

Attached references, in order:

1. `evidence/egg-placement/852x393/placed.png`
2. `assets/lagoon/reef.png`
3. `assets/ui/egg.png`
4. `assets/species/platy.png`
5. `design/mockups/clash-inspired-2026-09-15/references/clash-gameplay.png`

## 2. Fullscreen fish shop

```text
Use case: ui-mockup
Concept name: Fullscreen Fish Shop
Create ONE high-fidelity 1920 x 1080 landscape mobile game shop screenshot for Fishius. The user explicitly requests a FULLSCREEN shop just like the attached Clash of Clans shop, while remaining true to our actual aquarium game.
Reference priority:
1. Clash of Clans Gems shop: primary reference for FULLSCREEN structure, large connected top tabs, pale title band, dark merchandise tray, strong item cards and fixed bottom currency strip.
2. Current Fishius fish shop screenshot: use its actual species identity and aquatic art family, not its old modal layout, old prices or old durations.
3. Actual Fishius Guppy sprite: preserve this blue-silver body, broad orange fan tail, friendly large eye and glossy painted 2D style.
4. Actual Fishius Neon Tetra sprite: preserve this blue/red side-view fish design.
5. Actual Fishius Platy sprite: preserve this compact orange fish design.
Preserve Fishius gold star coins, iridescent pearls, fish species, five real shop categories, and the egg-placement purchase flow.

CRITICAL FULLSCREEN COMPOSITION:
The shop fills the ENTIRE rectangular image, edge to edge. No aquarium visible behind it, no floating dialog, no rounded outer modal frame, no scene border or margins around the shop, no water peeking around edges. No gameplay HUD, side navigation, round Shop button, Mastery button or tank-selector jar floating outside the shop. The entire canvas IS the shop interface.
- Top 155px: a deep petrol-blue band, five large attached chunky icon tabs centered across it. Exact labels in order: "Fish", "Plants", "Decorations", "Environment", "Food". Icons: our friendly clownfish, green aquatic leaves, coral, faceted rock, blue food jar. Active Fish tab is warm ivory and joins the title band below; inactive tabs are desaturated stone-gray/teal, with a strong lower bevel. Use generous wide tabs and readable labels.
- Top-right corner: one red raised rounded-square close button with a white "X". Small "SHOP" heading at top-left.
- y=155..275: a flat, full-width warm ivory title band. Center the large chunky heading "Fish". Beneath it, exact instruction "Choose a fish, then tap the water to place its egg." No marketing banner or decorative hero.
- y=275..960: one full-width dark charcoal-teal merchandise tray. Four equal tall item cards in a single row, with narrow even gutters. Every card is completely visible. Together they use nearly all horizontal space. Card faces are muted aqua, simple clean radial light behind the large fish, short dark lower bevel, restrained top highlight. No detailed aquarium scene inside cards. Large bold outlined white names at the top; beautifully rendered side-view fish in the upper middle; three compact dark-teal text lines on a pale aqua summary area; large lime-green beveled price button at the bottom. All four cards use identical structure and alignment.
- Bottom 120px: full-width fixed teal-green currency strip, structurally like Clash. At left small contextual text "Tank 1" and "Growing 4/10  |  Display 0/8". In the center, our gold star coin icon with "250" and a small plus control, and our iridescent pearl icon with "0" and a small plus control. At right, small previous/next arrows and four pagination dots, first selected. Keep everything inside this shop strip, with no duplicate resource bars at the top.

EXACT CURRENT LEVEL-1 CATALOG COPY, LEFT TO RIGHT:
Card 1:
"Neon Tetra"
"Adult: 20m while fed"
"Collect: 11 coins"
"Profit: 6  |  2 XP"
Green button: gold star coin icon + "5"

Card 2:
"Guppy"
"Adult: 2h while fed"
"Collect: 49 coins"
"Profit: 39  |  10 XP"
Green button: gold star coin icon + "10"

Card 3:
"Platy"
"Adult: 20h while fed"
"Collect: 375 coins"
"Profit: 300  |  80 XP"
Green button: gold star coin icon + "75"

Card 4:
"Molly"
"Adult: 44h while fed"
"Collect: 759 coins"
"Profit: 607  |  162 XP"
Green button: gold star coin icon + "152"
Molly is the real black aquarium fish with the tall dark dorsal fin, matching the current Fishius shop screenshot.

ART DIRECTION:
Borrow the Clash shop's hierarchy and tactile graphic clarity closely: broad clean shapes, thick dark-teal outlines, chunky rounded-square buttons, short lower shadows, ivory title surface, compact typography and large collectible art. Use Fishius cyan, warm ivory, teal, green and gold. Balance its existing friendly Baloo 2 / Nunito-like lettering with bold outlined display names. At most two font styles. Body copy is clean dark type on pale aqua with strong contrast. Cards should feel roomy and product-ready. Use real illustrated icons, not emoji.

No additional categories, no gems or elixir, no lootboxes, no combat, no offers or real-money prices, no premium claims, no currency rewards on purchase. Buying selects a species, then the player places and pays for eggs in the aquarium as the instruction states. No filters or invented sort options. No device, bezel, browser chrome, presentation board, annotations, watermark or external title. This must read unmistakably as a FULLSCREEN Fishius shop inspired by the attached Clash screenshot.
```

Attached references, in order:

1. `design/mockups/clash-inspired-2026-09-15/references/clash-gems.png`
2. `evidence/shop-dialog-size/desktop.png`
3. `assets/species/guppy.png`
4. `assets/species/neonTetra.png`
5. `assets/species/platy.png`

## 3. Fullscreen decorations shop

```text
Use case: ui-mockup
Concept name: Fullscreen Decorations Shop
Create ONE high-fidelity 1920 x 1080 landscape Fishius game screenshot showing the FULLSCREEN Decorations shop. This is a matching second category state of the fullscreen Fish shop in input Image 1, keeping its chrome, layout, typography, colors, button shapes, broad card structure and edge-to-edge composition.

Input images:
1. Newly designed fullscreen Fishius Fish shop: main layout and design-system reference. Preserve the full-width dark navy top tab band, warm ivory title band, charcoal-teal merchandise tray, four aqua cards and fixed teal currency footer.
2. Actual River Pebble Pair sprite, CD-01: preserve its asymmetric two smooth stones with a small moss-green accent.
3. Actual Spiral Shell sprite, CD-02: preserve its broad peach/cream spiral, coral bands and plum-pink opening.
4. Actual Terracotta Plant Pot sprite, CD-03: preserve its squat tilted orange clay pot, thick chipped rim and dark interior. No plant inside.
5. Actual Driftwood Fork sprite, CD-04: preserve the short Y-shaped cinnamon-brown branch, rounded cut ends and large knot.

The user explicitly asked for a fullscreen shop just like Clash of Clans, but true to Fishius. This screen must look like another tab of the SAME shop as Image 1.
- Shop fills the ENTIRE rectangular canvas, no floating modal, no outer frame or rounded overall container, no surrounding aquarium, no gameplay controls.
- Keep "SHOP" top-left and one red X top-right.
- Keep the same five large connected tabs in the same positions and order: "Fish", "Plants", "Decorations", "Environment", "Food", with the same fish, plant, coral, rock and blue food-jar icons. This time Decorations is active in ivory, joined to the title band. Fish is inactive like the other unselected tabs.
- Full-width ivory title band: large central heading "Decorations", then the exact smaller instruction "Choose an item, position it, then tap the green tick to buy."
- In the full-width dark merchandise tray, four equally sized tall aqua cards across a single row, all fully visible, using the SAME card proportions and column alignment as Image 1. Large exact catalog names at top. For the longer names, use intentional two-line breaks within a common fixed title area. Give all illustrations the same generous space and keep all stats and buy buttons aligned across the row.
- Retain the restrained radial aqua light behind each actual item. The object should be a clean, faithful rendition of the supplied sprite, not a new fantasy ornament. Maintain the matte, broad-shaded art style of our actual decorations even though the UI has glossy controls.
- Each card has three readable dark-teal lines on a pale aqua summary area, then a large raised lime-green price button with our gold STAR coin icon.
EXACT CARD CONTENT, LEFT TO RIGHT:

"River Pebble Pair"
"Common · Level 1"
"Tank score: 10"
"Permanent item"
Coin price button: "70"

"Spiral Shell"
"Common · Level 2"
"Tank score: 10"
"Permanent item"
Coin price button: "70"

"Terracotta Plant Pot"
"Common · Level 3"
"Tank score: 10"
"Permanent item"
Coin price button: "110"

"Driftwood Fork"
"Common · Level 4"
"Tank score: 20"
"Permanent item"
Coin price button: "305"

These are the current real catalog prices. The player is at level 4, so all four are available. Buying decor grants NO account XP, so do not add any XP reward line, first-purchase bonus, passive coin generation or earning rate. Owned decorations are permanent items and use preview/confirm placement.

Fixed bottom strip, same layout as Image 1:
- Left two lines: "Tank 1 · Level 4" and "Growing 8/10  |  Display 1/8".
- Center: gold star coin icon, "1,500", small plus; iridescent pearl icon, "3", small plus.
- Right: small previous/next controls and four pagination dots, first selected.
Use the same blue/teal/gold/ivory and lime palette, subtle lower bevels, strong clean dark-teal outlines, chunky display headings and readable rounded supporting text. At most two fonts. Preserve generous spacing. This is one focused shopping screen, no extra content boxes, filters, offers, decorative banners or extra categories.

Make the full-screen difference from the previous floating Fishius dialog unmistakable: the interface reaches all four image edges, and the aquarium world is entirely replaced by the shop. No device, browser chrome, OS status bar, presentation board, watermark, external captions or option labels. All names, prices, scores and levels must be accurate and readable.
```

Attached references, in order:

1. `design/mockups/fishius-2026-09-15-v2/02-fullscreen-fish-shop.png (original generated path attached)`
2. `assets/decor/catalog/CD-01.png`
3. `assets/decor/catalog/CD-02.png`
4. `assets/decor/catalog/CD-03.png`
5. `assets/decor/catalog/CD-04.png`
