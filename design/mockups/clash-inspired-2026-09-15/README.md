# Fishius: Clash-inspired mockups

Created on 15 September 2026 using the built-in ImageGen tool.

These are two static concept images for the existing landscape aquarium game. The display order in the conversation is the same as the file numbering.

- [1. Reef Home](01-reef-home.png): aquarium scene with controls around the edges, raised Food and Shop buttons, resource counters, and a tank selector.
- [2. Fish Market](02-fish-market.png): fish shop with connected category tabs, four large fish cards, and clear prices.
- [Exact generation prompts](prompts.md)

## Design direction

The references suggest a reusable set of visual choices: thick dark outlines, broad raised button faces, short lower bevels, pale panels, bold outlined headings, and large illustrated icons. The mockups apply those choices to Fishius using cyan water, warm ivory panels, teal secondary controls, lime green purchase and feeding controls, and gold shop emphasis.

The aquarium mockup uses the existing fish species, gold star coins, pearls, XP, tank capacity, and navigation. The shop mockup uses the same button and typography treatment.

## Sources

- [User reference gallery](https://interfaceingame.com/games/clash-of-clans/)
- [Clash of Clans gameplay reference](https://interfaceingame.com/screenshots/clash-of-clans-collect-resources/)
- [Clash of Clans shop reference](https://interfaceingame.com/screenshots/clash-of-clans-gems/)
- Local copies of the two Clash screenshots are in [references](references/). These are third-party reference images, not runtime assets.
- Existing Fishius aquarium: `evidence/lagoon-redesign/aquarium.png`.
- Existing Fishius shop: `evidence/shop-dialog-size/desktop.png`.
- Existing shared visual rules: `assets/lagoon/README.md`.
- Shop values: `assets/content.json`, read on 15 September 2026.

## Catalog values used

| Fish | Coin price | Time to adult | Adult rehome coins |
| --- | ---: | --- | ---: |
| Neon Tetra | 5 | 20m | 11 |
| Guppy | 10 | 2h | 49 |
| Platy | 75 | 20h | 375 |
| Molly | 152 | 44h | 759 |

The balances and capacity are sample game state. The shop summary is intentionally brief for this visual exploration; it does not show all reward information.

## Review

Both generated images were visually inspected. The eight visible fish match the aquarium capacity. The shop has all four intended fish, legible prices and adult times, one close control, and complete card bounds. The generated text matches the specified labels and catalog values.

The prompts requested 1920 x 1080 landscape compositions. Files retain the generator's native output dimensions without rescaling. These are visual mockups, not implemented or interaction-tested screens.

## Original generated files

- Reef Home: `/Users/amoghrijal/.codex/generated_images/01a0a344-c13d-7622-bdc0-d5c3860244aa/exec-1bf61f15-d2b6-492a-8135-8802bf9239c6.png`
- Fish Market: `/Users/amoghrijal/.codex/generated_images/01a0a344-c13d-7622-bdc0-d5c3860244aa/exec-14a6e856-a940-44f2-b6a7-9d95124f6bb7.png`

The workspace copies above are the saved deliverables.
