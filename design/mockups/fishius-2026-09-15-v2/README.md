# Fishius mockups: game flows and fullscreen shop

Created on 15 September 2026 with the built-in ImageGen tool. The user asked for more mockups grounded in Fishius, then requested a fullscreen shop inspired by Clash of Clans.

## Images

The numbers match their display order in this set.

1. [Egg placement](01-egg-placement.png)
2. [Fullscreen fish shop](02-fullscreen-fish-shop.png)
3. [Fullscreen decorations shop](03-fullscreen-decorations-shop.png)

[Exact generation prompts](prompts.md)

All three images are 1672 x 941 pixels. They retain the generator's native dimensions. The prompts requested 1920 x 1080 landscape compositions.

## Fullscreen shop

The fish and decoration screens share five connected category tabs, a full-width ivory heading, four large item cards and a fixed currency strip. The shop covers the entire viewport. The tabs are Fish, Plants, Decorations, Environment and Food.

The fish screen retains growth time while fed, total return, profit, XP and the egg-placement flow. Its level 1 sample prices and returns come from the current catalog.

| Fish | Price | Adult time while fed | Total coins | Profit | XP |
| --- | ---: | --- | ---: | ---: | ---: |
| Neon Tetra | 5 | 20m | 11 | 6 | 2 |
| Guppy | 10 | 2h | 49 | 39 | 10 |
| Platy | 75 | 20h | 375 | 300 | 80 |
| Molly | 152 | 44h | 759 | 607 | 162 |

The decoration screen uses a level 4 sample state, so all four shown items are available. Selecting an item opens placement; the green tick confirms the purchase. Decor gives no account XP under the current rules.

| Decoration | Price in coins | Unlock level | Tank score |
| --- | ---: | ---: | ---: |
| River Pebble Pair | 70 | 1 | 10 |
| Spiral Shell | 70 | 2 | 10 |
| Terracotta Plant Pot | 110 | 3 | 10 |
| Driftwood Fork | 305 | 4 | 20 |

## Egg placement

The aquarium image uses the existing sandy background, four starter fish, two purchased Guppy eggs and fixed peripheral controls. The sample wallet is 230 coins after two 10-coin eggs. Buying eggs grants no XP. The tank shows six growing slots used and zero display slots used.

## Sources

- [Clash of Clans reference gallery](https://interfaceingame.com/games/clash-of-clans/)
- [Fullscreen shop reference](https://interfaceingame.com/screenshots/clash-of-clans-gems/)
- `assets/content.json`: current catalog prices, growth durations, returns, XP, decor levels and tank scores.
- `src/skin.cpp`: current shop information and category names.
- `src/decor.cpp`: zero XP for decor purchases.
- `src/decor_view.cpp`: preview and confirmation for decor placement.
- `src/details.cpp`: Keep and Rehome reward behavior.
- `README.md`: current play rules.
- `evidence/egg-placement/852x393/placed.png`: real egg-placement layout.
- `evidence/shop-dialog-size/desktop.png`: real fish shop and species.
- `assets/lagoon/reef.png`: actual aquarium environment.
- `assets/ui/egg.png`: actual egg artwork.
- `assets/species/guppy.png`, `neonTetra.png` and `platy.png`: species art references.
- `assets/decor/catalog/CD-01.png` through `CD-04.png`: actual decoration art references.

## Review

Both fullscreen shop images were visually checked for complete card bounds, active category, consistent navigation and footer, readable copy, correct prices and matching item identities. All four fish profit figures equal total coins minus purchase price. The decoration values match the current catalog. The egg count matches its displayed growing capacity.

These are generated visual mockups grounded in the supplied artwork. They are not implemented screens, and no runtime behavior has changed. No code tests were needed for these static images.

## Original generation files

- Egg placement: `/Users/amoghrijal/.codex/generated_images/01a0a344-c13d-7622-bdc0-d5c3860244aa/exec-966a99d2-e6ff-4e47-a33f-05fff64a7dd4.png`
- Fish shop: `/Users/amoghrijal/.codex/generated_images/01a0a344-c13d-7622-bdc0-d5c3860244aa/exec-847bb0d9-4390-41ac-99a2-9f4ff74a5022.png`
- Decorations shop: `/Users/amoghrijal/.codex/generated_images/01a0a344-c13d-7622-bdc0-d5c3860244aa/exec-b70b8a4c-190d-4b7c-a713-6c97ff174027.png`

The numbered workspace PNGs above are the saved deliverables.
