# Funds dialog artwork

The coin and pearl dialogs share the approved orange octopus composition. Each image supplies the body artwork. The game draws the amount, frame, close control and Open Shop button separately so they remain live controls.

| Asset | Purpose | Generation prompt |
| --- | --- | --- |
| [funds-octopus-coins-v1.png](funds-octopus-coins-v1.png) | Octopus holding a gold star coin | [Provenance and prompt](funds-octopus-coins-v1.provenance.json) |
| [funds-octopus-pearls-v1.png](funds-octopus-pearls-v1.png) | Matching octopus holding an iridescent pearl | [Provenance and prompt](funds-octopus-pearls-v1.provenance.json) |
| [funds-underwater-v1.png](funds-underwater-v1.png) | Clean backdrop behind the smaller scene in compact windows | [Provenance and prompt](funds-underwater-v1.provenance.json) |

All three PNGs were made with the built-in `image_gen.imagegen` tool and copied unchanged. They are opaque images. The native renderer rounds the scene to the dialog body and feathers its edges when compact spacing requires a smaller scene.

The foreground uses about 22% to 77% of each scene's height. `src/hud_funds.cpp` fits that area between the message and button. The asset manifest records each file's dimensions and SHA-256 hash.

The [approved mockup](../../design/mockups/funds-octopus-2026-09-17/approved-coins.png) and [its prompt](../../design/mockups/funds-octopus-2026-09-17/provenance.json) are retained in the project. [Native visual checks](../../evidence/funds-octopus/design-qa.md) document the implementation and its small rendering differences.

## Level-up dialog artwork

The level-up dialog uses the [approved horizontal grid](../../design/mockups/level-up-2026-09-18/horizontal-grid/approved.png). Its compact badge sits above two rows of cards. Cards scroll sideways together, while rewards and Continue stay fixed. Landscape phones use one taller row with names below the artwork and Continue beside the rewards. Coins and pearls fly from the reward row to the HUD after a 500-millisecond pause.

| Asset | Purpose | Generation prompt |
| --- | --- | --- |
| [level-up-cabinet-background-v1.png](level-up-cabinet-background-v1.png) | Quiet ivory and aqua interior with corner sea plants | [Provenance](level-up-cabinet-background-v1.provenance.json) |
| [level-up-cabinet-background-portrait-v1.png](level-up-cabinet-background-portrait-v1.png) | Portrait interior with natural corner art proportions | [Provenance](level-up-cabinet-background-portrait-v1.provenance.json) |
| [level-up-cabinet-badge-v1.png](level-up-cabinet-badge-v1.png) | LEVEL ribbon and blank gold medal for the live number | [Provenance](level-up-cabinet-badge-v1.provenance.json) |
| [level-up-cabinet-card-v1.png](level-up-cabinet-card-v1.png) | Empty mint card with a small sand shelf | [Provenance](level-up-cabinet-card-v1.provenance.json) |
| [level-up-cabinet-next-v1.png](level-up-cabinet-next-v1.png) | Next arrow, mirrored for Previous | [Provenance](level-up-cabinet-next-v1.provenance.json) |

These are opaque outputs from built-in Create Image. The renderer feathers the badge's cream margins into the background and clips arrow art to a circle. Its live level number uses the blank medal face at normalized bounds (0.370, 0.330, 0.260, 0.510). All item and currency sprites come from the existing game assets. The frame, labels, amounts, card outlines and controls remain native UI. The manifest records dimensions and SHA-256 hashes. Earlier artwork and the previous approved mockup remain as design history.

## Mobile-first level receipt, 18 September 2026

The latest [selected phone design](../../design/mockups/level-up-2026-09-18/mobile-first/approved.png) supersedes the cabinet layout above. It uses a fixed left receipt and Continue button, a vertical divider, and a single row of square cards with names below the item artwork. The visible next card and position marks indicate horizontal scrolling. Portrait layouts stack these sections.

| Asset | Purpose | Generation prompt |
| --- | --- | --- |
| [level-up-mobile-background-v1.png](level-up-mobile-background-v1.png) | Quiet cream interior with soft seafoam edges | [Provenance](level-up-mobile-background-v1.provenance.json) |
| [level-up-mobile-card-v1.png](level-up-mobile-card-v1.png) | Square pale aquarium card with a clear caption area | [Provenance](level-up-mobile-card-v1.provenance.json) |
| [level-up-mobile-badge-v1.png](level-up-mobile-badge-v1.png) | Tightly framed LEVEL medal with a blank face | [Provenance](level-up-mobile-badge-v1.provenance.json) |

All three mobile assets use built-in Create Image. The item art and currency icons are reused. Native text and controls remain interactive. The manifest records dimensions and SHA-256 hashes. Earlier art remains as design history.
